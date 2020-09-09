#include "../headers/common.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <ctime>
#include <queue>
#include <cassert>

enum {
    FREQ_BUFF, ID_BUFF, MAIN_BUFF, BUFF_COUNT
};

constexpr int DOCS_PER_SPLIT_FILE = 1000000;
constexpr int TERMS_PER_SPLIT_FILE = 10000;

std::string outputDir;
int fileCount;

std::ifstream ***readBuffers;
std::ofstream **writeBuffers;

std::ofstream milestoneWords;
std::ofstream statFile;
std::ofstream personalStatFile;

void KWayMerge() {
    auto st = new timespec(), et = new timespec();
    assert(st != nullptr);
    assert(et != nullptr);
    long double timer;

    start_time

    readBuffers = (std::ifstream ***) malloc(sizeof(std::ifstream **) * BUFF_COUNT);
    assert(readBuffers != nullptr);
    for (int i = 0; i < BUFF_COUNT; i++) {
        readBuffers[i] = (std::ifstream **) malloc(sizeof(std::ifstream *) * fileCount);
        assert(readBuffers[i] != nullptr);
    }

    // { token-str, fileNumber }
    typedef std::pair<std::string, int> nextTokenType;
    std::priority_queue<nextTokenType, std::vector<nextTokenType>, std::greater<>> currTokenId;

    std::vector<int> totalSizes(fileCount, 0);
    for (auto i = 0; i < fileCount; i++) {
        auto iStr = std::to_string(i);

        readBuffers[FREQ_BUFF][i] = new std::ifstream(outputDir + "if" + iStr);
        readBuffers[ID_BUFF][i] = new std::ifstream(outputDir + "iid" + iStr);
        readBuffers[MAIN_BUFF][i] = new std::ifstream(outputDir + "i" + iStr);
        for (size_t j = 0; j < BUFF_COUNT; j++)
            if (!(readBuffers[j][i] != nullptr and *readBuffers[j][i])) exit(3);

        auto &tempBuff = *readBuffers[MAIN_BUFF][i];

        tempBuff >> totalSizes[i];
        std::string token;
        tempBuff >> token;
        currTokenId.emplace(token, i);
        totalSizes[i]--;
        if (!(*readBuffers[MAIN_BUFF][i])) exit(3);
    }

    // merged index{zone}, merged index main, merged index docids
    // mimain has termid-doccount, miids has docids one after the other
    // mif, mimain, miids
    auto perTermDocCount = (int **) malloc(sizeof(int *) * TERMS_PER_SPLIT_FILE);
    auto perTermFileNumbers = (int **) malloc(sizeof(int *) * TERMS_PER_SPLIT_FILE);
    auto perTermFileCount = (int *) malloc(sizeof(int) * TERMS_PER_SPLIT_FILE);
    auto perTermToWrite = (bool *) malloc(sizeof(bool) * TERMS_PER_SPLIT_FILE);
    assert(perTermFileCount != nullptr);
    assert(perTermFileNumbers != nullptr);
    assert(perTermDocCount != nullptr);
    assert(perTermToWrite != nullptr);
    for (int i = 0; i < TERMS_PER_SPLIT_FILE; i++) {
        perTermDocCount[i] = (int *) malloc(sizeof(int) * fileCount);
        assert(perTermDocCount[i] != nullptr);
        perTermFileNumbers[i] = (int *) malloc(sizeof(int) * fileCount);
        assert(perTermFileNumbers[i] != nullptr);
    }

    int docsSeen = 0;
    int termsSeen = 0;
    int termsWritten = 0;
    std::string latestToken;

    auto resetVars = [&]() {
        docsSeen = termsSeen = termsWritten = 0;
        latestToken = "";
    };

    int currentMergedCount = 0;
    writeBuffers = (std::ofstream **) malloc(BUFF_COUNT * sizeof(std::ofstream *));
    auto initializeBuffer = [&]() {
        resetVars();

        auto str = std::to_string(currentMergedCount);
        writeBuffers[FREQ_BUFF] = new std::ofstream(outputDir + "mif" + str);
        writeBuffers[ID_BUFF] = new std::ofstream(outputDir + "miids" + str);
        writeBuffers[MAIN_BUFF] = new std::ofstream(outputDir + "mimain" + str);
        for (size_t i = 0; i < BUFF_COUNT; i++)
            if (!(writeBuffers[i] != nullptr and *writeBuffers[i])) exit(5);

        currentMergedCount++;
    };

    int totalTermsWritten = 0;

    auto closeBuffers = [&]() {
        totalTermsWritten += termsWritten;
        std::cout << totalTermsWritten << std::endl;

        for (int i = 0; i < BUFF_COUNT; i++) {
            writeBuffers[i]->close();
            if (!(*writeBuffers[i])) exit(6);
            delete writeBuffers[i];
        }
    };

    int milestoneCount = 0;
    auto flushBuffers = [&](bool reInit = true) {
        for (int buffI = 0; buffI < BUFF_COUNT - 1; buffI++) {
            auto &buffers = readBuffers[buffI];
            auto &writeBuff = *writeBuffers[buffI];

            for (int termI = 0; termI < termsSeen; termI++) {
                const int lim = perTermFileCount[termI];
                const bool shouldWrite = perTermToWrite[termI];

                if (buffI == ID_BUFF) {
                    int prevDocId = -1;

                    for (int i = 0; i < lim; i++) {
                        const auto &count = perTermDocCount[termI][i];
                        const auto &fileN = perTermFileNumbers[termI][i];

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
                        const auto &count = perTermDocCount[termI][i];
                        const auto &fileN = perTermFileNumbers[termI][i];
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
        }

        if (termsWritten > 0) {
            milestoneWords << latestToken << " " << termsWritten << '\n';
            milestoneCount++;
        }

        if (termsWritten > 0) {
            closeBuffers();
        }

        if (reInit) {
            if (termsWritten > 0) {
                initializeBuffer();
            } else resetVars();
        } else {
            statFile << totalTermsWritten << std::endl;
            statFile << milestoneCount << std::endl;
            personalStatFile << totalTermsWritten << std::endl;
            personalStatFile << milestoneCount << std::endl;
        }
    };

    initializeBuffer();

    if (currTokenId.top().first.empty()) {
        std::cout << currTokenId.top().second << std::endl;
        exit(5);
    }

    while (not currTokenId.empty()) {
        auto smallestToken = currTokenId.top().first;

        int currTokenDocCount = 0;
        perTermFileCount[termsSeen] = 0;
        auto &currFileCount = perTermFileCount[termsSeen];

        while (not currTokenId.empty() and currTokenId.top().first == smallestToken) {
            int fileN = perTermFileNumbers[termsSeen][currFileCount] = currTokenId.top().second;

            int docCountForThisFile;
            (*readBuffers[MAIN_BUFF][fileN]) >> docCountForThisFile;
            perTermDocCount[termsSeen][currFileCount] = docCountForThisFile;
            currTokenDocCount += docCountForThisFile;

            currTokenId.pop();
            currFileCount++;
        }

        docsSeen += currTokenDocCount;

        bool actualWrite = currTokenDocCount > 4;
        perTermToWrite[termsSeen] = actualWrite;

        for (int i = 0; i < currFileCount; i++) {
            int fileN = perTermFileNumbers[termsSeen][i];

            if (totalSizes[fileN] > 0) {
                std::string token;
                (*readBuffers[MAIN_BUFF][fileN]) >> token;
                totalSizes[fileN]--;
                currTokenId.emplace(token, fileN);
            }
        }

        if (actualWrite) {
            if (latestToken.empty()) latestToken = smallestToken;
            *writeBuffers[MAIN_BUFF] << smallestToken << ' ' << currTokenDocCount << ' ';
            termsWritten++;
        }

        termsSeen++;
        if (termsSeen >= TERMS_PER_SPLIT_FILE or docsSeen >= DOCS_PER_SPLIT_FILE) {
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
            char c = readBuffers[i][j]->peek();

            assert(c == -1);

            if (!(*readBuffers[i][j])) exit(7);
            delete readBuffers[i][j];
        }
    }
    for (int i = 0; i < BUFF_COUNT; i++) {
        free(readBuffers[i]);
    }
    free(readBuffers);

    statFile.close();
    personalStatFile.close();
    if (!personalStatFile or !statFile) exit(49);

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
    if (!countFile) exit(1);
    if (!(countFile >> fileCount)) exit(1);
    countFile.close();

    milestoneWords.open(outputDir + "milestone.txt");
    if (!milestoneWords) exit(2);
    statFile.open(statFileName, std::ios_base::out | std::ios_base::app);
    if (!statFile) exit(2);
    personalStatFile.open(outputDir + "stat.txt", std::ios_base::out | std::ios_base::app);
    if (!personalStatFile) exit(2);

    KWayMerge();

    return 0;
}
