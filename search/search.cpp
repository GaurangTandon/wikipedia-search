#include <cassert>
#include <string>
#include <vector>
#include <iostream>
#include "../common.h"

int main(int argc, char *argv[]) {
    assert(argc == 2);

    char* query = argv[1];

    std::vector<int> zones(255, -1);
    zones['t'] = TITLE_ZONE;
    zones['i'] = INFOBOX_ZONE;
    zones['c'] = CATEGORY_ZONE;

    constexpr char QUERY_SEP = ':';

    std::vector<std::string> zonalQueries(ZONE_COUNT);
    int currZone = TEXT_ZONE;

    while (*query){
        auto c = *query;

        if (zones[c] > -1 and *(query + 1) == QUERY_SEP) {
            currZone = zones[c];
            query += 2;
            continue;
        }

        zonalQueries[currZone] += *query;
        query++;
    }

    for (auto &string : zonalQueries) std::cout << string << std::endl;

    return 0;
}
