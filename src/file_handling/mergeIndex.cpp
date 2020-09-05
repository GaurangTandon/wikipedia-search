#include "zip_operations.cpp"
#include <fstream>
#include <vector>
#include <cassert>
#include <queue>
#include "../headers/common.h"

#define min(x, y) ((x) < (y) ? (x) : (y))

std::string outputDir;
int fileCount;

// pointer dereference costs very little: // https://stackoverflow.com/questions/1910712/ // https://stackoverflow.com/questions/431469
typedef struct readWriteType {
    int zone;
    int fileCount;
    int *docCount;
    int *fileNumbers;
    ReadBuffer ***readBuffer;
    WriteBuffer **writeBuffer;
} readWriteType;

// each thread only reads buffers relevant to its zone
void *readAndWriteSequential(void *arg) {
    const auto data = (readWriteType *) arg;
    const int lim = data->fileCount;

    auto &buffers = data->readBuffer[data->zone];

    for (int i = 0; i < lim; i++) {
        const auto &count = data->docCount[i];
        const auto &fileN = data->fileNumbers[i];

        for (int j = 0; j < count; j++) {
            int val = buffers[fileN]->readInt();
            data->writeBuffer[data->zone]->write(val, ' ');
        }

        if (data->zone == ZONE_COUNT) {
            assert(buffers[fileN]->readChar() == '\n');
        }
    }

    delete data;
    return nullptr;
}

void KWayMerge() {
    std::vector<int> pointers(fileCount, 0);
    std::vector<int> totalSizes(fileCount, 0);
    // { token-id, fileCount }
    std::priority_queue<std::pair<int, int>, std::vector<std::pair<int, int>>, std::less<>> currTokenId;
    auto readBuffers = (ReadBuffer ***) malloc(sizeof(ReadBuffer *) * (ZONE_COUNT + 1));
    for (int i = 0; i <= ZONE_COUNT; i++) {
        readBuffers[i] = (ReadBuffer **) malloc(sizeof(ReadBuffer *) * fileCount);
    }

    for (auto i = 0; i < fileCount; i++) {
        auto iStr = std::to_string(i);

        for (int j = 0; j < ZONE_COUNT; j++) {
            readBuffers[j][i] = new ReadBuffer(outputDir + "i" + zoneFirstLetter[j] + iStr);
        }
        auto &tempBuff = readBuffers[ZONE_COUNT][i] = new ReadBuffer(outputDir + "i" + iStr);

        totalSizes[i] = tempBuff->readInt('\n');
        int tokenid = tempBuff->readInt();
        currTokenId.emplace(tokenid, i);
    }

    // as ou see at least as many docs in the current merged index,
    // then split into a new merged index for the new term
    constexpr int DOC_THRESHOLD = 100000;
    int currentMergedCount = 0;
    int currentDocCount = 0;
    int termsWritten = 0;

    auto writeBuffers = (WriteBuffer **) malloc((ZONE_COUNT + 2) * sizeof(WriteBuffer *));
    // merged index{zone}, merged index main, merged index docids
    // mimain has termid-doccount, miids has docids one after the other
    // mit, mic, mil, mib, mir, mii, mimain, miids

    auto initializeBuffer = [&]() {
        currentDocCount = 0;
        termsWritten = 0;

        auto str = std::to_string(currentMergedCount);
        for (int i = 0; i < ZONE_COUNT; i++) {
            writeBuffers[i] = new WriteBuffer(outputDir + "mi" + zoneFirstLetter[i] + str);
        }
        writeBuffers[ZONE_COUNT] = new WriteBuffer(outputDir + "miids" + str);
        writeBuffers[ZONE_COUNT + 1] = new WriteBuffer(outputDir + "mimain" + str);
        currentMergedCount++;
    };

    std::ofstream statFile("final_stat.txt", std::ios_base::out);

    auto closeBuffers = [&]() {
        for (int i = 0; i <= ZONE_COUNT + 1; i++) {
            writeBuffers[i]->close();
        }
        statFile << termsWritten << std::endl;
    };

    initializeBuffer();

    auto docCount = (int *) malloc(sizeof(int) * fileCount);
    auto fileNumbers = (int *) malloc(sizeof(int) * fileCount);

    while (not currTokenId.empty()) {
        int smallestTermId = currTokenId.top().first;
        termsWritten++;

        int totalDocCount = 0;
        int currFileCount = 0;

        while (not currTokenId.empty() and currTokenId.top().first == smallestTermId) {
            int fileN = fileNumbers[currFileCount] = currTokenId.top().second;
            int docCountForThisFile = readBuffers[ZONE_COUNT][fileN]->readInt();
            docCount[currFileCount] = docCountForThisFile;
            totalDocCount += docCountForThisFile;
            currTokenId.pop();
            currFileCount++;
        }

        pthread_t threads[ZONE_COUNT + 1];

        for (int zone = 0; zone <= ZONE_COUNT; zone++) {
            auto threadData = new readWriteType();
            threadData->zone = zone;
            threadData->writeBuffer = writeBuffers;
            threadData->readBuffer = readBuffers;
            threadData->docCount = docCount;
            threadData->fileNumbers = fileNumbers;
            threadData->fileCount = currFileCount;
            pthread_create(&threads[zone], nullptr, readAndWriteSequential, threadData);
        }
        for (int i = 0; i <= ZONE_COUNT; i++) {
            pthread_join(threads[i], nullptr);
        }
        writeBuffers[ZONE_COUNT + 1]->write(smallestTermId, ' ');
        writeBuffers[ZONE_COUNT + 1]->write(totalDocCount, ' ');

        for (int i = 0; i < currFileCount; i++) {
            int fileN = fileNumbers[i];

            totalSizes[fileN]--;

            if (totalSizes[fileN] > 0) currTokenId.emplace(readBuffers[ZONE_COUNT][fileN]->readInt(), fileN);
        }

        currentDocCount += totalDocCount;
        if (currentDocCount >= DOC_THRESHOLD) {
            closeBuffers();
            initializeBuffer();
        }
    }
    if (currentDocCount > 0)
        closeBuffers();
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