#include "mapreduce.h"
#include "prefix.h"
#include <iostream>
#include <string>

int main(int argc, char const *argv[]) {
    const auto usage = "mapreduce <src> <mnum> <rnum>";
    std::string path;
    std::size_t map_threads, reduce_threads;
    try {
        if (argc < 4) {
            std::cerr << "Not all required arguments were passed. Program usage:\n" << usage;
            exit(1);
        };
        path.assign(argv[1]);
        map_threads = std::stol(argv[2]);
        reduce_threads = std::stol(argv[3]);
    } catch (const std::exception &e) {
        std::cerr << "Invalid argument passed: " << e.what();
        exit(1);
    }

    auto pipeline = mapreduce::make_pipeline(map_threads, reduce_threads, prefix::map, prefix::reduce);
    try {
        pipeline.run(path);
    } catch (const std::exception &e) {
        std::cerr << "Invalid argument passed: " << e.what() << std::endl;
    }
}