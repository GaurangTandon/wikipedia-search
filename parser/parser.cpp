#include <xml/parser>
#include<string>
#include<iostream>
#include<fstream>
#include <time.h>

/*
 * next_expect(): if `content` argument is set, then event type must be start element
 */

const std::string NS = "http://www.mediawiki.org/xml/export-0.10/";
struct timespec *st = new timespec(), *et = new timespec();
constexpr int MAX_CHECK = 5;
int curr_check = 0;

class WikiPage;
constexpr int MX_MEM = 5000;
WikiPage* mem[MX_MEM];
int curr = 0;

class WikiPage {
public:
    std::string title, text;

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

#define DEBUG std::cout << p.next() << " " << p.value() << " " << p.name() << " " << p.qname() << std::endl;

class WikiSiteInfo {
public:
    WikiSiteInfo (xml::parser &p) {
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

#define start_time clock_gettime(CLOCK_MONOTONIC, st);

#define end_time(st, et) \
    clock_gettime(CLOCK_MONOTONIC, et); \
    long double time = (et->tv_sec - st->tv_sec) + 1e-9l * (et->tv_nsec - st->tv_nsec);

void checkpoint(bool is_last = false) {
    end_time(st, et);
    curr_check++;

    std::cout << "Exhausted reading " << curr << " records in time " << time << std::endl;
//    for (auto page : mem) {
//        if (not page) break;
//        std::cout << page->title << " " << page->text.size() << std::endl;
//    }
    for (int i = 0; i < curr; i++) {
        free(mem[i]);
    }
    curr = 0;

    if (curr_check == MAX_CHECK or is_last) exit(0);

    start_time;
}

class WikiObject {
public:
    WikiObject(xml::parser &p) {
        p.next_expect(xml::parser::start_element, NS, "mediawiki", xml::content::complex);

        new WikiSiteInfo(p);

        start_time
        while (p.peek() == xml::parser::start_element) {
            auto page = new WikiPage(p);
            mem[curr++] = page;

            if (curr == MX_MEM) {
                checkpoint();
            }
        }

        p.next_expect(xml::parser::end_element, NS, "mediawiki");
    }
};

const std::string filePath = "large.xml";

int main() {
    try {
        start_time
        std::ifstream ifs(filePath);
        // our xml is in the namespace denoted by the xmlns attribute in the XML file
        // we don't want attributes or namespace declarations
        xml::parser p(ifs, filePath, xml::parser::receive_characters | xml::parser::receive_elements);

        end_time(st, et);

        std::cout << "File read and parser readied in time " << time << std::endl;

        // if you put the receive_namespace_decls flag in the parser argument, it will start receiving
        // namespace decls also, which may be desirable, but for now, skip it and hardcode the namespace
        new WikiObject(p);

        checkpoint(true);
    } catch (xml::parsing &e) {
        std::cout << e.what() << std::endl;
        return 1;
    }

    return 0;
}
