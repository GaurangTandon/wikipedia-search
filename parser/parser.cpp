#include <xml/parser>
#include<string>
#include<iostream>
#include<fstream>

/*
 * next_expect(): if `content` argument is set, then event type must be start element
 */

class WikiPage {
public:
    std::string title, text;

    WikiPage(xml::parser &p) {
        p.next_expect(xml::parser::start_element, "page");

        bool inTitle = false, inText = false;

        for (auto e : p) {
            switch (e) {
               case xml::parser::start_element:
                    if (p.name() == "title") {
                        inTitle = true;
                    }
                    if (p.name() == "title") {
                        inText = true;
                    }
                    break;
                case xml::parser::end_element:
                    if (p.name() == "title") {
                        inTitle = false;
                    }
                    if (p.name() == "title") {
                        inText = false;
                    }
                    if (p.name() == "page") {
                        std::cout << title << " " << text << std::endl;
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
                case xml::parser::start_attribute:
                case xml::parser::end_attribute:
                    break;
            }
        }
    }
};

class WikiSiteInfo {
public:
    WikiSiteInfo (xml::parser &p) {
        p.next_expect(xml::parser::start_element, "siteinfo", xml::content::complex);

        while (p.peek() == xml::parser::start_element) {
            p.next_expect(xml::parser::start_element);
            p.next_expect(xml::parser::characters);
            p.next_expect(xml::parser::end_element);
        }

        p.next_expect(xml::parser::end_element, "siteinfo");
    }
};

class WikiObject {
public:
    WikiObject(xml::parser &p) {
        p.next_expect(xml::parser::start_element, "mediawiki", xml::content::complex);

        new WikiSiteInfo(p);

        while (p.peek() == xml::parser::start_element) new WikiPage(p);

        p.next_expect(xml::parser::end_element, "mediawiki");
    }
};

const std::string filePath = "small.xml";

int main() {
    try {
        std::ifstream ifs(filePath);
        xml::parser p(ifs, filePath);
        new WikiObject(p);
    } catch (xml::parsing &e) {
        std::cout << e.what() << std::endl;
        return 1;
    }
    return 0;
}
