# built thanks to https://stackoverflow.com/a/20830354

LIBSTUDXML_EXE = /usr/local/lib/libstudxml.a
LIBSTEM = libstemmer_c
BZIP_DIR = bzip2
EXEDIR = dist

STEM_EXE = $(LIBSTEM)/libstemmer.o
BZIP_EXE = $(BZIP_DIR)/libbz2.a
CXXFLAGS = -pthread -O0 -g -fsanitize=address -fsanitize=undefined

debug: prep runner searcher
all: prep runner searcher

searcher:
	$(CXX) search/search.cpp $(BZIP_EXE) $(STEM_EXE) $(CXXFLAGS) -o $(EXEDIR)/search.sh

runner: 
	$(CXX) parser/parser.cpp $(BZIP_EXE) $(LIBSTUDXML_EXE) $(STEM_EXE) $(CXXFLAGS) -o $(EXEDIR)/index.sh

prep:
	@mkdir -p $(EXEDIR)
	$(MAKE) -C $(LIBSTEM)
	$(MAKE) -C $(BZIP_DIR)

clean:
	rm -fr $(EXEDIR)
	
clean_full: clean
	$(MAKE) -C $(LIBSTEM) clean
	$(MAKE) -C $(BZIP_DIR) clean

remake: clean all

