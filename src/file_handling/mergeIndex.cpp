#include "zip_operations.cpp"
#include <fstream>
#include <vector>
#include <ctime>
#include <queue>
#include "../headers/common.h"

#define min(x, y) ((x) < (y) ? (x) : (y))

std::string outputDir;
int fileCount;

ReadBuffer ***readBuffers;
WriteBuffer **writeBuffers;

// pointer dereference costs very little: // https://stackoverflow.com/questions/1910712/ // https://stackoverflow.com/questions/431469
typedef struct readWriteType {
    int zone;
    int termCount;
    int *perTermFileCount;
    int **docCount;
    int **fileNumbers;
} readWriteType;

// each thread only reads buffers relevant to its zone
void *readAndWriteSequential(void *arg) {
    const auto data = (readWriteType *) arg;

    auto &buffers = readBuffers[data->zone];
    auto &writeBuff = writeBuffers[data->zone];

    for (int termI = 0; termI < data->termCount; termI++) {
        const int lim = data->perTermFileCount[termI];

        for (int i = 0; i < lim; i++) {
            const auto &count = data->docCount[termI][i];
            const auto &fileN = data->fileNumbers[termI][i];

            for (int j = 0; j < count; j++) {
                int val = buffers[fileN]->readInt();
                writeBuff->write(val, ' ');
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

    std::vector<int> pointers(fileCount, 0);
    std::vector<int> totalSizes(fileCount, 0);
    // { token-id, fileCount }
    std::priority_queue<std::pair<int, int>, std::vector<std::pair<int, int>>, std::greater<>> currTokenId;
    readBuffers = (ReadBuffer ***) malloc(sizeof(ReadBuffer *) * (ZONE_COUNT + 2));
    for (int i = 0; i <= ZONE_COUNT + 1; i++) {
        readBuffers[i] = (ReadBuffer **) malloc(sizeof(ReadBuffer *) * fileCount);
    }

    for (auto i = 0; i < fileCount; i++) {
        auto iStr = std::to_string(i);

        for (int j = 0; j < ZONE_COUNT; j++) {
            readBuffers[j][i] = new ReadBuffer(outputDir + "i" + zoneFirstLetter[j] + iStr);
        }
        readBuffers[ZONE_COUNT][i] = new ReadBuffer(outputDir + "iid" + iStr);
        auto &tempBuff = readBuffers[ZONE_COUNT + 1][i] = new ReadBuffer(outputDir + "i" + iStr);

        totalSizes[i] = tempBuff->readInt('\n');
        int tokenid = tempBuff->readInt();
        currTokenId.emplace(tokenid, i);
    }

    // as ou see at least as many docs in the current merged index,
    // then split into a new merged index for the new term
    int currentMergedCount = 0;
    int termsWritten = 0;

    writeBuffers = (WriteBuffer **) malloc((ZONE_COUNT + 2) * sizeof(WriteBuffer *));
    // merged index{zone}, merged index main, merged index docids
    // mimain has termid-doccount, miids has docids one after the other
    // mit, mic, mil, mib, mir, mii, mimain, miids

    auto perTermDocCount = (int **) malloc(sizeof(int *) * TERMS_PER_SPLIT_FILE);
    auto perTermFileNumbers = (int **) malloc(sizeof(int *) * TERMS_PER_SPLIT_FILE);
    auto perTermFileCount = (int *) malloc(sizeof(int) * TERMS_PER_SPLIT_FILE);
    for (int i = 0; i < TERMS_PER_SPLIT_FILE; i++) {
        perTermDocCount[i] = (int *) malloc(sizeof(int) * fileCount);
        perTermFileNumbers[i] = (int *) malloc(sizeof(int) * fileCount);
    }

    pthread_t threads[ZONE_COUNT + 1];

    auto initializeBuffer = [&]() {
        termsWritten = 0;

        auto str = std::to_string(currentMergedCount);
        for (int i = 0; i < ZONE_COUNT; i++) {
            writeBuffers[i] = new WriteBuffer(outputDir + "mi" + zoneFirstLetter[i] + str);
        }
        writeBuffers[ZONE_COUNT] = new WriteBuffer(outputDir + "miids" + str);
        writeBuffers[ZONE_COUNT + 1] = new WriteBuffer(outputDir + "mimain" + str);
        currentMergedCount++;
    };

    std::ofstream statFile("last_file_terms.txt", std::ios_base::out);

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
        for (int zone = 0; zone <= ZONE_COUNT; zone++) {
            auto threadData = new readWriteType();
            threadData->zone = zone;
            threadData->termCount = termsWritten;
            threadData->docCount = perTermDocCount;
            threadData->fileNumbers = perTermFileNumbers;
            threadData->perTermFileCount = perTermFileCount;
            pthread_create(&threads[zone], nullptr, readAndWriteSequential, threadData);
        }
        for (int i = 0; i <= ZONE_COUNT; i++) {
            pthread_join(threads[i], nullptr);
        }
        closeBuffers();

        if (reInit)
            initializeBuffer();
        else statFile << termsWritten;
    };

    initializeBuffer();

    while (not currTokenId.empty()) {
        int smallestTermId = currTokenId.top().first;

        int currTokenDocCount = 0;
        auto &currFileCount = perTermFileCount[termsWritten] = 0;

        while (not currTokenId.empty() and currTokenId.top().first == smallestTermId) {
            int fileN = perTermFileNumbers[termsWritten][currFileCount] = currTokenId.top().second;
            int docCountForThisFile = readBuffers[ZONE_COUNT + 1][fileN]->readInt();
            perTermDocCount[termsWritten][currFileCount] = docCountForThisFile;
            currTokenDocCount += docCountForThisFile;
            currTokenId.pop();
            currFileCount++;
        }

        for (int i = 0; i < currFileCount; i++) {
            int fileN = perTermFileNumbers[termsWritten][i];

            totalSizes[fileN]--;

            if (totalSizes[fileN] > 0) {
                int tokenid = readBuffers[ZONE_COUNT + 1][fileN]->readInt();
                currTokenId.emplace(tokenid, fileN);
            }
        }

        writeBuffers[ZONE_COUNT + 1]->write(smallestTermId, ' ');
        writeBuffers[ZONE_COUNT + 1]->write(currTokenDocCount, ' ');

        termsWritten++;

        if (termsWritten >= TERMS_PER_SPLIT_FILE) {
            flushBuffers();
        }
    }
    if (termsWritten > 0)
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
}

int main(int argc, char *argv[]) {
    if (argc == 2) {
        outputDir = std::string(std::string(argv[1])) + "/";
    }

    std::ifstream countFile(outputDir + "file_stat.txt");
    countFile >> fileCount;

    KWayMerge();

    return 0;
}