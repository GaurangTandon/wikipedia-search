cmake_minimum_required(VERSION 3.17)
project(wikipedia_search)

set(CMAKE_CXX_STANDARD 14)

include_directories(parser)
include_directories(rapidxml)

add_executable(wikipedia_search
        parser/parser.cpp)
