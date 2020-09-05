#include <pthread.h>
#include <map>
#include <string>
std::map<std::string, int> termIDmapping;
int termsCount = 1;
std::map<int, std::string> docDetails;

pthread_mutex_t term_id_mutex = PTHREAD_MUTEX_INITIALIZER;

// MUST HOLD LOCK WHEN CALLING
inline int add_term(const std::string &term) {
    if (not termIDmapping[term]) termIDmapping[term] = termsCount++;
    return termIDmapping[term];
}
