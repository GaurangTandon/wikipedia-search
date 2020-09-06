#include <vector>
#include <string>
#include "zip_operations.cpp"

std::string outputDir = "/home/gt/iiit/ire/wikipedia-search/output/";

void setOutputDir(const std::string &dir) {
    outputDir = dir + "/";
}

typedef struct write_data_type {
    const data_type *allDataP;
    int zone;
    int fileNum;
} write_data_type;

void *writeParallel(void *dataP) {
    auto data = *(write_data_type *) dataP;
    auto &allData = *(data.allDataP);

    auto filename = outputDir + "i" + zoneFirstLetter[data.zone] + std::to_string(data.fileNum);
    std::ofstream buffer(filename);

    for (const auto &term_data : allData) {
        const auto &postings = term_data.second;

        for (const auto &doc_data : postings) {
            const auto &freq = doc_data.second;

            buffer << freq[data.zone] << ' ';
        }
    }

    buffer.close();

    return nullptr;
}

void writeIndex(const data_type *allDataP, const int fileNum) {
    auto &allData = *allDataP;

    // term count; term id+doc cout for each term goes here
    std::ofstream idBuff(outputDir + "iid" + std::to_string(fileNum));
    std::ofstream mainBuff(outputDir + "i" + std::to_string(fileNum));
    // the freq related information belongs to other buffers

    mainBuff << allData.size() << '\n';

    pthread_t threads[ZONE_COUNT];
    write_data_type *writeData[ZONE_COUNT];
    for (int i = 0; i < ZONE_COUNT; i++) {
        writeData[i] = (write_data_type *) malloc(sizeof(write_data_type));
        writeData[i]->zone = i;
        writeData[i]->allDataP = allDataP;
        writeData[i]->fileNum = fileNum;
        pthread_create(&threads[i], nullptr, writeParallel, (void *) writeData[i]);
    }

    for (const auto &term_data : allData) {
        const auto &termString = term_data.first;
        const auto &postings = term_data.second;
        // document ids in a postings list are always already sorted

        mainBuff << termString << ' ';
        // newline is unreliable here; as it gets stripped for the last line automatically
        // which causes bzip issue when trying to read
        mainBuff << postings.size() << ' ';

        for (const auto &doc_data : postings) {
            const auto &docid = doc_data.first;
            idBuff << docid << ' ';
        }
    }

    idBuff.close();
    mainBuff.close();

    for (int i = 0; i < ZONE_COUNT; i++) {
        pthread_join(threads[i], nullptr);
        free(writeData[i]);
    }
}
