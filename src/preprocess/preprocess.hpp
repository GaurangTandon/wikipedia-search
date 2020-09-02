#include<string>
#include<vector>
#include <cassert>
#include <iostream>
#include <fstream>
#include <pthread.h>
#include "../libstemmer_c/include/libstemmer.h"
#include "../headers/common.h"

struct FastTrie {
    static constexpr int N = 26;
    static constexpr int root = 0;

    std::vector<std::vector<int>> trans;
    std::vector<bool> isend;

    static constexpr int char_index(char c);

    FastTrie();

    static inline std::vector<int> get_def();

    inline int new_node();

    void insert(std::string &str);

    inline int next(int, char);

    inline bool is_end_string(int);
};

struct Preprocessor {
    sb_stemmer *stemmer;
    pthread_mutex_t stemmer_mutex;
    FastTrie trie;
    sb_symbol *commonWord;
    std::map<std::string, int> freq;

    Preprocessor();

    ~Preprocessor();

    static inline constexpr char lowercase(char c);

    static inline constexpr bool validChar(char c);

    int processText(data_type &all_data, int docid, int zone, const std::string &text, int start, int end);

    static bool fast_equals(const std::string &src, const std::string &target, int pos);
    static bool fast_equals(const std::string &src, const std::vector<std::string> &targets, int pos);

    std::vector<std::string> getStemmedTokens(const std::string &text, int start, int end);

    inline std::string stemming(const sb_symbol *word, int len) const;
};
