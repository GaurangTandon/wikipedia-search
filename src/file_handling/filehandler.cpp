#include <vector>
#include <string>
#include <algorithm>
#include "id_handler.cpp"
#include "zip_operations.cpp"

std::string outputDir = "/home/gt/iiit/ire/wikipedia-search/output/";

void setOutputDir(const std::string &dir) {
    outputDir = dir + "/";
}

void writeIndex(const data_type *allDataP, const int fileNum) {
    auto &allData = *allDataP;
    std::vector<WriteBuffer> buffers(ZONE_COUNT + 1);

    for (int i = 0; i < ZONE_COUNT; i++) {
        auto filename = outputDir + "i" + zoneFirstLetter[i] + std::to_string(fileNum);
        buffers[i] = WriteBuffer(filename);
    }
    // term count; term id+doc cout for each term goes here
    buffers[ZONE_COUNT] = WriteBuffer(outputDir + "i" + std::to_string(fileNum));
    auto &mainBuff = buffers.back();
    // the freq related information belongs to other buffers

    std::vector<std::pair<int, std::string>> termIDs;
    termIDs.reserve(allData.size());

    pthread_mutex_lock(&term_id_mutex);
    for (const auto &term_data : allData) {
        termIDs.emplace_back(add_term(term_data.first), term_data.first);
    }
    pthread_mutex_unlock(&term_id_mutex);
    std::sort(termIDs.begin(), termIDs.end());

    mainBuff.write(allData.size(), '\n');

    int termIdx = 0;
    for (const auto &term_data : termIDs) {
        const auto &termid = term_data.first;
        const auto &postings = allData.at(term_data.second);
        // document ids in a postings list are always already sorted

        mainBuff.write(termid, ' ');
        mainBuff.write(postings.size(), ' ');

        for (const auto &doc_data : postings) {
            const auto &docid = doc_data.first;
            const auto &freq = doc_data.second;
            mainBuff.write(docid, ' ');

            // TODO: parallelize
            for (int i = 0; i < ZONE_COUNT; i++) {
                buffers[i].write(freq[i], ' ');
            }
        }

        mainBuff.write('\n');
        termIdx++;
    }

    for (auto &buff : buffers)
        buff.close();
}
