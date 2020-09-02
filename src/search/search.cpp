#include <cassert>
#include <string>
#include <vector>
#include <set>
#include <limits>
#include <pthread.h>
#include <ctime>
#include "../preprocess/preprocess.cpp"
#include "../file_handling/zip_operations.cpp"

constexpr int BLOCK_SIZE = 1;
#define ceil(x, y) (x + y - 1) / y

Preprocessor *processor;
std::string outputDir;
std::map<std::string, int> termIDMap;
std::map<int, std::string> docIdMap;
int fileCount;

std::vector<std::string> extractZonalQueries(char *query) {
    std::vector<int> zones(255, -1);
    zones['t'] = TITLE_ZONE;
    zones['i'] = INFOBOX_ZONE;
    zones['b'] = TEXT_ZONE;
    zones['c'] = CATEGORY_ZONE;
    zones['r'] = REFERENCES_ZONE;
    zones['l'] = EXTERNAL_LINKS_ZONE;

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


    std::cout << "Extracted queries" << '\n';
    for (int i = 0; i < ZONE_COUNT; i++) {
        std::cout << "Zone " << reverseZonal[i] << ": " << zonalQueries[i] << std::endl;
    }
    std::cout << "----\n";

    return zonalQueries;
}

std::vector<std::vector<std::string>> searchResults(ZONE_COUNT);

typedef std::vector<std::vector<int>> shared_mem_type;
typedef struct query_type {
    int zone;
    int block;
    const std::set<int> *tokens;
    shared_mem_type *sharedMem;
} query_type;

void *searchFileThreaded(void *arg) {
    const auto *query = (query_type *) arg;
    if (query->tokens->empty()) {
        return nullptr;
    }
    const auto token_begin = query->tokens->begin();
    const auto token_end = query->tokens->end();
    std::set<int> docids;

#define min(x, y) ((x) < (y) ? (x) : (y))

    for (int currFile = query->block * BLOCK_SIZE, lim = min(fileCount, currFile + BLOCK_SIZE);
         currFile < lim; currFile++) {
        std::string filepath = outputDir + "index" + std::to_string(currFile);
        auto buffer = ReadBuffer(filepath);

        auto tokenIT = token_begin;
        auto currTokenId = *tokenIT;

        int count = buffer.readInt('\n');

        for (int i = 0; i < count; i++) {
            int id = buffer.readInt();

            if (id != currTokenId) {
                if (i < count - 1) buffer.ignoreTillDelim();
                continue;
            }

            int docCount = buffer.readInt();
            for (int j = 0; j < docCount; j++) {
                int docid = buffer.readInt();

                std::vector<int> freq(ZONE_COUNT, 0);
                int k = 0;

                while (true) {
                    auto res = buffer.readInt(';', ',');
                    freq[k++] = res.first;
                    if (res.second == ';') break;
                }

                if (freq[query->zone]) docids.insert(docid);
            }

            assert(buffer.readChar() == '\n');

            tokenIT++;
            if (tokenIT == token_end) break;
            currTokenId = *tokenIT;
        }

        buffer.close();
    }

    auto &vec = (*(query->sharedMem))[query->block];
    vec.insert(vec.end(), docids.begin(), docids.end());


    return nullptr;
}

std::set<int> performSearch(const std::string &query, int zone) {
    int threadCount = ceil(fileCount, BLOCK_SIZE);
    auto shared_data = new shared_mem_type(threadCount);

    auto tokens = processor->getStemmedTokens(query, 0, query.size() - 1);

    auto tokenIDS = new std::set<int>();
    for (auto &token : tokens) {
        auto id = termIDMap[token];
        tokenIDS->insert(id);
    }

    std::vector<pthread_t> threads(threadCount);
    std::vector<query_type *> query_data_vec(threadCount);

    for (int i = 0; i < threadCount; i++) {
        auto queryData = new query_type();
        queryData->tokens = tokenIDS;
        queryData->sharedMem = shared_data;
        queryData->zone = zone;
        queryData->block = i;
        query_data_vec[i] = queryData;
        pthread_create(&threads[i], nullptr, searchFileThreaded, queryData);
    }

    for (int i = 0; i < threadCount; i++) {
        pthread_join(threads[i], nullptr);
        delete query_data_vec[i];
    }

    std::set<int> docIds;
    const auto shared_data_deref = *shared_data;
    for (const auto &data : shared_data_deref) {
        if (data.empty()) continue;
        docIds.insert(data.begin(), data.end());
    }

    delete tokenIDS;
    delete shared_data;

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
    std::ifstream docIDs(outputDir + "docs", std::ios_base::in);
    int docCount;
    docIDs >> docCount;
    docIDs.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    for (int id = 1; id <= docCount; id++) {
        getline(docIDs, docIdMap[id]);
    }
}

int main(int argc, char *argv[]) {
    assert(argc == 3);

    auto st = new timespec(), et = new timespec();
    long double timer;
    start_time

    processor = new Preprocessor();
    outputDir = std::string(argv[1]) + "/";

    std::ifstream fileStats(outputDir + "file_stat.txt", std::ios_base::in);
    fileStats >> fileCount;
    assert(fileCount > 0 and fileCount < 100);

    readTermIds();
    readDocIds();

    char *query = argv[2];
    auto zonalQueries = extractZonalQueries(query);
    int zoneI = 0;

    for (const auto &zone : zonalQueries) {
        const auto docIds = performSearch(zone, zoneI);
        for (const auto id : docIds) {
            const auto &str = docIdMap[id];
            assert(not str.empty());
            searchResults[zoneI].push_back(str);
        }
        zoneI++;
    }

    zoneI = 0;
    for (auto &result : searchResults) {
        std::cout << "Zone " << reverseZonal[zoneI] << std::endl;
        for (auto &title : result) {
            std::cout << title << "; ";
        }
        std::cout << std::endl;
        std::cout << "Total results: " << result.size() << std::endl;
        std::cout << "------";
        std::cout << std::endl;
        zoneI++;
    }

    delete processor;
    end_time

    std::cout << "Search finished in time " << timer << std::endl;

    return 0;
}
