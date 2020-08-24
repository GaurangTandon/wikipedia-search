#include<iostream>
#include<map>
#include<fstream>
#include<string>

std::string outputDir;

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

int main(int argc, char *argv[]) {
    if (argc != 1) return 1;
    outputDir = std::string(argv[0]);
}