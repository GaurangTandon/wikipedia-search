#include <xml/parser>
#include<string>
#include<iostream>
#include<fstream>
#include<map>
#include <time.h>
#include<vector>
#include<utility>
#include "../preprocess/preprocess.cpp"
#include "../file_handling/filehandler.cpp"

/*
 * next_expect(): if `content` argument is set, then event type must be start element
 */

#define DEBUG std::cout << p.next() << " " << p.value() << " " << p.name() << " " << p.qname() << std::endl;

const std::string NS = "http://www.mediawiki.org/xml/export-0.10/";
const std::string filePath = "parser/medium.xml";

enum {
    INFOBOX_ZONE, CATEGORY_ZONE, TEXT_ZONE, ZONE_COUNT
} zones;

struct timespec *st = new timespec(), *et = new timespec();
constexpr int MAX_CHECK = 5;
int currCheck = 0;

std::map<std::string, int> termIDmapping;
int termsCount = 1;
std::map<int, std::string> docDetails;
int docCount = 1;

class WikiPage;

constexpr int MX_MEM = 5000;
WikiPage *mem[MX_MEM];
int curr = 0;

class WikiPage {
public:
    std::string title, text;
    // each part is governed by zonal markers
    std::vector<std::vector<std::string>> terms;

    WikiPage(xml::parser &p) {
        terms.resize(ZONE_COUNT);

        p.next_expect(xml::parser::start_element, NS, "page");

        bool inTitle = false, inText = false;

        for (auto e : p) {
            switch (e) {
                case xml::parser::start_element:
                    if (p.name() == "title") {
                        inTitle = true;
                    }
                    if (p.name() == "text") {
                        inText = true;
                    }
                    break;
                case xml::parser::end_element:
                    if (p.name() == "title") {
                        inTitle = false;
                    }
                    if (p.name() == "text") {
                        inText = false;
                    }
                    if (p.name() == "page") {
                        cleanup();
                        return;
                    }
                    break;
                case xml::parser::characters:
                    if (inTitle) {
                        title += p.value();
                    }
                    if (inText) {
                        text += p.value();
                    }
                    break;
                default:
                    break;
            }
        }
    }

    void cleanup() {
    }
};

const std::string INFOBOX = "{{Infobox";
const std::string CATEGORY = "[[category";
Preprocessor *processor;

int extractInfobox(WikiPage *page, const std::string &text, int start) {
    int cnt = 0;
    int end = start;

    assert(not text.empty());

    while (end < text.size() - 1) {
        if (text[end] == '{' and text[end + 1] == '{') cnt++;
        else if (text[end] == '}' and text[end + 1] == '}') {
            cnt--;
            if (cnt == 0) {
                end++;
                break;
            }
        }

        end++;
    }

    assert (cnt == 0);

    // start..end is inclusive
    auto infobox = text.substr(start, end + 1);

    page->terms[INFOBOX_ZONE] = processor->processText(infobox);

    return end;
}

int extractCategory(WikiPage *page, const std::string &text, int start) {
    int end = start;

    assert(not text.empty());

    while (end < text.size() - 1) {
        if (text[end] == ']' and text[end + 1] == ']') {
            end++;
            break;
        }
        end++;
    }

    auto category = text.substr(start + CATEGORY.size(), end + 1);

    auto tokens = processor->processText(category);
    page->terms[CATEGORY_ZONE].insert(page->terms[CATEGORY_ZONE].end(), tokens.begin(), tokens.end());

    return end;
}

void extractData(WikiPage *page) {
    auto &text = page->text;
    std::string bodyText;

    for (int i = 0; i < text.size(); i++) {
        if (processor->fast_equals(text, INFOBOX, i)) {
            i = extractInfobox(page, text, i);
        } else if (processor->fast_equals(text, CATEGORY, i)) {
            i = extractCategory(page, text, i);
        } else {
            bodyText += text[i];
        }
    }

    page->terms[TEXT_ZONE] = processor->processText(bodyText);
}

class WikiSiteInfo {
public:
    WikiSiteInfo(xml::parser &p) {
        p.next_expect(xml::parser::start_element, NS, "siteinfo", xml::content::complex);

        for (auto e : p) {
            if (e == xml::parser::start_element) {
            }
            if (e == xml::parser::end_element) {
                if (p.name() == "siteinfo") return;
            }
        }

        assert(false);
    }
};

long double timer;

#define start_time clock_gettime(CLOCK_MONOTONIC, st);

#define end_time \
    clock_gettime(CLOCK_MONOTONIC, et); \
    timer = (et->tv_sec - st->tv_sec) + 1e-9l * (et->tv_nsec - st->tv_nsec);

int get_termid(const std::string &term) {
    if (termIDmapping[term]) return termIDmapping[term];
    else return termIDmapping[term] = termsCount++;
}

std::map<int, std::map<int, std::vector<int>>> allData;

// writes all the pages seen so far into a file
void writeToFile() {
    allData.clear();

    for (int i = 0; i < curr; i++) {
        auto page = mem[i];
        int docID = docCount++;

        docDetails[docID] = page->title;

        int zone_i = -1;
        for (const auto &zone : page->terms) {
            zone_i++;
            for (const auto &term : zone) {
                int termID = get_termid(term);

                auto &freq = allData[termID][docID];
                if (freq.empty()) freq = std::vector<int>(ZONE_COUNT);

                freq[zone_i]++;
            }
        }
    }

    writeIndex(allData);
}

bool checkpoint() {
    end_time
    currCheck++;

    std::cout << "Exhausted reading " << curr << " records in time " << timer << std::endl;

    int LOG_POINT = 1000;

    start_time

    int sum = 0;

    for (int i = 0; i < curr; i++) {
        auto &page = mem[i];
        sum += page->text.size();
        extractData(page);

        if ((i + 1) % LOG_POINT == 0) {
            end_time
            std::cout << "Read " << LOG_POINT << " records with total length " << sum << " in time " << timer
                      << std::endl;
            sum = 0;
            start_time
        }
    }

    end_time

    start_time
    writeToFile();
    end_time
    std::cout << "Written in time " << timer << std::endl;

    for (int i = 0; i < curr; i++) {
        delete mem[i];
    }

    curr = 0;

    if (currCheck == MAX_CHECK) {
        return false;
    }

    start_time;

    return true;
}


class WikiObject {
public:
    WikiObject(xml::parser &p) {
        p.next_expect(xml::parser::start_element, NS, "mediawiki", xml::content::complex);

        auto wsi = new WikiSiteInfo(p);
        bool interrupted = false;

        start_time

        while (p.peek() == xml::parser::start_element) {
            auto page = new WikiPage(p);
            mem[curr++] = page;

            if (curr == MX_MEM) {
                if (not checkpoint()) {
                    interrupted = true;
                    break;
                }
            }
        }

        if (not interrupted) p.next_expect(xml::parser::end_element, NS, "mediawiki");

        delete wsi;
    }
};


int main() {
    try {
        start_time

        processor = new Preprocessor();
        std::ifstream ifs(filePath);
        // our xml is in the namespace denoted by the xmlns attribute in the XML file
        // we don't want attributes or namespace declarations
        xml::parser p(ifs, filePath, xml::parser::receive_characters | xml::parser::receive_elements);

        end_time

        std::cout << "File read and parser readied in time " << timer << std::endl;

        // if you put the receive_namespace_decls flag in the parser argument, it will start receiving
        // namespace decls also, which may be desirable, but for now, skip it and hardcode the namespace

        auto wo = new WikiObject(p);

        checkpoint();

        ifs.close();
        delete wo;
        delete processor;

        start_time
        writeTermMapping(termIDmapping);
        writeDocMapping(docDetails);
        end_time
        std::cout << "Written terms and docs in time " << timer << std::endl;
    } catch (xml::parsing &e) {
        std::cout << e.what() << std::endl;
        return 1;
    }

    return 0;
}
