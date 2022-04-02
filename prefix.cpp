#include "prefix.h"
#include <map>
#include <unordered_set>

namespace prefix {

    std::vector<std::pair<std::string, std::string>> map(const std::string &word) {
        if (word.empty()) return {};

        std::vector<std::pair<std::string, std::string>> prefixes;
        for (size_t ix = 1; ix <= word.size(); ++ix)
            prefixes.emplace_back(word.substr(0, ix), word);
        return prefixes;
    }

    std::vector<std::string> reduce(const std::vector<std::pair<std::string, std::string>> &prefixes) {
        std::vector<std::string> result;
        std::map<std::string, std::vector<std::string>> pfx_counter;
        std::unordered_set<std::string> added;

        for (auto &pfx : prefixes) pfx_counter[pfx.first].push_back(pfx.second);
        result.reserve(pfx_counter.size());
        for (auto &it : pfx_counter) {
            if (it.second.size() > 1) continue;
            if (added.find(it.second[0]) != added.end()) continue;
            else added.insert(it.second[0]);
            result.push_back(it.first);
        }

        return result;
    }

    std::vector<std::string> evaluate_unique_prefixes(const std::vector<std::string> &words) {
        std::vector<std::pair<std::string, std::string>> prefixes;
        for (auto &w : words) {
            auto pfx = map(w);
            prefixes.insert(prefixes.end(), pfx.begin(), pfx.end());
        };
        return reduce(prefixes);
    }
}