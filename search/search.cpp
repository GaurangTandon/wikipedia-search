#include <cassert>
#include <string>
#include <vector>
#include "../preprocess/preprocess.cpp"

Preprocessor processor;
std::string outputDir;

std::vector<std::string> extractZonalQueries(char *query) {
    std::vector<int> zones(255, -1);
    zones['t'] = TITLE_ZONE;
    zones['i'] = INFOBOX_ZONE;
    zones['c'] = CATEGORY_ZONE;

    constexpr char QUERY_SEP = ':';

    std::vector<std::string> zonalQueries(ZONE_COUNT);
    int currZone = TEXT_ZONE;

    while (*query) {
        auto c = *query;

        if (zones[c] > -1 and *(query + 1) == QUERY_SEP) {
            currZone = zones[c];
            query += 2;
            continue;
        }

        zonalQueries[currZone] += *query;
        query++;
    }

    return zonalQueries;
}

std::vector<std::vector<std::string>> searchResults(ZONE_COUNT);

void performSearch(const std::string &query, int zone) {
    auto tokens = processor.getStemmedTokens(query, 0, query.size() - 1);

    for (auto &token : tokens) {

    }
}

int main(int argc, char *argv[]) {
    assert(argc == 3);
    processor = Preprocessor();
    outputDir = std::string(argv[1]);

    int fileCount;
    std::ifstream fileStats("file_stats.txt", std::ios_base::in);
    fileStats >> fileCount;
    assert(fileCount < 30);

    char *query = argv[2];
    auto zonalQueries = extractZonalQueries(query);
    int zoneI = 0;

    for (auto &zone : zonalQueries) {
        performSearch(zone, zoneI);
        zoneI++;
    }

    for (auto &result : searchResults) {
        for (auto &title : result) {
            std::cout << title << "; ";
        }
        std::cout << std::endl;
        std::cout << "------";
        std::cout << std::endl;
    }

    return 0;
}
