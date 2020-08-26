#include <xml/parser>
#include<string>
#include<iostream>
#include<map>
#include <time.h>
#include<vector>
#include "../preprocess/preprocess.cpp"
#include <pthread.h>

#include "../file_handling/termDocIdHandler.cpp"

/*
 * next_expect(): if `content` argument is set, then event type must be start element
 */

#define DEBUG std::cout << p.next() << " " << p.value() << " " << p.name() << " " << p.qname() << '\n';

const std::string NS = "http://www.mediawiki.org/xml/export-0.10/";
const std::string filePath = "parser/official.xml";

// have text zone first, since most tokens occur in text only
enum {
    TEXT_ZONE, INFOBOX_ZONE, CATEGORY_ZONE, ZONE_COUNT
};

struct timespec *st = new timespec(), *et = new timespec();
int currCheck = 0;

constexpr int MX_THREADS = 100;
pthread_t threads[MX_THREADS];
int threadCount = 0;

class WikiPage;

constexpr int MX_MEM = 1000;

typedef struct memory_type {
    WikiPage **store = nullptr;
    int size = 0;
} memory_type;

memory_type *memory;

void allocate_mem() {
    memory = (memory_type *) malloc(sizeof(memory_type));
    memory->store = (WikiPage **) malloc(sizeof(WikiPage *) * MX_MEM);
    memory->size = 0;
}

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

    if (cnt != 0) {
        std::cout << page->title << std::endl;
        std::cout << cnt << std::endl;
        exit(1);
    }

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

#define calc_time(st, et) ((et->tv_sec - st->tv_sec) + 1e-9l * (et->tv_nsec - st->tv_nsec))

#define end_time \
    clock_gettime(CLOCK_MONOTONIC, et); \
    timer = calc_time(st, et);


// TODO: instead of doing this separately, do this directly while
// creating the tokens, instead of storing everything in a double-map first
// and then doing this

// writes all the pages seen so far into a file
void writeToFile(memory_type *mem) {
    long double timer;
    struct timespec *st = new timespec(), *et = new timespec();

    std::map<int, std::map<int, std::vector<int>>> allData;
    start_time
    allData.clear();

    for (int i = 0; i < mem->size; i++) {
        auto page = mem->store[i];

        int docID = get_docid();
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

    end_time
    std::cout << "Written in time " << timer << '\n';
}

void *thread_checkpoint(void *arg) {
    auto mem = (memory_type *) arg;
    long double timer;
    struct timespec *st = new timespec(), *et = new timespec();

    start_time

    int sum = 0;

    for (int i = 0; i < mem->size; i++) {
        auto &page = mem->store[i];
        sum += page->text.size();
        extractData(page);
    }

    end_time

    std::cout << "Stemmed " << mem->size << " records with total length " << sum << " in time " << timer << '\n';

    writeToFile(mem);

    for (int i = 0; i < mem->size; i++) {
        delete mem->store[i];
    }

    free(mem);

    return nullptr;
}

void checkpoint() {
    currCheck++;

    pthread_create(&threads[threadCount++], nullptr, thread_checkpoint, memory);

    allocate_mem();
}


class WikiObject {
public:
    WikiObject(xml::parser &p) {
        p.next_expect(xml::parser::start_element, NS, "mediawiki", xml::content::complex);

        auto wsi = new WikiSiteInfo(p);

        start_time

        while (p.peek() == xml::parser::start_element) {
            auto page = new WikiPage(p);
            memory->store[memory->size++] = page;

            if (memory->size == MX_MEM) {
                checkpoint();
            }
        }

        p.next_expect(xml::parser::end_element, NS, "mediawiki");

        delete wsi;
    }
};

int main(int argc, char *argv[]) {
    auto *stt = new timespec(), *ett = new timespec();
    clock_gettime(CLOCK_MONOTONIC, stt);

    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);
    std::cout.tie(nullptr);

    if (argc == 2) {
        char *dir = argv[1];

        setOutputDir(std::string(dir));
    }

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

        start_time
        writeTermMapping(termIDmapping);
        writeDocMapping(docDetails);
        end_time

        ifs.close();
        delete wo;
        delete processor;
        free(memory);

        std::cout << "Written terms and docs in time " << timer << '\n';
    } catch (xml::parsing &e) {
        std::cout << e.what() << '\n';
        return 1;
    }

    clock_gettime(CLOCK_MONOTONIC, ett);
    long double global_time = calc_time(stt, ett);
    std::cout << "Total time taken " << global_time << std::endl;

    return 0;
}
