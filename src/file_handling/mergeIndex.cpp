#include "zip_operations.cpp"
#include <fstream>
#include <vector>
#include <ctime>
#include <queue>
#include <cassert>

enum {
    FREQ_BUFF, ID_BUFF, MAIN_BUFF, BUFF_COUNT
};

std::string outputDir;
int fileCount;

std::ifstream ***readBuffers;
std::ofstream **writeBuffers;

std::ofstream milestoneWords;
std::ofstream statFile;
std::ofstream personalStatFile;

// pointer dereference costs very little: https://stackoverflow.com/questions/1910712/ https://stackoverflow.com/questions/431469
typedef struct readWriteType {
    int buff;
    int termCount;
    int *perTermFileCount;
    int **docCount;
    int **fileNumbers;
    bool *toWrite;
} readWriteType;

// each thread only reads buffers relevant to its zone
void *readAndWriteSequential(void *arg) {
    const auto data = (readWriteType *) arg;

    auto &buffers = readBuffers[data->buff];
    auto &writeBuff = *writeBuffers[data->buff];

    for (int termI = 0; termI < data->termCount; termI++) {
        const int lim = data->perTermFileCount[termI];
        const bool shouldWrite = data->toWrite[termI];

        if (data->buff == ID_BUFF) {
            int prevDocId = -1;

            for (int i = 0; i < lim; i++) {
                const auto &count = data->docCount[termI][i];
                const auto &fileN = data->fileNumbers[termI][i];
                auto &buff = *buffers[fileN];

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
        } else { // else it's FREQ_BUFF
            for (int i = 0; i < lim; i++) {
                const auto &count = data->docCount[termI][i];
                const auto &fileN = data->fileNumbers[termI][i];
                auto &buff = *buffers[fileN];

                for (int j = 0; j < count; j++) {
                    std::string val;
                    buff >> val;

                    if (shouldWrite) {
                        writeBuff << val << ' ';
                    }
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

    readBuffers = (std::ifstream ***) malloc(sizeof(std::ifstream **) * BUFF_COUNT);
    for (int i = 0; i < BUFF_COUNT; i++) {
        readBuffers[i] = (std::ifstream **) malloc(sizeof(std::ifstream *) * fileCount);
    }

    for (auto i = 0; i < fileCount; i++) {
        auto iStr = std::to_string(i);

        readBuffers[FREQ_BUFF][i] = new std::ifstream(outputDir + "if" + iStr);
        readBuffers[ID_BUFF][i] = new std::ifstream(outputDir + "iid" + iStr);
        readBuffers[MAIN_BUFF][i] = new std::ifstream(outputDir + "i" + iStr);
        auto &tempBuff = *readBuffers[MAIN_BUFF][i];

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

    writeBuffers = (std::ofstream **) malloc(BUFF_COUNT * sizeof(std::ofstream *));
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

    pthread_t threads[BUFF_COUNT];
    int milestoneCount = 0;

    auto initializeBuffer = [&]() {
        termsSeen = termsWritten = 0;
        latestToken = "";

        auto str = std::to_string(currentMergedCount);
        writeBuffers[FREQ_BUFF] = new std::ofstream(outputDir + "mif" + str);
        writeBuffers[ID_BUFF] = new std::ofstream(outputDir + "miids" + str);
        writeBuffers[MAIN_BUFF] = new std::ofstream(outputDir + "mimain" + str);
        currentMergedCount++;
    };

    int totalTermsWritten = 0;

    auto closeBuffers = [&]() {
        totalTermsWritten += termsWritten;
        std::cout << totalTermsWritten << std::endl;
        for (int i = 0; i < BUFF_COUNT; i++) {
            writeBuffers[i]->close();
            delete writeBuffers[i];
        }
    };

    auto flushBuffers = [&](bool reInit = true) {
        if (termsWritten > 0) {
            for (int buff = 0; buff < BUFF_COUNT - 1; buff++) {
                auto threadData = new readWriteType();
                threadData->buff = buff;
                threadData->termCount = termsSeen;
                threadData->docCount = perTermDocCount;
                threadData->fileNumbers = perTermFileNumbers;
                threadData->perTermFileCount = perTermFileCount;
                threadData->toWrite = perTermToWrite;
                pthread_create(&threads[buff], nullptr, readAndWriteSequential, threadData);
            }
            for (int buff = 0; buff < BUFF_COUNT - 1; buff++) {
                pthread_join(threads[buff], nullptr);
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
            (*readBuffers[MAIN_BUFF][fileN]) >> docCountForThisFile;
            perTermDocCount[termsSeen][currFileCount] = docCountForThisFile;
            currTokenDocCount += docCountForThisFile;
            currTokenId.pop();
            currFileCount++;
        }

        bool actualWrite = currTokenDocCount > 4;
        perTermToWrite[termsSeen] = actualWrite;

        for (int i = 0; i < currFileCount; i++) {
            int fileN = perTermFileNumbers[termsSeen][i];

            totalSizes[fileN]--;

            if (totalSizes[fileN] > 0) {
                std::string token;
                (*readBuffers[MAIN_BUFF][fileN]) >> token;
                currTokenId.emplace(token, fileN);
            }
        }

        if (actualWrite) {
            *writeBuffers[MAIN_BUFF] << smallestToken << ' ' << currTokenDocCount << ' ';
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
    free(perTermToWrite);
    free(writeBuffers);
    for (int i = 0; i < BUFF_COUNT; i++) {
        for (int j = 0; j < fileCount; j++) {
            readBuffers[i][j]->close();
            delete readBuffers[i][j];
        }
    }
    for (int i = 0; i < BUFF_COUNT; i++) {
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