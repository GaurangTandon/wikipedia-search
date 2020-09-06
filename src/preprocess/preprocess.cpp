#include "preprocess.hpp"

// ignoring apostrophe for now, valid word is just a-z, $, _, 0-9

constexpr inline int FastTrie::char_index(char c) {
    return c - 'a';
}


FastTrie::FastTrie() {
    trans = {get_def()};
    isend = {""};
}

inline std::vector<int> FastTrie::get_def() {
    return std::vector<int>(N, -1);
}

inline int FastTrie::new_node() {
    int i = trans.size();
    trans.push_back(get_def());
    isend.push_back("");
    return i;
}

inline void FastTrie::start(char c) {
    currNode = FastTrie::root;
    next(c);
}

void FastTrie::insert(std::string &str, std::string val) {
    int curr = 0;

    for (auto c : str) {
        int i = char_index(c);
        int &next = trans[curr][i];

        if (next == -1) {
            next = new_node();
        }

        curr = next;
    }

    isend[curr] = val;
}

inline void FastTrie::next(char move) {
    if (currNode == -1) return;
    int i = char_index(move);
    if (i >= 0 and i < N) currNode = trans[currNode][i];
}

inline bool FastTrie::is_end_string() {
    return currNode != -1 and isend[currNode].size() > 0;
}

inline std::string FastTrie::getVal() {
    return (currNode == -1) ? "" : isend[currNode];
}

Preprocessor::Preprocessor() : stemmer_mutex(PTHREAD_MUTEX_INITIALIZER) {
    // const char** list = sb_stemmer_list();
    // there is porter2 in this

    stemmer = sb_stemmer_new("porter", nullptr);
    assert(stemmer != nullptr);

    trie = FastTrie();
    stemTrie = FastTrie();
    commonWord = (sb_symbol *) malloc(MAX_WORD_LEN * sizeof(sb_symbol));

    std::ifstream stopwords_file("preprocess/stopwords_plain.txt", std::ios_base::in);

    int count;
    stopwords_file >> count;

    assert(count < 200);
    // OPTIMIZATION: read character by character and inline the trie.insert thing
    //  instead of using stream/based file i/o
    // but as it is done only threadCount times, it is probably useless to optimize this
    while (count--) {
        std::string word;
        stopwords_file >> word;
        trie.insert(word);
    }

    stopwords_file.close();

    for (auto &word : mostFrequentStems) {
        stemTrie.insert(word, stemming(reinterpret_cast<const sb_symbol *>(word.c_str()), word.size()));
    }
}

Preprocessor::~Preprocessor() {
    sb_stemmer_delete(stemmer);
    free(commonWord);
}

inline constexpr bool Preprocessor::validChar(char c) {
    if (c >= 'a' and c <= 'z') return true;
    if (c >= 'A' and c <= 'Z') return true;
    if (c >= '0' and c <= '9') return true;
    if (c == '_') return true;
    if (c == '$') return true;

    return false;
}

inline constexpr char Preprocessor::lowercase(char c) {
    if (c >= 'A' and c <= 'Z') return (char) (c + 32);
    return c;
}

inline constexpr bool Preprocessor::isnum(char c) {
    return c <= '9' and c >= '0';
}

// MUST BE CALLED WITH LOCK HELD
inline std::string Preprocessor::stemming(const sb_symbol *word, const int len) const {
    const sb_symbol *res = sb_stemmer_stem(stemmer, word, len);
    int new_len = sb_stemmer_length(stemmer);

    std::string ret(reinterpret_cast<const char *>(res), new_len);

    return ret;
}

inline std::vector<std::string> Preprocessor::getStemmedTokens(const std::string &text, int start, int end) {
    // tokenize
    // stopwords removal
    // stemmer

    std::vector<std::string> stemmedTokens;

    pthread_mutex_lock(&stemmer_mutex);
    for (int left = start; left <= end; left++) {
        if (not validChar(text[left])) continue;

        trie.start(text[left]);
        stemTrie.start(text[left]);

        bool hasNumber = isnum(text[left]);
        int right = left;
        while (right < end and validChar(text[right + 1])) {
            right++;
            trie.next(text[right]);
            trie.next(text[right]);
            hasNumber = hasNumber or isnum(text[right]);
        }

        int word_len = right - left + 1;

        // >= 2 since a single letter word is 1. too vague 2. gets stemmed into an empty string by stemmer
        if (word_len <= MAX_WORD_LEN and not trie.is_end_string() and word_len >= MIN_WORD_LEN) {
            for (int i = 0; i < word_len; i++) {
                commonWord[i] = text[i + left];
            }

            auto stemmedVal = stemTrie.getVal();

            if (hasNumber) { // don't stem words containing numbers
                stemmedTokens.emplace_back(text.substr(left, word_len));
            } else if (stemmedVal.size() > 0) {
                stemmedTokens.emplace_back(stemmedVal);
            } else {
                const auto &str = stemming(commonWord, word_len);
                stemmedTokens.emplace_back(str);
            }
        }

        left = right + 1;
    }
    pthread_mutex_unlock(&stemmer_mutex);

    return stemmedTokens;
}

int
Preprocessor::processText(data_type &alldata, const int docid, const int zone, const std::string &text, int start,
                          int end) {
    auto stemmedTokens = getStemmedTokens(text, start, end);

    for (auto &term : stemmedTokens) {
        auto &post_list = alldata[term];
        if (post_list.empty() or post_list.back().first != docid) {
            post_list.push_back({docid, std::vector<int>(ZONE_COUNT)});
        }
        post_list.back().second[zone]++;
    }

    return stemmedTokens.size();
}

bool Preprocessor::fast_equals(const std::string &src, const std::string &target, int pos) {
    int j = 0, i = pos;

    while (i < src.size()) {
        if (src[i] != target[j]) return false;
        j++, i++;
        if (j == target.size()) return true;
    }

    return false;
}

bool Preprocessor::fast_equals(const std::string &src, const std::vector<std::string> &targets, int pos) {
    for (const auto &target: targets) {
        if (fast_equals(src, target, pos)) {
            return true;
        }
    }

    return false;
}
