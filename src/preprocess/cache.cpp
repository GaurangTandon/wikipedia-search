// https://deepakvadgama.com/blog/lfu-cache-in-O%281%29/ based on this blog; although not sure how
// useful caching would be while stemming
#include <map>
#include <string>
#include <unordered_map>

class Node {
    int count;
};

// Doubly Linked list
class DLL {

};

class Cache {
    int MAX_SIZE;
    std::unordered_map<std::string, std::string> values;

    Cache(int size) : MAX_SIZE(size) {}

    std::string get(std::string key) {
        auto &val = values[key];
        return val;
    }

    std::string add(std::string key) {

    }
};