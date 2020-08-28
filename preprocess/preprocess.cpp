#include<string>
#include<vector>
#include<set>
#include <cassert>
#include <iostream>
#include <fstream>
#include <pthread.h>
#include "../libstemmer_c/include/libstemmer.h"

// wouldn't recommend going below 25 because few popular words
// like internationalization are very long
// Full stats: http://norvig.com/mayzner.html
constexpr int MAX_WORD_LEN = 25;

class Preprocessor {
public:
    sb_stemmer *stemmer = nullptr;
    std::set<std::string> stopwords;
    pthread_mutex_t stemmer_mutex = PTHREAD_MUTEX_INITIALIZER;


    Preprocessor() {
        // const char** list = sb_stemmer_list();
        // there is porter2 in this

        stemmer = sb_stemmer_new("porter", nullptr);
        assert(stemmer != nullptr);
        std::ifstream stopwords_file("preprocess/stopwords.txt", std::ios_base::in);
        int count;
        stopwords_file >> count;
        assert(count < 200);
        while (count--) {
            std::string word;
            stopwords_file >> word;
            stopwords.insert(word);
        }
        stopwords_file.close();
    }

    ~Preprocessor() {
        sb_stemmer_delete(stemmer);
    }

// O3 will optimize the for loops out
// https://godbolt.org/z/bT9398
    inline bool validChar(char c) {
        if (c >= 'a' and c <= 'z') return true;
        if (c >= 'A' and c <= 'Z') return true;
        if (c >= '0' and c <= '9') return true;
        if (c == '_') return true;
        if (c == '$') return true;

        return false;
    }

    inline char lowercase(char c) {
        if (c >= 'A' and c <= 'Z') return c + 32;
        return c;
    }

    inline bool isStopword(const char *word) {
        std::string w(word);
        return stopwords.find(w) != stopwords.end();
    }

    // MUST BE CALLED WITH LOCK HELD
    inline std::string stemming(const char *word, const int len) {
        const sb_symbol *res = sb_stemmer_stem(stemmer, reinterpret_cast<const sb_symbol *>(word), len);
        auto ans = reinterpret_cast<const char *>(res);
        auto ret = std::string(ans);

        return ret;
    }

    std::vector<std::string> processText(std::string text) {
        // tokenize
        // stopwords removal
        // stemmer

        int len = text.size();
        std::vector<std::pair<char*, int>> tokens;
        std::vector<std::string> stemmedTokens;

        for (int left = 0; left < len; left++) {
            if (not validChar(text[left])) continue;

            int right = left;
            while (right < len - 1 and validChar(text[right + 1])) {
                right++;
            }

            int word_len = right - left + 1;

            if (word_len <= MAX_WORD_LEN) {
                char *word = (char *) malloc((word_len + 1) * sizeof(char));
                for (int i = 0; i < word_len; i++) {
                    word[i] = lowercase(text[i + left]);
                }
                word[word_len] = 0;

                if (not isStopword(word)) {
                    tokens.push_back({word, word_len});
                }
            }

            left = right + 1;
        }

        stemmedTokens.reserve(tokens.size());
        pthread_mutex_lock(&stemmer_mutex);
        for (auto &data : tokens) {
            stemmedTokens.push_back(stemming(data.first, data.second));
            free(data.first);
        }
        pthread_mutex_unlock(&stemmer_mutex);

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
};
