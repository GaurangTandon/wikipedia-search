cmake_minimum_required(VERSION 3.17)
project(wikipedia_search)

set(CMAKE_CXX_STANDARD 14)

set(GCC_COVERAGE_COMPILE_FLAGS "-O3 -g -pthread")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}" )

include_directories(src/parser)
include_directories(src/libstemmer_c)
include_directories(src/bzip2)

SET(OBJS
    src/libstemmer_c/libstemmer.o
    src/bzip2/libbz2.a
    /usr/local/lib/libstudxml.a )

SET_SOURCE_FILES_PROPERTIES(
        ${OBJS}
        PROPERTIES
        EXTERNAL_OBJECT true
        GENERATED true
)

add_executable(wikipedia_search
        ${OBJS}
        src/parser/parser.cpp)

add_executable(searcher
        ${OBJS}
        src/search/search.cpp)
