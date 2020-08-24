#include<string>
#include<vector>
#include<set>
#include <cassert>
#include <iostream>
#include <fstream>
#include <pthread.h>
#include "../libstemmer_c/include/libstemmer.h"

constexpr int MAX_WORD_LEN = 20;

class Preprocessor {
public:
    sb_stemmer *stemmer = nullptr;
    std::set<std::string> stopwords;
    std::locale loc;
    pthread_mutex_t stemmer_mutex = PTHREAD_MUTEX_INITIALIZER;

    Preprocessor() {
        // const char** list = sb_stemmer_list();
        // there is porter2 in this

        loc = std::locale();
        stemmer = sb_stemmer_new("porter", nullptr);
        assert(stemmer != nullptr);
        std::ifstream stopwords_file("stopwords.txt");
        int count;
        stopwords_file >> count;
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

    inline bool isStopword(const char *word, int len) {
        std::string w(word);
        return stopwords.find(w) != stopwords.end();
    }

    inline std::string stemming(const char *word, const int len) {
        pthread_mutex_lock(&stemmer_mutex);

        const sb_symbol *res = sb_stemmer_stem(stemmer, reinterpret_cast<const sb_symbol *>(word), len);
        auto ans = reinterpret_cast<const char *>(res);
        auto ret = std::string(ans);

        pthread_mutex_unlock(&stemmer_mutex);

        return ret;
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

            if (word_len <= MAX_WORD_LEN) {
                char *word = (char *) malloc((word_len + 1) * sizeof(char));
                for (int i = 0; i < word_len; i++) {
                    word[i] = tolower(text[i + left], loc);
                }
                word[word_len] = 0;

                if (not isStopword(word, len)) {
//                std::cout << word << std::endl;
                    stemmedTokens.push_back(stemming(word, word_len));
                }

                free(word);
            }

            left = right + 1;
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
};
