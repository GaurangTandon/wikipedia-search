# built thanks to https://stackoverflow.com/a/20830354

LIBSTUDXML_EXE = /usr/local/lib/libstudxml.a
LIBSTEM = libstemmer_c
EXEDIR = dist

STEM_EXE = $(LIBSTEM)/libstemmer.o
CXXFLAGS = -pthread -O3 # -O0 -g -fsanitize=address -fsanitize=undefined

debug: prep indexer searcher
all: prep indexer searcher

searcher:
	$(CXX) search/search.cpp $(STEM_EXE) $(CXXFLAGS) -o $(EXEDIR)/search.sh

indexer: 
	$(CXX) parser/parser.cpp $(LIBSTUDXML_EXE) $(STEM_EXE) $(CXXFLAGS) -o $(EXEDIR)/indexer.sh
	$(CXX) file_handling/mergeIndex.cpp $(CXXFLAGS) -o $(EXEDIR)/merger.sh

prep:
	mkdir -p $(EXEDIR)
	$(MAKE) -C $(LIBSTEM)

clean:
	rm -fr $(EXEDIR)
	
clean_full: clean
	$(MAKE) -C $(LIBSTEM) clean

remake: clean all
