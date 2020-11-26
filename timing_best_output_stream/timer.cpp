#include <sstream>
#include <iostream>
#include <string>
#include <time.h>

using namespace std;

constexpr int T = 1000000;

void method1() {
    stringstream ss;

    for (int i = 0; i < T; i++) {
        ss << i << " ";
    }
    ss << '\n';

    cout << ss.rdbuf();
}

void method2() {
    stringstream ss;

    for (int i = 0; i < T; i++) {
        ss << i << " ";
    }
    ss << '\n';

    cout << ss.str();
}

void method3() {
    for (int i = 0; i < T; i++) {
        cout << i << " ";
    }
    cout << '\n';
}

void method4() {
    string data;
    for (int i = 0; i < T; i++) {
        data += to_string(i) + " ";
    }
    data += '\n';
    cout << data;
}

struct timespec *st = new timespec(), *et = new timespec();
long double timer;
#define start clock_gettime(CLOCK_MONOTONIC_RAW, st);
#define end clock_gettime(CLOCK_MONOTONIC_RAW, et); \
    timer = (et->tv_sec - st->tv_sec) + 1e-9l * (et->tv_nsec - st->tv_nsec);

int main() {
    string data;
    start;
    method1();
    end;
    data += to_string(timer) + "\n";

    start;
    method2();
    end;
    data += to_string(timer) + "\n";

    start;
    method3();
    end;
    data += to_string(timer) + "\n";

    start;
    method4();
    end;
    data += to_string(timer) + "\n";

    cout << data << endl;

    return 0;
}
