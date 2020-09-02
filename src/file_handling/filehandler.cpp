#include <vector>
#include <string>
#include "id_handler.cpp"
#include "zip_operations.cpp"

std::string outputDir = "/home/gt/iiit/ire/wikipedia-search/output/";

void setOutputDir(const std::string &dir) {
    outputDir = dir + "/";
}

void writeIndex(const data_type *allDataP, const int fileNum) {
    const auto &allData = *allDataP;
    const std::string filename = outputDir + "index" + std::to_string(fileNum);

    auto outputBuffer = WriteBuffer(filename);

    const char INTRA_SEP = ',';
    const char INTER_SEP = ';';
    std::vector<int> termIDs;
    termIDs.reserve(allData.size());

    pthread_mutex_lock(&term_id_mutex);
    for (const auto &term_data : allData) {
        termIDs.push_back(add_term(term_data.first));
    }
    pthread_mutex_unlock(&term_id_mutex);

    outputBuffer.write(allData.size(), '\n');

    int termIdx = 0;
    for (const auto &term_data : allData) {
        const auto &termid = termIDs[termIdx];
        const auto &postings = term_data.second;

        outputBuffer.write(termid, ' ');
        outputBuffer.write(postings.size(), ' ');

        for (const auto &doc_data : postings) {
            const auto &docid = doc_data.first;
            const auto &freq = doc_data.second;
            outputBuffer.write(docid, ' ');

            int lim = freq.size() - 1;
            while (lim >= 0 and freq[lim] == 0) lim--;

            for (int i = 0; i <= lim; i++) {
                char sep = (i == lim) ? INTER_SEP : INTRA_SEP;
                outputBuffer.write(freq[i], sep);
            }
        }

        outputBuffer.write('\n');
        termIdx++;
    }

    outputBuffer.close();
}
