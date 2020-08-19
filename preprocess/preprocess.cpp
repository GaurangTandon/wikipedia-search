#include<string>
#include<vector>
#include <assert.h>

std::vector<std::string> processText(std::string text) {
    // tokenize
    // stopwords removal
    // stemmer
    return { "TODO" };
}

// reduced time from 2.2s to 0.8s (compared to src.substr(pos, target.size()) == target)
bool fast_equals(const std::string &src, const std::string &target, int pos = 0) {
    int j = 0, i = pos;

    assert(not target.empty());

    while (i < src.size()) {
        if (src[i] != target[j]) return false;
        j++, i++;
        if (j == target.size()) return true;
    }

    return false;
}

