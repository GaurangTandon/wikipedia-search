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

    static std::vector<std::vector<int>> trans;
    static std::vector<bool> isend;
    int currNode;

    static constexpr int char_index(char c);

    FastTrie();

    static inline std::vector<int> get_def();
    static inline int new_node();
    static void insert(std::string &str, bool val = true);

    void start(char c);

    inline void next(char);

    inline bool is_end_string();
};

struct Preprocessor {
    sb_stemmer *stemmer;
    FastTrie trie;
    sb_symbol *commonWord;

    Preprocessor();

    ~Preprocessor();

    static inline void init();

    static inline constexpr char lowercase(char c);

    static inline constexpr bool isnum(char c);

    static inline constexpr bool validChar(char c);

    int processText(data_type &all_data, int docid, int zone, const std::string &text, int start, int end);

    inline static bool fast_equals(const std::string &src, const std::string &target, int pos);

    inline static bool fast_equals(const std::string &src, const std::vector<std::string> &targets, int pos);

    std::vector<std::string> getStemmedTokens(const std::string &text, int start, int end);

    inline std::string stemming(const sb_symbol *word, int len) const;
};
