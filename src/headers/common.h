#ifndef COMMON_HEADER
#define COMMON_HEADER

#include <map>
#include <unordered_map>
#include <vector>

// have text zone first, since most tokens occur in text only
enum {
    TEXT_ZONE, INFOBOX_ZONE, CATEGORY_ZONE, TITLE_ZONE, EXTERNAL_LINKS_ZONE, REFERENCES_ZONE, ZONE_COUNT
};

std::vector<std::string> reverseZonal = {"Body", "Infobox", "Category", "Title", "Links", "References"};
std::vector<std::string> zoneFirstLetter = {"b", "i", "c", "t", "l", "r"};
std::vector<double> zoneSearchWeights = {0.1, 1, 1, 10, 0.2, 0.2};

typedef std::map<std::string, std::vector<int>> local_data_type;
// data[term_name] = postings list; list[i] = { doc_id, { freq_information }};
typedef std::vector<std::pair<int, std::vector<int>>> postings_list_type;
typedef std::map<std::string, postings_list_type> data_type;

#define start_time clock_gettime(CLOCK_MONOTONIC, st);

#define calc_time(st, et) ((et->tv_sec - st->tv_sec) + 1e-9l * (et->tv_nsec - st->tv_nsec))

#define end_time \
    clock_gettime(CLOCK_MONOTONIC, et); \
    timer = calc_time(st, et);

constexpr int TERMS_PER_SPLIT_FILE = 100000;

// wouldn't recommend going below 25 because few popular words
// like internationalization are very long
// Full stats: http://norvig.com/mayzner.html
constexpr int MAX_WORD_LEN = 25;
constexpr int MIN_WORD_LEN = 3;

std::vector<std::string> mostFrequentStems = {"system", "film", "british", "york", "john", "english", "list", "small",
                                              "february", "november", "gov", "years", "use", "century", "september",
                                              "left", "center", "august", "many", "people", "war", "december",
                                              "october", "march", "state", "google", "national", "june", "january",
                                              "live", "april", "july", "city", "dead", "press", "one",
                                              "location", "text", "can", "time", "used", "issue",
                                              "states", "website", "thumb", "two", "university", "united",
                                              "math", "history", "style", "american", "world", "one", "may", "status",
                                              "html", "category", "books"};


#endif