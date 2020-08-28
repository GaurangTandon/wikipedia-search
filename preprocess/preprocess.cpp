#include<string>
#include<vector>
#include<set>
#include <cassert>
#include <iostream>
#include <fstream>
#include <pthread.h>
#include "../libstemmer_c/include/libstemmer.h"
#include "../common.h"

// ignoring apostrophe for now, valid word is just a-z, $, _, 0-9

// wouldn't recommend going below 25 because few popular words
// like internationalization are very long
// Full stats: http://norvig.com/mayzner.html
constexpr int MAX_WORD_LEN = 25;

struct FastTrie {
    constexpr int char_index(char c) {
        return c - 'a';
    }

    static constexpr int N = 26;
    static constexpr int root = 0;

    std::vector<std::vector<int>> trans;
    std::vector<bool> isend;

    FastTrie() {
        trans = {get_def()};
        isend = {false};
    }

    static inline std::vector<int> get_def() {
        return std::vector<int>(N, -1);
    }

    inline int new_node() {
        int i = trans.size();
        trans.push_back(get_def());
        isend.push_back(false);
        return i;
    }

    void insert(std::string &str) {
        int curr = 0;

        for (auto c : str) {
            int i = char_index(c);
            int &next = trans[curr][i];

            if (next == -1) {
                next = new_node();
            }

            curr = next;
        }

        isend[curr] = true;
    }

    inline int next(int curr, char move) {
        if (curr == -1) return curr;
        int i = char_index(move);
        if (i >= 0 and i < N) return trans[curr][i];
        else return -1;
    }

    inline bool is_end_string(int node) {
        return node != -1 and isend[node];
    }
};

class Preprocessor {
public:
    sb_stemmer *stemmer = nullptr;
    std::set<std::string> stopwords;
    pthread_mutex_t stemmer_mutex = PTHREAD_MUTEX_INITIALIZER;
    FastTrie trie;

    Preprocessor() {
        // const char** list = sb_stemmer_list();
        // there is porter2 in this

        stemmer = sb_stemmer_new("porter", nullptr);
        assert(stemmer != nullptr);

        trie = FastTrie();

        std::ifstream stopwords_file("preprocess/stopwords_plain.txt", std::ios_base::in);

        int count;
        stopwords_file >> count;

        assert(count < 200);
        while (count--) {
            std::string word;
            stopwords_file >> word;
            trie.insert(word);
        }

        stopwords_file.close();
    }

    ~Preprocessor() {
        sb_stemmer_delete(stemmer);
    }

// O3 will optimize the for loops out
// https://godbolt.org/z/bT9398
    inline constexpr bool validChar(char c) {
        if (c >= 'a' and c <= 'z') return true;
        if (c >= 'A' and c <= 'Z') return true;
        if (c >= '0' and c <= '9') return true;
        if (c == '_') return true;
        if (c == '$') return true;

        return false;
    }

    inline constexpr char lowercase(char c) {
        if (c >= 'A' and c <= 'Z') return c + 32;
        return c;
    }

    // MUST BE CALLED WITH LOCK HELD
    inline std::string stemming(const char *word, const int len) {
        const sb_symbol *res = sb_stemmer_stem(stemmer, reinterpret_cast<const sb_symbol *>(word), len);
        auto ans = reinterpret_cast<const char *>(res);
        auto ret = std::string(ans);

        return ret;
    }

    void processText(memory_type *mem, const WikiPage *page, const int zone, const std::string &text) {
        // tokenize
        // stopwords removal
        // stemmer

        int len = text.size();
        std::vector<std::pair<char *, int>> tokens;
        std::vector<std::string> stemmedTokens;

        for (int left = 0; left < len; left++) {
            if (not validChar(text[left])) continue;

            int curr = trie.next(trie.root, lowercase(text[left]));
            int right = left;
            while (right < len - 1 and validChar(text[right + 1])) {
                right++;
                curr = trie.next(curr, lowercase(text[right]));
            }

            int word_len = right - left + 1;

            if (word_len <= MAX_WORD_LEN) {
                if (not trie.is_end_string(curr)) {
                    char *word = (char *) malloc((word_len + 1) * sizeof(char));
                    for (int i = 0; i < word_len; i++) {
                        word[i] = lowercase(text[i + left]);
                    }
                    word[word_len] = 0;

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

        std::map<std::string, std::vector<int>> local;
        for (auto &term : stemmedTokens) {
            auto &freq = local[term];
            if (freq.empty()) freq = std::vector<int>(ZONE_COUNT);
            freq[zone]++;
        }

        for (auto &ldata : local) {
            mem->alldata[ldata.first].push_back({page->docid, ldata.second});
        }
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
