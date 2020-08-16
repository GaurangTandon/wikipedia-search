#include <xml/parser>
#include<string>
#include<iostream>
#include<fstream>

class position {
public:
    double lat_, lon_;

    position(xml::parser &p) {
        p.next_expect(xml::parser::start_element, "position", xml::content::empty);
        lat_ = p.attribute<double>("lat");
        lon_ = p.attribute<double>("lon");
        std::cout << "Parsed position with " << lat_ << " " << lon_ << std::endl;
    }
};

class object {
public:
    std::string name, type;
    int id;

    object(xml::parser &p) {
        p.next_expect(xml::parser::start_element, "object", xml::content::complex);

        id = p.attribute<int>("id");
        name = p.attribute("name");
        type = p.attribute("type");

        std::cout << "Parsed " << id << " " << name << " " << type << std::endl;

        while (p.peek() == xml::parser::start_element) {
            new position(p);
        }

        p.next_expect(xml::parser::end_element);
    }
};

const std::string filePath = "text.xml";
int main() {
    try {
        std::ifstream ifs(filePath);
        xml::parser p(ifs, filePath);
        new object(p);
    } catch (xml::parsing &e) {
        std::cout << e.what() << std::endl;
        return 1;
    }
    return 0;
}
