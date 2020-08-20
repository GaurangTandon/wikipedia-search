#include<string>
#include<vector>
#include<locale>
#include <assert.h>
#include "../libstemmer_c/include/libstemmer.h"

// O3 will optimize the for loops out
// https://godbolt.org/z/bT9398
bool validChar(char c) {
    char ranges[][2] = {
            {'a', 'z'},
            {'A', 'Z'},
            {'0', '9'}
    };
    char singles[] = {'_', '$'};

    for (auto r : ranges) {
        if (c >= r[0] and c <= r[1]) return true;
    }

    for (auto s : singles) {
        if (c == s) return true;
    }

    return false;
}

sb_stemmer *stemmer = nullptr;

std::string stemming(const char *word, const int len) {
    if (not stemmer) {
        stemmer = sb_stemmer_new("porter2", nullptr);
    }

    const sb_symbol *res = sb_stemmer_stem(stemmer, reinterpret_cast<const sb_symbol *>(word), len);
    return std::string(reinterpret_cast<const char *>(res));
}

std::vector<std::string> processText(std::string text) {
    // tokenize
    // stopwords removal
    // stemmer

    int len = text.size();
    std::vector<std::string> stemmedTokens;

    for (int left = 0; left < len; left++) {
        if (not validChar(text[left])) continue;

        int right = left;
        while (right < len - 1 and validChar(text[right + 1])) {
            right++;
        }

        int word_len = right - left + 1;
        char* word = (char*) malloc(word_len * sizeof(char));
        for (int i = 0; i < word_len; i++) {
            word[i] = text[i + left];
        }

        stemmedTokens.push_back(stemming(word, word_len));
    }

    return stemmedTokens;
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

