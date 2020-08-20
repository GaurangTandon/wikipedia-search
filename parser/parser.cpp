#include <xml/parser>
#include<string>
#include<iostream>
#include<fstream>
#include <time.h>
#include<vector>
#include "../preprocess/preprocess.cpp"

/*
 * next_expect(): if `content` argument is set, then event type must be start element
 */

#define DEBUG std::cout << p.next() << " " << p.value() << " " << p.name() << " " << p.qname() << std::endl;

const std::string NS = "http://www.mediawiki.org/xml/export-0.10/";
const std::string filePath = "parser/large.xml";

struct timespec *st = new timespec(), *et = new timespec();
constexpr int MAX_CHECK = 5;
int curr_check = 0;

class WikiPage;

constexpr int MX_MEM = 5000;
WikiPage *mem[MX_MEM];
int curr = 0;

class WikiPage {
public:
    std::string title, text;
    std::vector<std::string> infobox, bodyText, category;

    WikiPage(xml::parser &p) {
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
Preprocessor* processor;

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

    page->infobox = processor->processText(infobox);

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
    page->category.insert(page->category.end(), tokens.begin(), tokens.end());

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

    page->bodyText = processor->processText(bodyText);
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

bool checkpoint() {
    end_time
    curr_check++;

    std::cout << "Exhausted reading " << curr << " records in time " << timer << std::endl;

    int LOG_POINT = 1000;

    start_time

    int sum = 0;

    for (int i = 0; i < curr; i++) {
        auto &page = mem[i];
        sum += page->text.size();
        extractData(page);

        if ((i + 1) % LOG_POINT == 0) {
            // use this to print
            // std::cout << page->title << std::endl;
            // for (auto word : page->bodyText) {
            //     std::cout << word << std::endl;
            // }

            end_time
            std::cout << "Read " << LOG_POINT << " records with total length " << sum << " in time " << timer << std::endl;
            sum = 0;
            start_time
        }
    }

    end_time

    for (int i = 0; i < curr; i++) {
        delete mem[i];
    }

    curr = 0;

    if (curr_check == MAX_CHECK) {
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

        delete wo;
        delete processor;
    } catch (xml::parsing &e) {
        std::cout << e.what() << std::endl;
        return 1;
    }

    return 0;
}
