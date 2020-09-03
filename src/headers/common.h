#include <map>
#include <vector>

// have text zone first, since most tokens occur in text only
enum {
    TEXT_ZONE, INFOBOX_ZONE, CATEGORY_ZONE, TITLE_ZONE, EXTERNAL_LINKS_ZONE, REFERENCES_ZONE, ZONE_COUNT
};

typedef std::map<std::string, std::vector<int>> local_data_type;

std::map<int, std::string> reverseZonal = {
        {TEXT_ZONE, "Body"},
        {INFOBOX_ZONE, "Infobox"},
        {CATEGORY_ZONE, "Category"},
        {TITLE_ZONE, "Title"},
        {EXTERNAL_LINKS_ZONE, "External links"},
        {REFERENCES_ZONE, "References"}
};

// data[term_name] = postings list; list[i] = { doc_id, { freq_information }};
typedef std::map<std::string, std::vector<std::pair<int, std::vector<int>>>> data_type;

#define start_time clock_gettime(CLOCK_MONOTONIC, st);

#define calc_time(st, et) ((et->tv_sec - st->tv_sec) + 1e-9l * (et->tv_nsec - st->tv_nsec))

#define end_time \
    clock_gettime(CLOCK_MONOTONIC, et); \
    timer = calc_time(st, et);
