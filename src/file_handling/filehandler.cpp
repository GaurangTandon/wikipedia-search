#include <vector>
#include <string>
#include "zip_operations.cpp"

std::string outputDir = "/home/gt/iiit/ire/wikipedia-search/output/";

void setOutputDir(const std::string &dir) {
    outputDir = dir + "/";
}

void writeIndex(const data_type *allDataP, const int fileNum) {
    auto &allData = *allDataP;
    std::vector<WriteBuffer> buffers(ZONE_COUNT + 2);

    for (int i = 0; i < ZONE_COUNT; i++) {
        auto filename = outputDir + "i" + zoneFirstLetter[i] + std::to_string(fileNum);
        buffers[i] = WriteBuffer(filename);
    }
    // term count; term id+doc cout for each term goes here
    buffers[ZONE_COUNT] = WriteBuffer(outputDir + "iid" + std::to_string(fileNum));
    auto &mainBuff = buffers[ZONE_COUNT + 1] = WriteBuffer(outputDir + "i" + std::to_string(fileNum));
    // the freq related information belongs to other buffers

    mainBuff.write(allData.size(), '\n');

    int termIdx = 0;
    for (const auto &term_data : allData) {
        const auto &termString = term_data.first;
        const auto &postings = term_data.second;
        // document ids in a postings list are always already sorted

        mainBuff.write(termString, ' ');
        // newline is unreliable here; as it gets stripped for the last line automatically
        // which causes bzip issue when trying to read
        mainBuff.write(postings.size(), ' ');

        for (const auto &doc_data : postings) {
            const auto &docid = doc_data.first;
            const auto &freq = doc_data.second;
            buffers[ZONE_COUNT].write(docid, ' ');

            // TODO: parallelize
            for (int i = 0; i < ZONE_COUNT; i++) {
                buffers[i].write(freq[i], ' ');
            }
        }

        termIdx++;
    }

    for (auto &buff : buffers)
        buff.close();
}
