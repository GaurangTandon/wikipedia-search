#include <map>
#include <vector>
#include <string>

class WikiPage {
public:
    std::string title, text;
    int docid;

    WikiPage(xml::parser &p);
};

typedef struct memory_type {
    WikiPage **store = nullptr;
    int size = 0;
    int checkpoint_num;

    data_type* alldata;
} memory_type;

