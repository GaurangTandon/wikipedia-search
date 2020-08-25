#include <map>
#include <vector>
#include <fstream>
#include <iostream>
#include <string>

int fileCount = 0;

typedef std::map<int, std::map<int, std::vector<int>>> datatype;
const auto filemode = std::ios_base::trunc | std::ios_base::out;

std::string outputDir = "output/";

void setOutputDir(const std::string &dir) {
    outputDir = dir + "/";
}

void writeIndex(const datatype &allData) {
    const std::string filename = outputDir + "index" + std::to_string(fileCount);
    fileCount++;
    std::ofstream output(filename, filemode);
    const std::string INTRA_SEP = ",";
    const std::string INTER_SEP = ";";

    for (const auto &term_data : allData) {
        const auto &termid = term_data.first;
        const auto &postings = term_data.second;
        output << termid << ":";

        for (const auto &doc_data : postings) {
            const auto &docid = doc_data.first;
            const auto &freq = doc_data.second;
            output << docid << INTRA_SEP;

            int lim = freq.size() - 1;
            while (lim >= 0 and freq[lim] == 0) lim--;

            for (int i = 0; i <= lim; i++) {
                output << freq[i];

                if (i == lim) {
                    output << INTER_SEP;
                } else {
                    output << INTRA_SEP;
                }
            }
        }

        output << '\n';
    }

    output.close();
}
