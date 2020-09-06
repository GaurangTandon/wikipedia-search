#include <cassert>
#include <string>
#include <vector>
#include <set>
#include <limits>
#include <pthread.h>
#include <ctime>
#include <algorithm>
#include <cstring>
#include <queue>
#include "../preprocess/preprocess.cpp"
#include "../file_handling/zip_operations.cpp"

constexpr int BLOCK_SIZE = 1;
#define ceil(x, y) (x + y - 1) / y
typedef std::pair<double, int> score_type; // { score, page-id }

Preprocessor *processor;
std::string outputDir;

std::vector<std::string> extractZonalQueries(const std::string &query) {
    std::vector<int> zones(255, -1);
    zones['t'] = TITLE_ZONE;
    zones['i'] = INFOBOX_ZONE;
    zones['b'] = TEXT_ZONE;
    zones['c'] = CATEGORY_ZONE;
    zones['r'] = REFERENCES_ZONE;
    zones['l'] = EXTERNAL_LINKS_ZONE;

    constexpr char QUERY_SEP = ':';

    std::vector<std::string> zonalQueries(ZONE_COUNT);
    int currZone = -1;

    for (int i = 0; i < query.size(); i++) {
        auto c = query[i];

        if (zones[c] > -1 and i < query.size() - 1 and query[i + 1] == QUERY_SEP) {
            currZone = zones[c];
            i++;
            continue;
        }

        if (currZone != -1) zonalQueries[currZone] += c;
        else for (int zoneI = 0; zoneI < ZONE_COUNT; zoneI++) zonalQueries[zoneI] += c;
    }

    return zonalQueries;
}

// PRECONDITION: tokens is not empty
std::set<int> *getTokenIDs(std::vector<std::string> &tokens) {
    auto idSet = new std::set<int>();
    std::sort(tokens.begin(), tokens.end());
    int currToken = 0;

    std::ifstream termIDsFile(outputDir + "terms", std::ios_base::in);

    int termCount;
    termIDsFile >> termCount;

    for (int i = 0; i < termCount; i++) {
        std::string token;
        termIDsFile >> token;
        int id;
        termIDsFile >> id;

        int res = std::strcmp(token.c_str(), tokens[currToken].c_str());
        if (res >= 0) {
            if (res == 0) {
                idSet->insert(id);
            }
            currToken++;
            if (currToken == tokens.size()) break;
        }
    }

    return idSet;
}

// PRECONDITION: query is not empty
std::vector<score_type> performSearch(const std::string &query, int zone) {
    auto tokens = processor->getStemmedTokens(query, 0, query.size() - 1);
    auto tokenIDS = getTokenIDs(tokens);


}

auto st = new timespec(), et = new timespec();
long double timer;

void readAndProcessQuery(std::ifstream &inputFile, std::ofstream &outputFile) {
    start_time
    int K;
    inputFile >> K;
    inputFile.ignore(std::numeric_limits<std::streamsize>::max(), ' '); // ignore ', '
    std::string query;
    getline(inputFile, query);
    auto zonalQueries = extractZonalQueries(query);
    int zoneI = 0;

    // least scoring element at the top so it can be popped
    std::priority_queue<score_type, std::vector<score_type>, std::greater<>> results;

    // TODO: see if per zone threading is required here
    for (const auto &zone : zonalQueries) {
        if (not zone.empty()) {
            const auto intermediateresults = performSearch(zone, zoneI);

            for (auto &res : intermediateresults) results.push(res);

            while (results.size() > K) results.pop();
        }

        zoneI++;
    }

    // sorted from most score->least score
    std::vector<std::pair<int, std::string>> outputResults;
    std::vector<std::pair<int, int>> docIdSorted(K); // id, index

    if (results.size() < K) {
        // TODO: append random results
    }

    for (int i = K - 1; i >= 0; i--) {
        int docid = results.top().second;
        outputResults[i].first = docid;
        docIdSorted[i] = {docid, i};
        results.pop();
    }

    sort(docIdSorted.begin(), docIdSorted.end());

    // Get string representation of title, and print it
    {
        std::ifstream docIDs(outputDir + "docs", std::ios_base::in);
        int docCount;
        docIDs >> docCount;

#define ignoreLine docIDs.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        ignoreLine

        // TODO: can optimize using lseek, see if necessary
        int currI = 0;
        for (int id = 1; id <= docCount; id++) {
            if (id < docIdSorted[currI].first) ignoreLine
            else {
                int idx = docIdSorted[currI].second;
                getline(docIDs, outputResults[idx].second);
                currI++;
                if (currI == K) break;
            }
        }
    }

    for (auto &result : outputResults) {
        std::cout << result.first << "," << result.second << std::endl;
    }

    end_time

    outputFile << timer << ", " << (timer / K) << "\n\n";
}

int main(int argc, char *argv[]) {
    assert(argc == 3);

    processor = new Preprocessor();
    outputDir = std::string(argv[1]) + "/";

    readDocIds();

    char *queryFilePath = argv[2];

    std::ifstream queryFile(queryFilePath);
    std::ofstream outputFile("queries_op.txt");

    int queryCount;
    queryFile >> queryCount;

    for (int qI = 0; qI < queryCount; qI++) {
        readAndProcessQuery(queryFile, outputFile);
    }

    delete processor;

    std::cout << "Search finished in time " << timer << std::endl;
    delete st;
    delete et;

    return 0;
}
