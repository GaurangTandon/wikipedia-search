#include <vector>
#include <string>
#include "zip_operations.cpp"

std::string outputDir = "/home/gt/iiit/ire/wikipedia-search/output/";

void setOutputDir(const std::string &dir) {
    outputDir = dir + "/";
}

void writeIndex(const data_type *allDataP, const int fileNum) {
    auto &allData = *allDataP;

    // term count; term id+doc cout for each term goes here
    std::ofstream idBuff(outputDir + "iid" + std::to_string(fileNum));
    std::ofstream mainBuff(outputDir + "i" + std::to_string(fileNum));
    std::ofstream freqBuffer(outputDir + "if" + std::to_string(fileNum));

    mainBuff << allData.size() << '\n';

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
            const auto &freq = doc_data.second;
            idBuff << docid << ' ';

            for (int i = 0; i < ZONE_COUNT; i++) {
                if (freq[i]) {
                    if (i == 0) freqBuffer << freq[i];
                    else freqBuffer << zoneFirstLetter[i] << freq[i];
                }
            }
            freqBuffer << ' ';
        }
    }

    idBuff.close();
    mainBuff.close();
    freqBuffer.close();
}
