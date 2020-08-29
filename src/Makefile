# built thanks to https://stackoverflow.com/a/20830354

LIBSTUDXML_EXE = /usr/local/lib/libstudxml.a
LIBSTEM = libstemmer_c
EXEDIR = dist

STEM_EXE = $(LIBSTEM)/libstemmer.o
CXXFLAGS = -pthread -O3

debug: prep runner searcher
all: prep runner searcher

searcher: $(EXEDIR)/search.sh
	$(CXX) search/search.cpp $(STEM_EXE) $(CXXFLAGS) -o $(EXEDIR)/search.sh

runner: $(EXEDIR)/index.sh
	$(CXX) parser/parser.cpp $(LIBSTUDXML_EXE) $(STEM_EXE) $(CXXFLAGS) -o $(EXEDIR)/index.sh

prep:
	@mkdir -p $(EXEDIR)
	$(MAKE) -C $(LIBSTEM)

clean:
	rm -fr $(EXEDIR)
	
clean_full: clean
	$(MAKE) -C $(LIBSTEM) clean

remake: clean all

