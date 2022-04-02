#ifndef HW11_MAPREDUCE_MAPREDUCE_H
#define HW11_MAPREDUCE_MAPREDUCE_H

#include <cstddef>
#include <functional>
#include <future>
#include <string>
#include <utility>
#include <vector>

namespace mapreduce {

    template<typename T> using sequence = std::vector<T>;
    template<typename K, typename V> using tuple = std::pair<K, V>;
    template<typename T> using range = std::pair<T, T>;
    using key_t = std::string;
    using value_t = std::string;
    using range_t = range<std::size_t>;

    sequence<range_t> split_file(const std::string &path, std::size_t num_of_parts);

    /*
        map (k1, v1) --> list(k2, v2)
     */
    class map_task {
    public:
        using result_t = sequence<tuple<key_t, value_t>>;
        using map_fn_t = std::function<result_t(key_t)>;

        map_task(std::string path, range_t range, map_fn_t fn) :
                path_(std::move(path)), range_(std::move(range)), map_fn_(std::move(fn)) {};

        result_t run();

        std::future<result_t> run_async();

    private:
        const std::string path_;
        range_t range_;
        map_fn_t map_fn_;
    };

    /*
        reduce (k2, list(v2)) --> list(v2)
     */
    class reduce_task {
    public:
        using result_t = sequence<value_t>;
        using input_t = sequence<tuple<value_t, value_t>>;
        using reduce_fn_t = std::function<result_t(input_t)>;

        reduce_task(input_t input, reduce_fn_t fn) :
                input_(std::move(input)), reduce_fn_(std::move(fn)) {};

        std::string run();

        std::future<std::string> run_async();

    private:
        std::string dump_results(result_t &results);

        input_t input_;
        reduce_fn_t reduce_fn_;
    };

    class Pipeline;

    Pipeline make_pipeline(
            std::size_t map_threads, std::size_t reduce_threads,
            map_task::map_fn_t map_fn, reduce_task::reduce_fn_t reduce_fn
    );

    class Pipeline {
        Pipeline(
                std::size_t map_threads, std::size_t reduce_threads,
                map_task::map_fn_t map_fn, reduce_task::reduce_fn_t reduce_fn
        ) : map_threads_(map_threads), reduce_threads_(reduce_threads), map_fn_(map_fn), reduce_fn_(reduce_fn) {};
        std::size_t map_threads_, reduce_threads_;
        map_task::map_fn_t map_fn_;
        reduce_task::reduce_fn_t reduce_fn_;

        friend Pipeline make_pipeline(
                std::size_t map_threads, std::size_t reduce_threads,
                map_task::map_fn_t map_fn, reduce_task::reduce_fn_t reduce_fn
        );

    public:
        virtual ~Pipeline() = default;

        void run(const std::string &path);

    };

    sequence<reduce_task::input_t> shuffle_results(
            sequence<map_task::result_t> input, std::size_t parts_num
    );

    template<typename T>
    std::future<decltype(std::declval<T>().run())>
    spawn_task(T *ptr) {
        using result_type = decltype(std::declval<T>().run());
        auto run_fn = [=]() { return ptr->run(); };
        std::packaged_task<result_type()> task(run_fn);
        std::future<result_type> result(task.get_future());
        std::thread t(std::move(task));
        t.detach();
        return result;
    }
}
#endif //HW11_MAPREDUCE_MAPREDUCE_H
