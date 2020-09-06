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
#include <cmath>
#include "../preprocess/preprocess.cpp"
#include "../file_handling/zip_operations.cpp"

#define ceil(x, y) (((x) + (y) - 1) / (y))

typedef std::pair<double, int> score_type; // { score, page-id }
// least scoring element at the top so it can be popped
typedef std::priority_queue<score_type, std::vector<score_type>, std::greater<>> results_container;
int totalDocCount;
std::vector<std::string> milestoneWords;

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

inline int getIndex(const std::string &token) {
    return std::lower_bound(milestoneWords.begin(), milestoneWords.end(), token) - milestoneWords.begin();
}

// PRECONDITION: query is not empty
results_container performSearch(const std::string &query, int zone, int maxSize) {
    auto tokens = processor->getStemmedTokens(query, 0, query.size() - 1);
    int prevFile = -1;
    ReadBuffer *mainBuff, *idBuff, *zonalBuff;
    results_container results;

    for (const auto &token : tokens) {
        int fileNum = getIndex(token);

        if (prevFile != fileNum) {
            auto str = std::to_string(fileNum);
            mainBuff = new ReadBuffer(outputDir + "mimain" + str);
            idBuff = new ReadBuffer(outputDir + "miids" + str);
            zonalBuff = new ReadBuffer(outputDir + "mi" + zoneFirstLetter[zone] + str);
            prevFile = fileNum;
        }

        std::vector<score_type> thisTokenScores;
        while (true) {
            auto currToken = mainBuff->readString();
            int docCount = mainBuff->readInt();
            int actualDocCount = 0; // number of documents with this term in their zone
            const bool isCurrTokenReq = currToken == token;

            for (int f = 0; f < docCount; f++) {
                int docId = idBuff->readInt();
                int termFreqInDoc = zonalBuff->readInt();

                if (isCurrTokenReq) {
                    thisTokenScores.emplace_back(termFreqInDoc, docId);
                    actualDocCount += (termFreqInDoc > 0);
                }
            }

            if (isCurrTokenReq) {
                assert(actualDocCount > 0);
                double denom = log10((double) totalDocCount / actualDocCount);

                for (auto e : thisTokenScores) {
                    e.first *= denom;
                    results.push(e);

                    if (results.size() > maxSize) results.pop();
                }

                break;
            }
        }
    }

    zonalBuff->close();
    mainBuff->close();
    idBuff->close();
    delete zonalBuff;
    delete mainBuff;
    delete idBuff;
    return results;
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

    results_container results;

    std::map<int, double> docScores; // maximal size would be ZONE_COUNT * K

    // TODO: see if per zone threading is required here
    for (const auto &zone : zonalQueries) {
        if (not zone.empty()) {
            auto intermediateResults = performSearch(zone, zoneI, K);

            while (not intermediateResults.empty()) {
                auto res = intermediateResults.top();
                intermediateResults.pop();
                double newScore = res.first * zoneSearchWeights[zoneI];
                docScores[res.second] += newScore;
            }
        }

        zoneI++;
    }

    for (auto &e : docScores) {
        results.push({e.second, e.first});
        assert(not std::isnan(e.second));
        if (results.size() > K) results.pop();
    }

    // sorted from most score->least score
    std::vector<std::pair<int, std::string>> outputResults(K);
    std::vector<double> scores(K);
    std::vector<std::pair<int, int>> docIdSorted(K); // id, index

    if (results.size() < K) {
        // TODO: append random results
    }

    for (int i = K - 1; i >= 0; i--) {
        int docid = results.top().second;
        scores[i] = results.top().first;
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
        for (int id = 0; id < docCount; id++) {
            if (id < docIdSorted[currI].first) ignoreLine
            else {
                int idx = docIdSorted[currI].second;
                getline(docIDs, outputResults[idx].second);
                currI++;
                if (currI == K) break;
            }
        }
    }

    int i = 0;
    for (auto &result : outputResults) {
        outputFile << scores[i] << " ";
        outputFile << result.first << "," << result.second << std::endl;
        i++;
    }

    end_time

    outputFile << timer << ", " << (timer / K) << "\n\n";
}

int main(int argc, char *argv[]) {
    assert(argc == 3);

    processor = new Preprocessor();
    outputDir = std::string(argv[1]) + "/";

    std::ifstream statFile(outputDir + "stat.txt");
    statFile >> totalDocCount; // first read is actually file count
    statFile >> totalDocCount;
    int uniqueTokensCount;
    statFile >> uniqueTokensCount;

    std::ifstream milestonesFile(outputDir + "milestone.txt");
    int mileCount = ceil(uniqueTokensCount, TERMS_PER_SPLIT_FILE);
    milestoneWords.reserve(mileCount);
    for (int i = 0; i < mileCount; i++) {
        std::string str;
        milestonesFile >> str;
        milestoneWords.push_back(str);
    }

    char *queryFilePath = argv[2];

    std::ifstream queryFile(queryFilePath);
    std::ofstream outputFile("queries_op.txt");

    int queryCount;
    queryFile >> queryCount;

    for (int qI = 0; qI < queryCount; qI++) {
        readAndProcessQuery(queryFile, outputFile);
    }

    delete processor;

    delete st;
    delete et;

    return 0;
}
