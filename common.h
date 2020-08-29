#include <map>

// have text zone first, since most tokens occur in text only
enum {
    TEXT_ZONE, INFOBOX_ZONE, CATEGORY_ZONE, TITLE_ZONE, EXTERNAL_LINKS_ZONE, ZONE_COUNT
};

// data[term_id] = postings list; list[i] = { doc_id, { freq_information }};
typedef std::map<std::string, std::vector<std::pair<int, std::vector<int>>>> data_type;

#define start_time clock_gettime(CLOCK_MONOTONIC, st);

#define calc_time(st, et) ((et->tv_sec - st->tv_sec) + 1e-9l * (et->tv_nsec - st->tv_nsec))

#define end_time \
    clock_gettime(CLOCK_MONOTONIC, et); \
    timer = calc_time(st, et);
