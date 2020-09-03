#include "../headers/common.h"
#include "zip_operations.cpp"
#include <iostream>

int main() {
    auto stt = new timespec(), ett = new timespec();
    clock_gettime(CLOCK_MONOTONIC_RAW, stt);
    long double timer;

    int count = 40; // ADJUST THIS PARAMETER

    for (int i = 0; i < count; i++) {
        auto st = new timespec(), et = new timespec();
        start_time
        auto buffer = ReadBuffer("output/index" + std::to_string(i));
        int charsRead = 0;

        while (buffer.readChar()) charsRead++;
        end_time

        std::cout << "Time to read " << charsRead << " chars: " << timer << std::endl;
    }
    clock_gettime(CLOCK_MONOTONIC_RAW, ett);
    timer = calc_time(stt, ett);
    std::cout << "Time to read all files: " << timer << std::endl;
    return 0;
}