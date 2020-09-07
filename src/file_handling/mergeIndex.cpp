#include "zip_operations.cpp"
#include <fstream>
#include <vector>
#include <ctime>
#include <queue>
#include <cassert>

std::string outputDir;
int fileCount;

std::ifstream ***readBuffers;
std::ofstream **writeBuffers;

std::ofstream milestoneWords;
std::ofstream statFile;
std::ofstream personalStatFile;

// pointer dereference costs very little: https://stackoverflow.com/questions/1910712/ https://stackoverflow.com/questions/431469
typedef struct readWriteType {
    int zone;
    int termCount;
    int *perTermFileCount;
    int **docCount;
    int **fileNumbers;
    bool *toWrite;
} readWriteType;

// each thread only reads buffers relevant to its zone
void *readAndWriteSequential(void *arg) {
    const auto data = (readWriteType *) arg;

    auto &buffers = readBuffers[data->zone];
    auto &writeBuff = *writeBuffers[data->zone];

    for (int termI = 0; termI < data->termCount; termI++) {
        const int lim = data->perTermFileCount[termI];
        const bool shouldWrite = data->toWrite[termI];

        for (int i = 0; i < lim; i++) {
            const auto &count = data->docCount[termI][i];
            const auto &fileN = data->fileNumbers[termI][i];
            auto &buff = *buffers[fileN];
            int prevDocId = -1;

            for (int j = 0; j < count; j++) {
                int val;
                buff >> val;

                if (shouldWrite) {
                    int valToWrite = prevDocId == -1 ? val : val - prevDocId;
                    writeBuff << valToWrite << ' ';

                    prevDocId = val;
                }
            }
        }
    }

    delete data;
    return nullptr;
}

void KWayMerge() {
    auto st = new timespec(), et = new timespec();
    long double timer;

    start_time

    std::string latestToken;
    std::vector<int> pointers(fileCount, 0);
    std::vector<int> totalSizes(fileCount, 0);
    // { token-str, fileCount }
    typedef std::pair<std::string, int> nextTokenType;
    std::priority_queue<nextTokenType, std::vector<nextTokenType>, std::greater<>> currTokenId;

    readBuffers = (std::ifstream ***) malloc(sizeof(std::ifstream **) * (ZONE_COUNT + 2));
    for (int i = 0; i <= ZONE_COUNT + 1; i++) {
        readBuffers[i] = (std::ifstream **) malloc(sizeof(std::ifstream *) * fileCount);
    }

    for (auto i = 0; i < fileCount; i++) {
        auto iStr = std::to_string(i);

        for (int j = 0; j < ZONE_COUNT; j++) {
            readBuffers[j][i] = new std::ifstream(outputDir + "i" + zoneFirstLetter[j] + iStr);
        }
        readBuffers[ZONE_COUNT][i] = new std::ifstream(outputDir + "iid" + iStr);
        readBuffers[ZONE_COUNT + 1][i] = new std::ifstream(outputDir + "i" + iStr);
        auto &tempBuff = *readBuffers[ZONE_COUNT + 1][i];

        tempBuff >> totalSizes[i];
        std::string token;
        tempBuff >> token;
        currTokenId.emplace(token, i);
    }

    // as ou see at least as many docs in the current merged index,
    // then split into a new merged index for the new term
    int currentMergedCount = 0;
    int termsSeen = 0;
    int termsWritten = 0;

    writeBuffers = (std::ofstream **) malloc((ZONE_COUNT + 2) * sizeof(std::ofstream *));
    // merged index{zone}, merged index main, merged index docids
    // mimain has termid-doccount, miids has docids one after the other
    // mit, mic, mil, mib, mir, mii, mimain, miids

    auto perTermDocCount = (int **) malloc(sizeof(int *) * TERMS_PER_SPLIT_FILE);
    auto perTermFileNumbers = (int **) malloc(sizeof(int *) * TERMS_PER_SPLIT_FILE);
    auto perTermFileCount = (int *) malloc(sizeof(int) * TERMS_PER_SPLIT_FILE);
    auto perTermToWrite = (bool *) malloc(sizeof(bool) * TERMS_PER_SPLIT_FILE);
    for (int i = 0; i < TERMS_PER_SPLIT_FILE; i++) {
        perTermDocCount[i] = (int *) malloc(sizeof(int) * fileCount);
        perTermFileNumbers[i] = (int *) malloc(sizeof(int) * fileCount);
    }

    pthread_t threads[ZONE_COUNT + 1];
    int milestoneCount = 0;

    auto initializeBuffer = [&]() {
        termsSeen = termsWritten = 0;
        latestToken = "";

        auto str = std::to_string(currentMergedCount);
        for (int i = 0; i < ZONE_COUNT; i++) {
            writeBuffers[i] = new std::ofstream(outputDir + "mi" + zoneFirstLetter[i] + str);
        }
        writeBuffers[ZONE_COUNT] = new std::ofstream(outputDir + "miids" + str);
        writeBuffers[ZONE_COUNT + 1] = new std::ofstream(outputDir + "mimain" + str);
        currentMergedCount++;
    };

    int totalTermsWritten = 0;

    auto closeBuffers = [&]() {
        totalTermsWritten += termsWritten;
        std::cout << totalTermsWritten << std::endl;
        for (int i = 0; i <= ZONE_COUNT + 1; i++) {
            writeBuffers[i]->close();
            delete writeBuffers[i];
        }
    };

    auto flushBuffers = [&](bool reInit = true) {
        if (termsWritten > 0) {
            for (int zone = 0; zone <= ZONE_COUNT; zone++) {
                auto threadData = new readWriteType();
                threadData->zone = zone;
                threadData->termCount = termsSeen;
                threadData->docCount = perTermDocCount;
                threadData->fileNumbers = perTermFileNumbers;
                threadData->perTermFileCount = perTermFileCount;
                threadData->toWrite = perTermToWrite;
                pthread_create(&threads[zone], nullptr, readAndWriteSequential, threadData);
            }
            for (int i = 0; i <= ZONE_COUNT; i++) {
                pthread_join(threads[i], nullptr);
            }
            milestoneWords << latestToken << " " << termsWritten << '\n';
            milestoneCount++;
        }

        closeBuffers();

        if (reInit)
            initializeBuffer();
        else {
            statFile << totalTermsWritten << std::endl;
            statFile << milestoneCount << std::endl;
            personalStatFile << totalTermsWritten << std::endl;
            personalStatFile << milestoneCount << std::endl;
        }
    };

    initializeBuffer();

    while (not currTokenId.empty()) {
        auto smallestToken = currTokenId.top().first;
        if (latestToken.empty()) latestToken = smallestToken;

        int currTokenDocCount = 0;
        auto &currFileCount = perTermFileCount[termsSeen] = 0;

        while (not currTokenId.empty() and currTokenId.top().first == smallestToken) {
            int fileN = perTermFileNumbers[termsSeen][currFileCount] = currTokenId.top().second;
            int docCountForThisFile;
            (*readBuffers[ZONE_COUNT + 1][fileN]) >> docCountForThisFile;
            perTermDocCount[termsSeen][currFileCount] = docCountForThisFile;
            currTokenDocCount += docCountForThisFile;
            currTokenId.pop();
            currFileCount++;
        }

        // if this word is only ever mentioned in 2 (!) documents
        bool actualWrite = currTokenDocCount > 4;
        perTermToWrite[termsSeen] = actualWrite;

        for (int i = 0; i < currFileCount; i++) {
            int fileN = perTermFileNumbers[termsSeen][i];

            totalSizes[fileN]--;

            if (totalSizes[fileN] > 0) {
                std::string token;
                (*readBuffers[ZONE_COUNT + 1][fileN]) >> token;
                currTokenId.emplace(token, fileN);
            }
        }

        if (actualWrite) {
            *writeBuffers[ZONE_COUNT + 1] << smallestToken << ' ' << currTokenDocCount << ' ';
            termsWritten++;
        }

        termsSeen++;
        if (termsSeen >= TERMS_PER_SPLIT_FILE) {
            flushBuffers();
        }
    }

    if (termsSeen > 0)
        flushBuffers(false);

    for (int i = 0; i < TERMS_PER_SPLIT_FILE; i++) {
        free(perTermFileNumbers[i]);
        free(perTermDocCount[i]);
    }
    free(perTermFileNumbers);
    free(perTermDocCount);
    free(perTermFileCount);
    free(writeBuffers);
    for (int i = 0; i <= ZONE_COUNT + 1; i++) {
        for (int j = 0; j < fileCount; j++) {
            readBuffers[i][j]->close();
            delete readBuffers[i][j];
        }
    }
    for (int i = 0; i <= ZONE_COUNT + 1; i++) {
        free(readBuffers[i]);
    }
    free(readBuffers);

    end_time
    std::cout << "Time taken to merge indexes and create split sorted files " << timer << std::endl;

    delete st;
    delete et;
}

int main(int argc, char *argv[]) {
    std::string statFileName;

    assert(argc == 3);
    outputDir = std::string(argv[1]) + "/";
    statFileName = std::string(argv[2]);

    std::ifstream countFile(outputDir + "stat.txt");
    countFile >> fileCount;
    countFile.close();

    milestoneWords.open(outputDir + "milestone.txt");
    statFile.open(statFileName, std::ios_base::out | std::ios_base::app);
    personalStatFile.open(outputDir + "stat.txt", std::ios_base::out | std::ios_base::app);

    KWayMerge();

    return 0;
}