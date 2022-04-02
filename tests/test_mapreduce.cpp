#include "mapreduce.h"
#include "prefix.h"
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

class MRTest : public ::testing::Test {
protected:
    void SetUp() override {
        namespace fs = std::filesystem;
        std::vector<std::string> input{"dog", "cat", "apple", "apricot", "first", "second", "firm"};
        fs::create_directory("mr_test");
        std::ofstream ofs("mr_test/mrfile.txt");
        std::for_each(input.begin(), input.end(), [&] (std::string &s) { ofs << s << "\n"; });
    }

    std::vector<std::string> as_vector(const std::string &path) {
        std::string line;
        std::vector<std::string> result;
        std::ifstream ifs(path, std::ios::in);
        while (std::getline(ifs, line)) result.push_back(line);
        return result;
    }

    void TearDown() override {
        std::filesystem::remove_all("mr_test");
    }
};

TEST_F(MRTest, test_map_task) {
    namespace mr = mapreduce;
    auto ranges = mr::split_file("mr_test/mrfile.txt", 12);
    mr::map_task t("mr_test/mrfile.txt", ranges[0], prefix::map);
    auto results = t.run();
    ASSERT_EQ(results, (mr::map_task::result_t {{"d", "dog"}, {"do", "dog"}, {"dog", "dog"}}));
}

TEST_F(MRTest, test_reduce_task) {
    namespace mr = mapreduce;
    auto ranges = mr::split_file("mr_test/mrfile.txt", 1);
    mr::map_task mt("mr_test/mrfile.txt", ranges[0], prefix::map);
    auto map_results = mt.run();
    auto shuffle_results = mr::shuffle_results({map_results}, 1);
    mr::reduce_task rt(shuffle_results[0], prefix::reduce);
    ASSERT_EQ(as_vector(rt.run()), (mr::reduce_task::result_t {"app", "apr", "c", "d", "firm", "firs", "s"}));
}

TEST(prefix, produce_sorted_result) {
    ASSERT_EQ(
            prefix::evaluate_unique_prefixes({"dog", "cat", "apple", "apricot", "first", "second", "firm"}),
            (std::vector<std::string>{"app", "apr", "c", "d", "firm", "firs", "s"})
    );
}

TEST(mapreduce, shuffle_to_one) {
    namespace mr = mapreduce;
    ASSERT_EQ(
            mr::shuffle_results(
                    {{{"d", "dog"}, {"o", "dog"}, {"g", "dog"}},
                     {{"c", "cat"}, {"a", "cat"}, {"t", "cat"}}}, 1
            ),
            (
                    mr::sequence<mr::reduce_task::input_t>{
                            {{"a", "cat"}, {"c", "cat"}, {"d", "dog"}, {"g", "dog"}, {"o", "dog"}, {"t", "cat"}}
                    }
            )
    );
}

TEST(mapreduce, shuffle_to_two) {
    namespace mr = mapreduce;
    const auto &results = mr::shuffle_results(
            {{{"d", "dog"}, {"o", "dog"}, {"g", "dog"}},
             {{"c", "cat"}, {"a", "cat"}, {"t", "cat"}}}, 2
    );
    ASSERT_EQ(
            results,
            (
                    mr::sequence<mr::reduce_task::input_t>{
                            {{"a", "cat"}, {"c", "cat"}, {"d", "dog"}},
                            {{"g", "dog"}, {"o", "dog"}, {"t", "cat"}}
                    }
            )
    );
}

TEST(mapreduce, shuffle_to_three) {
    namespace mr = mapreduce;
    const auto &results = mr::shuffle_results(
            {{{"d", "dog"}, {"o", "dog"}, {"g", "dog"}},
             {{"c", "cat"}, {"a", "cat"}, {"t", "cat"}}}, 3
    );
    ASSERT_EQ(
            results,
            (
                    mr::sequence<mr::reduce_task::input_t>{
                            {{"a", "cat"}, {"c", "cat"},},
                            {{"d", "dog"}, {"g", "dog"}},
                            {{"o", "dog"}, {"t", "cat"}}
                    }
            )
    );
}

TEST(mapreduce, split_file) {
    namespace fs = std::filesystem;
    const char *path = "test_input.txt";

    // 1 section
    auto sections = mapreduce::split_file(path, 1);
    ASSERT_EQ(sections.size(), 1);

    decltype(sections) one_sec{{0, fs::file_size(path)}};
    ASSERT_EQ(sections, one_sec);

    // 2 sections
    sections = mapreduce::split_file(path, 2);
    ASSERT_EQ(sections.size(), 2);

    decltype(sections) two_sec{
            {0,  23},
            {24, 47},
    };
    ASSERT_EQ(sections, two_sec);

    // 3 sections
    sections = mapreduce::split_file(path, 3);
    ASSERT_EQ(sections.size(), 3);

    decltype(sections) three_sec{
            {0,  17},
            {18, 35},
            {36, 47},
    };
    ASSERT_EQ(sections, three_sec);

    // 48 sections
    sections = mapreduce::split_file(path, 48);
    ASSERT_EQ(sections.size(), 1);

    one_sec = {
            {0, 47}
    };
    ASSERT_EQ(sections, one_sec);
}