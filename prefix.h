#ifndef HW11_MAPREDUCE_PREFIX_H
#define HW11_MAPREDUCE_PREFIX_H

#include <map>
#include <string>
#include <vector>

namespace prefix {
    std::vector<std::pair<std::string, std::string>> map(const std::string &word);

    std::vector<std::string> reduce(const std::vector<std::pair<std::string, std::string>> &prefixes);

    std::vector<std::string> evaluate_unique_prefixes(const std::vector<std::string> &words);
}

#endif //HW11_MAPREDUCE_PREFIX_H
