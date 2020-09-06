#include <cassert>
#include <string>
#include <vector>
#include <set>
#include <limits>
#include <pthread.h>
#include <ctime>
#include <algorithm>
#include <queue>
#include <cmath>
#include "../preprocess/preprocess.cpp"
#include "../file_handling/zip_operations.cpp"

#define ceil(x, y) (((x) + (y) - 1) / (y))

typedef std::pair<double, int> score_type; // { score, page-id }
// least scoring element at the top so it can be popped
typedef std::priority_queue<score_type, std::vector<score_type>, std::greater<>> results_container;
typedef struct thread_data {
    results_container *results;
    int zone;
} thread_data;
std::vector<int> zonePrefixMarkers(255, -1);

int totalDocCount;
int uniqueTokensCount;
std::vector<std::string> milestoneWords;

Preprocessor *processor;
std::string outputDir;
std::vector<std::string> zonalQueries(ZONE_COUNT);
int K;

void extractZonalQueries(const std::string &query) {
    for (auto &x : zonalQueries) x.clear();

    constexpr char QUERY_SEP = ':';

    int currZone = -1;

    for (int i = 0; i < query.size(); i++) {
        auto c = query[i];

        if (zonePrefixMarkers[c] > -1 and i < query.size() - 1 and query[i + 1] == QUERY_SEP) {
            currZone = zonePrefixMarkers[c];
            i++;
            continue;
        }

        if (currZone != -1) zonalQueries[currZone] += c;
        else for (int zoneI = 0; zoneI < ZONE_COUNT; zoneI++) zonalQueries[zoneI] += c;
    }
}

inline int getIndex(const std::string &token) {
    auto lowIt = std::upper_bound(milestoneWords.begin(), milestoneWords.end(), token) - 1;
    return lowIt - milestoneWords.begin();
}

// PRECONDITION: query is not empty
void *performSearch(void *dataP) {
    auto &data = *((thread_data *) dataP);

    const auto &query = zonalQueries[data.zone];
    auto tokens = processor->getStemmedTokens(query, 0, query.size() - 1);
    int prevFile = -1;
    ReadBuffer *mainBuff, *idBuff, *zonalBuff;
    data.results = new results_container();
    int readCount;
    int readLim;

    for (const auto &token : tokens) {
        int fileNum = getIndex(token);

        if (prevFile != fileNum) {
            auto str = std::to_string(fileNum);
            mainBuff = new ReadBuffer(outputDir + "mimain" + str);
            idBuff = new ReadBuffer(outputDir + "miids" + str);
            zonalBuff = new ReadBuffer(outputDir + "mi" + zoneFirstLetter[data.zone] + str);
            prevFile = fileNum;

            if (fileNum == milestoneWords.size() - 1) {
                readLim = uniqueTokensCount % TERMS_PER_SPLIT_FILE;
            } else readLim = TERMS_PER_SPLIT_FILE;

            readCount = 0;
        }

        std::vector<score_type> thisTokenScores;
        while (readCount < readLim) {
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

            if (isCurrTokenReq and actualDocCount > 0) {
                double denom = log10((double) totalDocCount / actualDocCount);

                for (auto e : thisTokenScores) {
                    e.first *= denom;
                    data.results->push(e);

                    if (data.results->size() > K) data.results->pop();
                }

                break;
            }

            readCount++;
        }
    }

    zonalBuff->close();
    mainBuff->close();
    idBuff->close();
    delete zonalBuff;
    delete mainBuff;
    delete idBuff;
    return nullptr;
}

auto st = new timespec(), et = new timespec();
long double timer;

void readAndProcessQuery(std::ifstream &inputFile, std::ofstream &outputFile) {
    start_time
    inputFile >> K;
    inputFile.ignore(std::numeric_limits<std::streamsize>::max(), ' '); // ignore ', '
    std::string query;
    getline(inputFile, query);
    extractZonalQueries(query);

    results_container results;

    std::map<int, double> docScores; // maximal size would be ZONE_COUNT * K

    int zoneI = 0;
    pthread_t threads[ZONE_COUNT];
    thread_data *threads_data[ZONE_COUNT];

    for (const auto &zone : zonalQueries) {
        if (not zone.empty()) {
            auto data = (thread_data *) malloc(sizeof(thread_data));
            data->zone = zoneI;
            threads_data[zoneI] = data;
            pthread_create(&threads[zoneI], nullptr, performSearch, (void *) data);
        }

        zoneI++;
    }

    zoneI = 0;
    for (const auto &zone : zonalQueries) {
        if (not zone.empty()) {
            pthread_join(threads[zoneI], nullptr);
            auto &data = *threads_data[zoneI];
            auto &intermediateResults = data.results;

            while (not intermediateResults->empty()) {
                auto res = intermediateResults->top();
                intermediateResults->pop();
                double newScore = res.first * zoneSearchWeights[zoneI];
                docScores[res.second] += newScore;
            }

            delete intermediateResults;
            free(threads_data[zoneI]);
        }

        zoneI++;
    }

    for (auto &e : docScores) {
        results.push({e.second, e.first});
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
        std::ifstream docTitleFile(outputDir + "docs", std::ios_base::in);

#define ignoreLine docTitleFile.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        int currI = 0;
        for (int id = 0; id < totalDocCount; id++) {
            if (id < docIdSorted[currI].first) ignoreLine
            else {
                int idx = docIdSorted[currI].second;
                getline(docTitleFile, outputResults[idx].second);
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
    statFile >> uniqueTokensCount;

    std::ifstream milestonesFile(outputDir + "milestone.txt");
    int mileCount = ceil(uniqueTokensCount, TERMS_PER_SPLIT_FILE);
    milestoneWords.reserve(mileCount);
    for (int i = 0; i < mileCount; i++) {
        std::string str;
        milestonesFile >> str;
        milestoneWords.push_back(str);
    }

    zonePrefixMarkers['t'] = TITLE_ZONE;
    zonePrefixMarkers['i'] = INFOBOX_ZONE;
    zonePrefixMarkers['b'] = TEXT_ZONE;
    zonePrefixMarkers['c'] = CATEGORY_ZONE;
    zonePrefixMarkers['r'] = REFERENCES_ZONE;
    zonePrefixMarkers['l'] = EXTERNAL_LINKS_ZONE;

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
