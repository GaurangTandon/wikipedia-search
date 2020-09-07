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

typedef long double score_value;
typedef std::pair<score_value, int> score_type; // { score, page-id }
// least scoring element at the top so it can be popped
typedef std::priority_queue<score_type, std::vector<score_type>, std::greater<>> results_container;
typedef std::map<int, score_value> score_container;
typedef struct thread_data {
    score_container *docScores;
    int zone;
    Preprocessor *processor;
} thread_data;
std::vector<int> zonePrefixMarkers(255, -1);
constexpr int BOOST_FACTOR = 10;

int totalDocCount;
int uniqueTokensCount;
int mileCount;
std::vector<std::string> milestoneWords;
std::vector<int> milestoneSizes;

std::string outputDir;
// [zone][boolean boosted]
std::vector<std::vector<std::string>> zonalQueries(ZONE_COUNT, std::vector<std::string>(2, ""));
int K;

constexpr inline score_value sublinear_scaling(int termFreq) {
    return termFreq > 0 ? (1 + log10(termFreq)) : 0;
}

int findTermFreq(const std::string &freqStr, int zone) {
    int num = 0, i = 0;
#define numgen while (i < freqStr.size() and Preprocessor::isnum(freqStr[i])) { \
        num = num * 10 + (freqStr[i] - '0'); \
        i++; \
    }

    if (zone == TEXT_ZONE) {
        numgen

        return num;
    }

    while (i < freqStr.size() and freqStr[i] != zoneFirstLetter[zone][0]) {
        i++;
    }

    i++; // skip the marker

    numgen

    return num;
}

void extractZonalQueries(const std::string &query) {
    for (auto &x : zonalQueries) { for (auto &y : x) y.clear(); }

    constexpr char QUERY_SEP = ':';

    int currZone = -1;

    for (int i = 0; i < query.size(); i++) {
        auto c = query[i];

        if (zonePrefixMarkers[c] > -1 and i < query.size() - 1 and query[i + 1] == QUERY_SEP) {
            currZone = zonePrefixMarkers[c];
            i++;
            continue;
        }

        if (currZone != -1) {
            zonalQueries[currZone][1] += c;
        } else
            for (int zoneI = 0; zoneI < ZONE_COUNT; zoneI++) {
                zonalQueries[zoneI][0] += c;
            }
    }
}

inline int getIndex(const std::string &token) {
    auto lowIt = std::upper_bound(milestoneWords.begin(), milestoneWords.end(), token) - 1;
    int idx = lowIt - milestoneWords.begin();
    return idx;
}

// PRECONDITION: query is not empty
void *performSearch(void *dataP) {
    auto &data = *((thread_data *) dataP);

    for (int boost = 0; boost < 2; boost++) {
        const auto &query = zonalQueries[data.zone][boost];
        if (query.empty()) continue;

        auto tokens = data.processor->getStemmedTokens(query, 0, query.size() - 1);
        if (tokens.empty()) continue;
        sort(tokens.begin(), tokens.end());

        int prevFile = -1;

        std::ifstream mainBuff, idBuff, freqBuff;

        int readCount, readLim;

        for (const auto &token : tokens) {
            int fileNum = getIndex(token);

            if (prevFile != fileNum) {
                auto str = std::to_string(fileNum);

                mainBuff = std::ifstream(outputDir + "mimain" + str);
                idBuff = std::ifstream(outputDir + "miids" + str);
                freqBuff = std::ifstream(outputDir + "mif" + str);
                prevFile = fileNum;

                readLim = milestoneSizes[fileNum];

                readCount = 0;
            }

            std::vector<score_type> thisTokenScores;
            while (readCount < readLim) {
                readCount++;

                std::string currToken;
                mainBuff >> currToken;
                int docCount;
                mainBuff >> docCount;

                // can store this variable in the file itself
                int actualDocCount = 0; // number of documents with this term in their zone
                const bool isCurrTokenReq = currToken == token;
                int prevDocId = -1;

                for (int f = 0; f < docCount; f++) {
                    int docId;
                    idBuff >> docId;
                    std::string freqData;
                    freqBuff >> freqData;

                    if (isCurrTokenReq) {
                        const int correctDocId = (prevDocId == -1) ? docId : prevDocId + docId;

//                        std::cout << std::to_string(correctDocId) + " ";
                        int termFreqInDoc = findTermFreq(freqData, data.zone);

                        if (correctDocId == 8662854)
                            std::cout << termFreqInDoc << " " << freqData << std::endl;

                        if (termFreqInDoc > 0) {
                            auto value = sublinear_scaling(termFreqInDoc);
                            thisTokenScores.emplace_back(value, correctDocId);
                            actualDocCount += (termFreqInDoc > 0);
                        }

                        prevDocId = correctDocId;
                    }
                }

                if (isCurrTokenReq and actualDocCount > 0) {
                    score_value denom = log10l((score_value) totalDocCount / actualDocCount);

                    for (auto e : thisTokenScores) {
                        constexpr int id = 8727168 - 1;

                        e.first *= denom;
                        if (boost) e.first *= BOOST_FACTOR;

                        (*data.docScores)[e.second] += e.first;
                    }

                    break;
                }

            }
        }
    }

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
    for (auto &c : query) c = Preprocessor::lowercase(c);
    extractZonalQueries(query);

    results_container results;

    std::map<int, score_value> docScores; // maximal size would be ZONE_COUNT * K

    int zoneI = 0;
    pthread_t threads[ZONE_COUNT];
    thread_data *threads_data[ZONE_COUNT];

    for (const auto &zone : zonalQueries) {
        auto data = (thread_data *) malloc(sizeof(thread_data));
        data->zone = zoneI;
        data->docScores = new score_container();
        data->processor = new Preprocessor();
        threads_data[zoneI] = data;
        pthread_create(&threads[zoneI], nullptr, performSearch, (void *) data);

        zoneI++;
    }

    zoneI = 0;
    for (const auto &zone : zonalQueries) {
        pthread_join(threads[zoneI], nullptr);
        auto &data = *threads_data[zoneI];
        auto &intermediateResults = *data.docScores;

        for (auto &e : intermediateResults) {
            docScores[e.first] += e.second * zoneSearchWeights[zoneI];
        }

        delete data.docScores;
        delete data.processor;
        free(threads_data[zoneI]);

        zoneI++;
    }

    for (auto &e : docScores) {
        results.push({e.second, e.first});
        if (results.size() > K) results.pop();
    }

    if (results.empty()) {
        for (int i = 0; i < K; i++) {
            results.push({0, i});
        }
    }

    // sorted from most score->least score
    std::vector<std::pair<int, std::string>> outputResults(K);
    std::vector<score_value> scores(K);
    std::vector<std::pair<int, int>> docIdSorted(results.size()); // id, index

    for (int i = int(results.size()) - 1; i >= 0; i--) {
        int docid = results.top().second;
        scores[i] = results.top().first;
        outputResults[i].first = docid;
        docIdSorted[i] = {docid, i};
        results.pop();
    }

    sort(docIdSorted.begin(), docIdSorted.end());
    int docIdTemp = 0, currI = 0;
    while (docIdSorted.size() < K) {
        if (currI < docIdSorted.size() and docIdSorted[currI].first == docIdTemp) {
            currI++;
            docIdTemp++;
            continue;
        }

        int newidx = docIdSorted.size();
        docIdSorted.emplace_back(docIdTemp, newidx);
        outputResults[newidx].first = docIdTemp;
        docIdTemp++;
    }

    // Get string representation of title, and print it
    {
        std::ifstream docTitleFile(outputDir + "docs", std::ios_base::in);

#define ignoreLine docTitleFile.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        int currDocArrayIdx = 0;
        for (int id = 0; id < totalDocCount; id++) {
            if (id < docIdSorted[currDocArrayIdx].first) ignoreLine
            else {
                int idx = docIdSorted[currDocArrayIdx].second;
                getline(docTitleFile, outputResults[idx].second);
                currDocArrayIdx++;
                if (currDocArrayIdx == K) break;
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

    outputDir = std::string(argv[1]) + "/";

    std::ifstream statFile(outputDir + "stat.txt");
    statFile >> totalDocCount; // first read is actually file count
    statFile >> totalDocCount;
    statFile >> uniqueTokensCount;
    statFile >> mileCount;

    std::ifstream milestonesFile(outputDir + "milestone.txt");
    milestoneWords.reserve(mileCount);
    milestoneSizes.reserve(mileCount);
    for (int i = 0; i < mileCount; i++) {
        std::string str;
        int terms;
        milestonesFile >> str >> terms;
        milestoneWords.push_back(str);
        milestoneSizes.push_back(terms);
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

    delete st;
    delete et;

    return 0;
}
