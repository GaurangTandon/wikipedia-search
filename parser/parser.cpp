#include <xml/parser>
#include<string>
#include<iostream>
#include<fstream>

/*
 * next_expect(): if `content` argument is set, then event type must be start element
 */

const std::string NS = "http://www.mediawiki.org/xml/export-0.10/";

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
                    if (p.name() == "title") {
                        inText = true;
                    }
        p.attribute_map();
                    break;
                case xml::parser::end_element:
                    if (p.name() == "title") {
                        inTitle = false;
                    }
                    if (p.name() == "text") {
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
                default:
                    break;
            }
        }
    }
};

#define DEBUG std::cout << p.next() << " " << p.value() << " " << p.name() << " " << p.qname() << std::endl;

class WikiSiteInfo {
public:
    WikiSiteInfo (xml::parser &p) {
        p.next_expect(xml::parser::start_element, NS, "siteinfo", xml::content::complex);

        for (auto e : p) {
            if (e == xml::parser::start_element) {
                p.attribute_map();
            }
            if (e == xml::parser::end_element) {
                if (p.name() == "siteinfo") return;
            }
        }

        assert(false);
    }
};


class WikiObject {
public:
    WikiObject(xml::parser &p) {
        p.next_expect(xml::parser::start_element, NS, "mediawiki", xml::content::complex);
        p.attribute_map();

        new WikiSiteInfo(p);

        while (p.peek() == xml::parser::start_element) new WikiPage(p);

        p.next_expect(xml::parser::end_element, NS, "mediawiki");
    }
};

const std::string filePath = "small.xml";

int main() {
//    try {
        std::ifstream ifs(filePath);
        // our xml is in the namespace denoted by the xmlns attribute in the XML file
        xml::parser p(ifs, filePath);
        // if you put the receive_namespace_decls flag in the parser argument, it will start receiving
        // namespace decls also, which may be desirable, but for now, skip it and hardcode the namespace
        new WikiObject(p);
//    } catch (xml::parsing &e) {
//        std::cout << e.what() << std::endl;
//        return 1;
//    }
    return 0;
}
