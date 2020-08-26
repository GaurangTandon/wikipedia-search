#include<iostream>
#include<map>
#include<string>
#include "filehandler.cpp"

std::map<std::string, int> termIDmapping;
int termsCount = 1;
std::map<int, std::string> docDetails;
int docCount = 1;

pthread_mutex_t term_id_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t doc_id_mutex = PTHREAD_MUTEX_INITIALIZER;

int get_termid(const std::string &term) {
    if (not termIDmapping[term]) {
        pthread_mutex_lock(&term_id_mutex);
        if (not termIDmapping[term]) {
            termIDmapping[term] = termsCount++;
        }
        pthread_mutex_unlock(&term_id_mutex);
    }
    const int id = termIDmapping[term];
    return id;
}

int get_docid() {
    pthread_mutex_lock(&doc_id_mutex);
    int id = docCount++;
    pthread_mutex_unlock(&doc_id_mutex);
    return id;
}

void writeTermMapping(const std::map<std::string, int> &terms) {
    std::ofstream output(outputDir + "terms", filemode);
    for (const auto &term : terms) {
        output << term.first << ":" << term.second << '\n';
    }
    output.close();
}

void writeDocMapping(const std::map<int, std::string> &docs) {
    std::ofstream output(outputDir + "docs", filemode);
    for (const auto &doc : docs) {
        output << doc.first << ":" << doc.second << '\n';
    }
    output.close();
}
