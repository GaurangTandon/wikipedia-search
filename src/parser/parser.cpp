#include <xml/parser>
#include<string>
#include<iostream>
#include <ctime>
#include<vector>
#include "../preprocess/preprocess.cpp"
#include <pthread.h>
#include <queue>
#include "../headers/parsing_common.h"
#include "../file_handling/filehandler.cpp"

/*
 * next_expect(): if `content` argument is set, then event type must be start element
 */

const std::string NS = "http://www.mediawiki.org/xml/export-0.10/";
std::string filePath;
int totalTokenCount = 0;
int totalDocCount = 0;
std::ofstream docTitlesOutput;

struct timespec *st = new timespec(), *et = new timespec();
int currCheck = 0;

constexpr int MX_THREADS = 200;
pthread_t threads[MX_THREADS];
int threadCount = 0;

constexpr int MX_MEM = 2000;
memory_type *globalMemory;

void allocate_mem() {
    globalMemory = (memory_type *) malloc(sizeof(memory_type));
    globalMemory->store = (WikiPage **) malloc(sizeof(WikiPage *) * MX_MEM);
    globalMemory->size = 0;
    globalMemory->checkpoint_num = currCheck;
    globalMemory->alldata = new data_type();
}

WikiPage::WikiPage(xml::parser &p) {
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

// KEEP lowercase
const std::vector<std::string> TEXT_INFOBOX = {"{{infobox", "{{ infobox"};
const std::vector<std::string> TEXT_CATEGORY = {"[[category", "[[ category"};
const std::vector<std::string> TEXT_EXTERNAL_LINKS = {"== external links ==", "==external links==",
                                                      "== external links==", "==external links =="};
const std::vector<std::string> TEXT_REFERENCES = {"== references ==", "==references==", "== references==",
                                                  "==references =="};
Preprocessor *processor;

inline void
processText(data_type &all_data, const int docid, const int zone, const std::string &text, int start, int end) {
    totalTokenCount += processor->processText(all_data, docid, zone, text, start, end);
}

int extractInfobox(const std::string &text, const int start) {
    int cnt = 0;
    int end = start;

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

    // TODO: remove this check later
    if (cnt != 0) {
//        std::cout << page->title << std::endl;
        std::cout << cnt << std::endl;
        exit(1);
    }

    return end;
}

int extractCategory(const std::string &text, const int start) {
    int end = start;

    while (end < text.size() - 1) {
        if (text[end] == ']' and text[end + 1] == ']') {
            end++;
            break;
        }
        end++;
    }

    return end;
}

int extractExternalLinks(const std::string &text, const int start) {
    int end = start;

    // assume external links are followed by categorical information
    while (end < text.size() - 1 and not Preprocessor::fast_equals(text, TEXT_CATEGORY, end + 1)) {
        end++;
    }

    return end;
}

int extractReferences(const std::string &text, int start) {
    int end = start;

    // assume external links are followed by categorical information
    while (end < text.size() - 1 and not Preprocessor::fast_equals(text, TEXT_EXTERNAL_LINKS, end + 1) and
           not Preprocessor::fast_equals(text, TEXT_CATEGORY, end + 1)) {
        end++;
    }

    return end;
}

void extractData(memory_type *mem, WikiPage *page) {
    auto &text = page->text;
    auto docid = page->docid;
    auto &all_data = *mem->alldata;
    std::string bodyText;

    for (auto &c : text) c = Preprocessor::lowercase(c);
    for (auto &c : page->title) c = Preprocessor::lowercase(c);

    for (int i = 0; i < text.size(); i++) {
        int end = -1;
        int start = i;
        int zone = -1;

        if (Preprocessor::fast_equals(text, TEXT_INFOBOX, i)) {
            start += TEXT_INFOBOX.size();
            zone = INFOBOX_ZONE;
            end = extractInfobox(text, i);
        } else if (Preprocessor::fast_equals(text, TEXT_CATEGORY, i)) {
            start += TEXT_CATEGORY.size();
            zone = CATEGORY_ZONE;
            end = extractCategory(text, i);
        } else if (Preprocessor::fast_equals(text, TEXT_EXTERNAL_LINKS, i)) {
            start += TEXT_EXTERNAL_LINKS.size();
            zone = EXTERNAL_LINKS_ZONE;
            end = extractExternalLinks(text, i);
        } else if (Preprocessor::fast_equals(text, TEXT_REFERENCES, i)) {
            start += TEXT_REFERENCES.size();
            zone = REFERENCES_ZONE;
            end = extractReferences(text, i);
        } else {
            bodyText += text[i];
        }

        if (zone != -1) {
            processText(all_data, docid, zone, text, start, end);
            i = end;
        }
    }

    processText(all_data, docid, TEXT_ZONE, bodyText, 0, bodyText.size() - 1);
    processText(all_data, docid, TITLE_ZONE, page->title, 0, page->title.size() - 1);
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

// writes all the pages seen so far into a file
inline void writeToFile(memory_type *mem) {
    long double timer;
    auto *st = new timespec(), *et = new timespec();

    start_time

    writeIndex(mem->alldata, mem->checkpoint_num);

    end_time
    std::cout << "Written in time " << timer << '\n';
    delete st;
    delete et;
}

inline int get_docid(int threadNum, int docIdx) {
    return threadNum * MX_MEM + docIdx;
}

void *thread_checkpoint(void *arg) {
    auto mem = (memory_type *) arg;
    long double timer;
    auto *st = new timespec(), *et = new timespec();

    start_time

    int sum = 0;
    for (int i = 0; i < mem->size; i++) {
        auto &page = mem->store[i];
        page->docid = get_docid(mem->checkpoint_num, i);
        sum += page->text.size();
        extractData(mem, page);
    }

    end_time

    std::cout << "Stemmed " << mem->size << " records with total length " << sum << " in time " << timer << '\n';

    writeToFile(mem);

    for (int i = 0; i < mem->size; i++) {
        delete mem->store[i];
    }

    delete mem->alldata;
    free(mem->store);
    free(mem);
    delete st;
    delete et;

    return nullptr;
}

void checkpoint() {
    currCheck++;

    int size = globalMemory->size;

    // needs to be done sequentially
    for (int i = 0; i < size; i++) {
        auto &page = globalMemory->store[i];
        docTitlesOutput << page->title << '\n';
    }
    totalDocCount += size;

    pthread_create(&threads[threadCount], nullptr, thread_checkpoint, globalMemory);
    threadCount++;

    allocate_mem();
}

class WikiObject {

public:
    explicit WikiObject(xml::parser &p) {
        p.next_expect(xml::parser::start_element, NS, "mediawiki", xml::content::complex);

        auto wsi = new WikiSiteInfo(p);
        delete wsi;

        start_time

        while (p.peek() == xml::parser::start_element) {
            auto page = new WikiPage(p);
            globalMemory->store[globalMemory->size++] = page;

            if (globalMemory->size == MX_MEM) {
                checkpoint();
            }
        }

        p.next_expect(xml::parser::end_element, NS, "mediawiki");
    }
};

int main(int argc, char *argv[]) {
    auto *stt = new timespec(), *ett = new timespec();
    clock_gettime(CLOCK_MONOTONIC, stt);

    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);
    std::cout.tie(nullptr);

    std::string statFile;
    assert(argc == 4);
    filePath = std::string(argv[1]);
    setOutputDir(std::string(argv[2]));
    statFile = std::string(argv[3]);

    docTitlesOutput.open(outputDir + "docs");

    try {
        processor = new Preprocessor();
        std::ifstream ifs(filePath);
        // our xml is in the namespace denoted by the xmlns attribute in the XML file
        // we don't want attributes or namespace declarations

        // if you put the receive_namespace_decls flag in the parser argument, it will start receiving
        // namespace decls also, which may be desirable, but for now, skip it and hardcode the namespace
        xml::parser p(ifs, filePath, xml::parser::receive_characters | xml::parser::receive_elements);

        allocate_mem();

        std::cout << "Starting parsing" << std::endl;

        auto wo = new WikiObject(p);

        checkpoint();

        std::cout << "Finished parsing the xml" << std::endl;

        for (int thread = 0; thread < threadCount; thread++) {
            pthread_join(threads[thread], nullptr);
        }

        std::ofstream stats(statFile, std::ios_base::out);
        stats << totalTokenCount << '\n';
        stats.close();

        std::ofstream file_stats(outputDir + "stat.txt", std::ios_base::out);
        file_stats << currCheck << '\n';
        file_stats << totalDocCount << '\n';
        file_stats.close();

        typedef std::pair<int, std::string> data_here;
        std::priority_queue<data_here, std::vector<data_here>, std::greater<>> pq;
        constexpr int THRESH = 1000;

        for (auto &e : freq) {
            pq.push({e.second, e.first});
            if (pq.size() > THRESH) pq.pop();
        }
        std::cout << pq.size() << " " << freq.size() << std::endl;

        while (not pq.empty()) {
            auto &e = pq.top();
            std::cout << e.first << " " << e.second << std::endl;
            pq.pop();
        }

        ifs.close();
        delete wo;
        delete processor;
        free(globalMemory->store);
        delete globalMemory->alldata;
        free(globalMemory);
    } catch (xml::parsing &e) {
        std::cout << e.what() << '\n';
        return 1;
    }

    clock_gettime(CLOCK_MONOTONIC, ett);
    long double global_time = calc_time(stt, ett);
    std::cout << "Total time taken " << global_time << std::endl;
    delete stt;
    delete ett;

    return 0;
}
