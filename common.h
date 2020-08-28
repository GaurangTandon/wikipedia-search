#include <map>
#include <vector>
#include <string>

// have text zone first, since most tokens occur in text only
enum {
    TEXT_ZONE, INFOBOX_ZONE, CATEGORY_ZONE, ZONE_COUNT
};

class WikiPage {
public:
    std::string title, text;
    int docid;
    // each part is governed by zonal markers
    std::vector<std::vector<std::string>> terms;

    WikiPage(xml::parser &p);
};

// TODO: can probably replace the indexer in map with string
// directly as in any case we are doing a string access later in get_termid

// data[term_id] = postings list; list[i] = { doc_id, { freq_information }};
typedef std::map<std::string, std::vector<std::pair<int, std::vector<int>>>> data_type;

typedef struct memory_type {
    WikiPage **store = nullptr;
    int size = 0;

    data_type alldata;
} memory_type;

