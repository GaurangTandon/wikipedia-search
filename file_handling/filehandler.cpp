#include <map>
#include <vector>
#include <fstream>
#include <iostream>
#include <string>

int fileCount = 0;

typedef std::map<int, std::map<int, std::vector<int>>> datatype;
const auto filemode = std::ios_base::trunc | std::ios_base::out;

const std::string outputDir = "output/";

void writeIndex(const datatype &allData) {
    const std::string filename = outputDir + "index" + std::to_string(fileCount);
    fileCount++;
    std::ofstream output(filename, filemode);
    const std::string INTRA_SEP = ",";
    const std::string INTER_SEP = ";";

    for (const auto &term_data : allData) {
        const auto &termid = term_data.first;
        const auto &postings = term_data.second;
        std::string data;
        data += std::to_string(termid) + ":";

        for (const auto &doc_data : postings) {
            const auto &docid = doc_data.first;
            const auto &freq = doc_data.second;
            data += std::to_string(docid) + INTRA_SEP;

            for (int i = 0, lim = freq.size() - 1; i < lim; i++) {
                data += std::to_string(freq[i]) + INTRA_SEP;
            }

            data += std::to_string(freq.back()) + INTER_SEP;
        }

        output << data << '\n';
    }

    output.close();
}

void writeTermMapping(const std::map<std::string, int>& terms) {
    std::ofstream output(outputDir + "terms", filemode);
    for (const auto &term : terms) {
        output << term.first << ":" << term.second << '\n';
    }
    output.close();
}

void writeDocMapping(const std::map<int, std::string>& docs) {
    std::ofstream output(outputDir + "docs", filemode);
    for (const auto &doc : docs) {
        output << doc.first << ":" << doc.second << '\n';
    }
    output.close();
}
