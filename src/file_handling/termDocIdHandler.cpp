#include<iostream>
#include<map>
#include<string>
#include "filehandler.cpp"

void writeTermMapping(std::map<std::string, int> &terms) {
    terms.erase("");

    std::ofstream output(outputDir + "terms", filemode);

    output << terms.size() << '\n';
    for (const auto &term : terms) {
        output << term.first << " " << term.second << '\n';
    }
    output.close();
}

void writeDocMapping(const std::map<int, std::string> &docs) {
    std::ofstream output(outputDir + "docs", filemode);
    output << docs.size() << '\n';
    int i = 0;
    for (const auto &doc : docs) {
        // TODO: remove this later
        assert(doc.first == ++i);
        output << doc.second << '\n';
    }
    assert(i == docs.size());
    output.close();
}
