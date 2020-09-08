#include "preprocess.hpp"

// ignoring apostrophe for now, valid word is just a-z, $, _, 0-9

std::vector<std::vector<int>> FastTrie::trans = {FastTrie::get_def()};
std::vector<bool> FastTrie::isend = {false};

constexpr inline int FastTrie::char_index(char c) {
    return c - 'a';
}

FastTrie::FastTrie() {
    currNode = 0;
}

inline std::vector<int> FastTrie::get_def() {
    return std::vector<int>(N, -1);
}

inline int FastTrie::new_node() {
    int i = trans.size();
    trans.push_back(get_def());
    isend.push_back(false);
    return i;
}

inline void FastTrie::start(char c) {
    currNode = FastTrie::root;
    next(c);
}

void FastTrie::insert(std::string &str, bool val) {
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
    return currNode != -1 and isend[currNode];
}

Preprocessor::Preprocessor() {
    // const char** list = sb_stemmer_list();
    // there is porter2 in this

    stemmer = sb_stemmer_new("porter", nullptr);
    assert(stemmer != nullptr);

    trie = FastTrie();
    commonWord = (sb_symbol *) malloc(MAX_WORD_LEN * sizeof(sb_symbol));
}


Preprocessor::~Preprocessor() {
    sb_stemmer_delete(stemmer);
    free(commonWord);
}

inline void Preprocessor::init() {
    std::ifstream stopwords_file("preprocess/stopwords_improved.txt", std::ios_base::in);
    if (!stopwords_file) exit(11);

    int count;
    stopwords_file >> count;
    if (count <= 0 or count > 1000) exit(11);

    while (count--) {
        std::string word;
        stopwords_file >> word;
        if (!stopwords_file) exit(11);
        FastTrie::insert(word);
    }

    stopwords_file.close();
    if (!stopwords_file) exit(11);
}

inline constexpr bool Preprocessor::validChar(char c) {
    if (c >= 'a' and c <= 'z') return true;
    if (c >= 'A' and c <= 'Z') return true;
    if (c >= '0' and c <= '9') return true;

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

    for (int left = start; left <= end; left++) {
        if (not validChar(text[left])) continue;

        trie.start(text[left]);

        bool startsNum = isnum(text[left]);
        bool hasNumber = startsNum;
        bool hasAlpha = not startsNum;
        int right = left;

        while (right < end and validChar(text[right + 1])) {
            right++;
            trie.next(text[right]);
            bool numHaiKya = isnum(text[right]);
            hasNumber = hasNumber or numHaiKya;
            hasAlpha = hasAlpha or (not numHaiKya);
        }

        int word_len = right - left + 1;
        bool dontProcess =
                word_len > MAX_WORD_LEN or trie.is_end_string() or word_len < MIN_WORD_LEN or (startsNum and hasAlpha);

        // >= 2 since a single letter word is 1. too vague 2. gets stemmed into an empty string by stemmer
        if (not dontProcess) {
            for (int i = 0; i < word_len; i++) {
                commonWord[i] = text[i + left];
            }

            if (hasNumber) { // don't stem words containing numbers
                stemmedTokens.emplace_back(text.substr(left, word_len));
            } else {
                const auto &str = stemming(commonWord, word_len);
                stemmedTokens.emplace_back(str);
            }
        }

        left = right + 1;
    }

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

inline bool Preprocessor::fast_equals(const std::string &src, const std::string &target, int pos) {
    int j = 0, i = pos;

    while (i < src.size()) {
        if (src[i] != target[j]) return false;
        j++, i++;
        if (j == target.size()) return true;
    }

    return false;
}

inline bool Preprocessor::fast_equals(const std::string &src, const std::vector<std::string> &targets, int pos) {
    for (const auto &target: targets) {
        if (fast_equals(src, target, pos)) {
            return true;
        }
    }

    return false;
}
