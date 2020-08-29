#include <cassert>
#include <string>
#include <vector>
#include <set>
#include <limits>
#include <pthread.h>
#include "../preprocess/preprocess.cpp"

Preprocessor processor;
std::string outputDir;
std::map<std::string, int> termIDMap;
std::map<int, std::string> docIdMap;

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
int fileCount;

typedef std::vector<std::vector<int>> shared_mem_type;
typedef struct query_type {
    int zone;
    int number;
    std::set<int> *tokens;
    shared_mem_type *sharedMem;
} query_type;

void *searchFileThreaded(void *arg) {
    const auto *query = (query_type *) arg;
    const auto &ids = *(query->tokens);

    assert(not ids.empty());
    auto it = ids.begin();

    std::ifstream file(outputDir + "index" + std::to_string(query->number), std::ios_base::in);
    std::set<int> docids;

    int count;
    file >> count;
    for (int i = 0; i < count; i++) {
        int id; file >> id;

        if (id < *it) {
            file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }

        int docCount; file >> docCount;
        for (int j = 0; j < docCount; j++) {
            int docid;
            file >> docid;
            std::vector<int> freq(ZONE_COUNT, 0);
            int k = 0;

            while (true) {
                file >> freq[k++];
                char c; file >> c;
                if (c == ';') break;
            }

            if (freq[query->zone]) docids.insert(docid);
        }

        file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        it++;
        if (it == ids.end()) break;
    }

    auto &vec = (*(query->sharedMem))[query->number];
    vec.insert(vec.end(), docids.begin(), docids.end());

    return nullptr;
}

std::set<int> performSearch(const std::string &query, int zone) {
    auto shared_data = new shared_mem_type(fileCount);

    auto tokens = processor.getStemmedTokens(query, 0, query.size() - 1);
    std::set<int> tokenIDS;
    for (auto &token : tokens) {
        tokenIDS.insert(termIDMap[token]);
    }

    pthread_t threads[fileCount];

    for (int i = 0; i < fileCount; i++) {
        query_type queryData;
        queryData.tokens = &tokenIDS;
        queryData.sharedMem = shared_data;
        queryData.zone = zone;
        queryData.number = i;
        pthread_create(&threads[i], nullptr, searchFileThreaded, &queryData);
    }

    std::set<int> docIds;
    for (int i = 0; i < fileCount; i++) {
        pthread_join(threads[i], nullptr);
    }

    const auto shared_data_deref = *shared_data;
    for (const auto &data : shared_data_deref) {
        if (data.empty()) continue;
        docIds.insert(data.begin(), data.end());
    }

    return docIds;
}

void readTermIds() {
    std::ifstream termIDs(outputDir + "terms", std::ios_base::in);
    int termCount;
    termIDs >> termCount;
    for (int i = 0; i < termCount; i++) {
        std::string token;
        termIDs >> token;
        termIDs >> termIDMap[token];
    }
}

void readDocIds() {
    std::ifstream termIDs(outputDir + "docs", std::ios_base::in);
    int termCount;
    termIDs >> termCount;
    for (int i = 0; i < termCount; i++) {
        int id;
        termIDs >> id;
        getline(termIDs, docIdMap[id]);
    }
}

int main(int argc, char *argv[]) {
    assert(argc == 3);
    processor = Preprocessor();
    outputDir = std::string(argv[1]) + "/";

    std::ifstream fileStats(outputDir + "file_stats.txt", std::ios_base::in);
    fileStats >> fileCount;
    assert(fileCount < 30);

    readTermIds();
    readDocIds();

    char *query = argv[2];
    auto zonalQueries = extractZonalQueries(query);
    int zoneI = 0;

    for (const auto &zone : zonalQueries) {
        const auto docIds = performSearch(zone, zoneI);
        for (const auto id : docIds) {
            searchResults[zoneI].push_back(docIdMap[id]);
        }
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
