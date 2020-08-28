#include <vector>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include "id_handler.cpp"

int fileCount = 0;

// Hopefully disabling io:sync makes it faster, if not, we need to switch to
// fopen and printf
const auto filemode = std::ios_base::out | std::ios_base::binary; // binary mode probably faster

std::string outputDir = "/home/gt/iiit/ire/wikipedia-search/output/";

void setOutputDir(const std::string &dir) {
    outputDir = dir + "/";
}

void writeIndex(const data_type &allData) {
    const std::string filename = outputDir + "index" + std::to_string(fileCount);
    fileCount++;

    std::ofstream output(filename, filemode);
    const std::string INTRA_SEP = ",";
    const std::string INTER_SEP = ";";

#define USE_SS

    pthread_mutex_lock(&term_id_mutex);
    for (const auto &term_data : allData) {
        add_term(term_data.first);
    }
    pthread_mutex_unlock(&term_id_mutex);

    for (const auto &term_data : allData) {
        const auto &termid = get_termid(term_data.first);
        const auto &postings = term_data.second;

#ifdef USE_SS
        std::stringstream line;
        line << termid << ":";
#else
        output << termid << ":";
#endif

        for (const auto &doc_data : postings) {
            const auto &docid = doc_data.first;
            const auto &freq = doc_data.second;
#ifdef USE_SS
            line << docid << INTRA_SEP;
#else
            output << docid << INTRA_SEP;
#endif

            int lim = freq.size() - 1;
            while (lim >= 0 and freq[lim] == 0) lim--;

            for (int i = 0; i <= lim; i++) {
#ifdef USE_SS
                line << freq[i];
#else
                output << freq[i];
#endif

                if (i == lim) {
#ifdef USE_SS
                    line << INTER_SEP;
#else
                    output << INTER_SEP;
#endif
                } else {
#ifdef USE_SS
                    line << INTRA_SEP;
#else
                    output << INTRA_SEP;
#endif
                }
            }
        }

#ifdef USE_SS
        line << '\n';
        output << line.rdbuf();
#else
        output << '\n';
#endif
    }

    output.close();
}
