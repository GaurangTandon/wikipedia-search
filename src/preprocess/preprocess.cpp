#include "preprocess.hpp"

// ignoring apostrophe for now, valid word is just a-z, $, _, 0-9

// wouldn't recommend going below 25 because few popular words
// like internationalization are very long
// Full stats: http://norvig.com/mayzner.html
constexpr int MAX_WORD_LEN = 25;

constexpr int FastTrie::char_index(char c) {
    return c - 'a';
}


FastTrie::FastTrie() {
    trans = {get_def()};
    isend = {false};
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

void FastTrie::insert(std::string &str) {
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

inline int FastTrie::next(int curr, char move) {
    if (curr == -1) return curr;
    int i = char_index(move);
    if (i >= 0 and i < N) return trans[curr][i];
    else return -1;
}

inline bool FastTrie::is_end_string(int node) {
    return node != -1 and isend[node];
}

Preprocessor::Preprocessor() : stemmer_mutex(PTHREAD_MUTEX_INITIALIZER) {
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

Preprocessor::~Preprocessor() {
    sb_stemmer_delete(stemmer);
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

// MUST BE CALLED WITH LOCK HELD
inline std::string Preprocessor::stemming(const sb_symbol *word, const int len) const {
    const sb_symbol *res = sb_stemmer_stem(stemmer, word, len);
    int new_len = sb_stemmer_length(stemmer);

    std::string ret;
    ret.reserve(new_len);
    for (int i = 0; i < new_len; i++) {
        ret += res[i];
    }

    return ret;
}

std::vector<std::string> Preprocessor::getStemmedTokens(const std::string &text, int start, int end) {
    // tokenize
    // stopwords removal
    // stemmer

    std::vector<std::pair<sb_symbol *, int>> tokens;
    std::vector<std::string> stemmedTokens;

    for (int left = start; left <= end; left++) {
        if (not validChar(text[left])) continue;

        int curr = trie.next(FastTrie::root, text[left]);
        int right = left;
        while (right < end and validChar(text[right + 1])) {
            right++;
            curr = trie.next(curr, text[right]);
        }

        int word_len = right - left + 1;

        if (word_len <= MAX_WORD_LEN and not trie.is_end_string(curr)) {
            auto *word = (sb_symbol *) malloc(word_len * sizeof(sb_symbol));
            for (int i = 0; i < word_len; i++) {
                word[i] = text[i + left];
            }

            tokens.emplace_back(word, word_len);
        }

        left = right + 1;
    }

    stemmedTokens.reserve(tokens.size());
    pthread_mutex_lock(&stemmer_mutex);
    for (const auto &data : tokens) {
        const auto &str = stemming(data.first, data.second);
        stemmedTokens.push_back(str);
        free(data.first);
    }
    pthread_mutex_unlock(&stemmer_mutex);

    return stemmedTokens;
}

int
Preprocessor::processText(local_data_type &localData, const int zone, const std::string &text, int start, int end) {
    auto stemmedTokens = getStemmedTokens(text, start, end);

    for (auto &term : stemmedTokens) {
        auto &freq = localData[term];
        if (freq.empty()) freq = std::vector<int>(ZONE_COUNT);
        freq[zone]++;
    }

    return stemmedTokens.size();
}

void Preprocessor::dumpText(data_type *alldata, const int docid, const local_data_type &localData) {
    auto &alldata_act = *alldata;
    for (const auto &ldata : localData) {
        alldata_act[ldata.first].push_back({docid, ldata.second});
    }
}


// reduced time from 2.2s to 0.8s (compared to src.substr(pos, target.size()) == target)
// required: both src and target should be in lowercase, no lowercasing is done here
bool Preprocessor::fast_equals(const std::string &src, const std::string &target, int pos) {
    int j = 0, i = pos;

    assert(not target.empty());

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
