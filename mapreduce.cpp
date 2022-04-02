#include "mapreduce.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <strstream>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>

namespace mapreduce {
    namespace fs = std::filesystem;
    namespace ip = boost::interprocess;

    Pipeline make_pipeline(
            std::size_t map_threads, std::size_t reduce_threads,
            map_task::map_fn_t map_fn, reduce_task::reduce_fn_t reduce_fn
    ) {
        return Pipeline(map_threads, reduce_threads, map_fn, reduce_fn);
    }

    sequence<range<std::size_t>> split_file(const std::string &path, std::size_t num_of_parts) {
        if (!fs::exists(path)) return {};

        std::size_t file_size = fs::file_size(path);
        if (file_size < num_of_parts) {
            return sequence<range<std::size_t>>{{0, file_size}};
        }

        sequence<range<std::size_t>> sections;
        std::size_t sec_size = file_size / num_of_parts;
        ip::file_mapping fm(path.c_str(), ip::read_only);
        ip::mapped_region region(fm, ip::read_only, 0, 0);

        const char *file_begin = static_cast<const char *>(region.get_address());
        const char *file_end = file_begin + region.get_size();
        const char *sec_begin = file_begin, *sec_end = std::min(sec_begin + sec_size, file_end);

        do {
            sec_end = std::find(sec_end, file_end, '\n');
            sections.push_back({sec_begin - file_begin, sec_end - file_begin});

            sec_begin = sec_end + 1;
            sec_end = std::min(sec_begin + sec_size, file_end);
        } while (sec_end <= file_end && sec_begin < file_end);

        return sections;
    }

    sequence<reduce_task::input_t>
    shuffle_results(sequence<map_task::result_t> input, std::size_t parts_num) {
        using map_type = map_task::result_t::value_type;
        auto map_cmp = [](const map_type &lhs, const map_type &rhs) { return lhs.first < rhs.first; };
        std::multiset<map_type, decltype(map_cmp)> merged_results(map_cmp);
        for (auto &r : input) merged_results.insert(r.begin(), r.end());

        const auto merged_size = merged_results.size();
        const auto batch_size = merged_size / parts_num;
        std::size_t counter = 0;
        sequence<reduce_task::input_t> shuffle_results;
        auto it = merged_results.begin();
        for (std::size_t i = 0; i < parts_num && it != merged_results.end(); ++i) {
            reduce_task::input_t batch;
            batch.reserve(batch_size);
            std::size_t added = 0;
            while (counter < merged_size && added < batch_size) {
                batch.push_back(*it);
                ++added, ++counter, ++it;
            }

            shuffle_results.push_back(batch);
        }
        return shuffle_results;
    }

    map_task::result_t map_task::run() {
        ip::file_mapping fm(path_.c_str(), ip::read_only);
        ip::mapped_region region(fm, ip::read_only, range_.first, 0);

        const char *file_begin = static_cast<const char *>(region.get_address());
        std::istrstream input(file_begin, range_.second - range_.first);
        value_t line;
        result_t result;
        while (std::getline(input, line)) {
            auto r = map_fn_(line);
            result.insert(result.end(), r.begin(), r.end());
        }
        return result;
    }

    std::future<map_task::result_t> map_task::run_async() {
        return spawn_task(this);
    }

    std::string reduce_task::run() {
        auto result = reduce_fn_(input_);
        return dump_results(result);
    }

    std::string reduce_task::dump_results(result_t &results) {
        auto path = std::to_string(reinterpret_cast<std::intptr_t> (this));
        std::ofstream ofs(path);
        std::for_each(results.begin(), results.end(), [&](std::string &s) { ofs << s << "\n"; });
        return path;
    }

    std::future<std::string> reduce_task::run_async() {
        return spawn_task(this);
    }

    void Pipeline::run(const std::string &path) {
        auto abs_path = fs::canonical(path).string();
        auto ranges = split_file(abs_path, map_threads_);
        sequence<std::future<map_task::result_t>> map_futures;
        sequence<map_task::result_t> map_results;
        sequence<map_task> map_tasks;

        for (auto &r : ranges) map_tasks.emplace_back(abs_path, r, map_fn_);
        for (auto &t : map_tasks) map_futures.push_back(t.run_async());
        for (auto &f : map_futures) {
            f.wait();
            map_results.push_back(f.get());
        }

        sequence<std::future<std::string>> reduce_futures;
        sequence<reduce_task> reduce_tasks;
        for (auto &r : shuffle_results(map_results, reduce_threads_)) reduce_tasks.emplace_back(r, reduce_fn_);
        for (auto &t : reduce_tasks) reduce_futures.push_back(t.run_async());

        std::cout << "Results saved in files:\n";
        for (auto &f : reduce_futures) {
            f.wait();
            std::cout << f.get() << "\n";
        }
    }
}
