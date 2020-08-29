# built thanks to https://stackoverflow.com/a/20830354

LIBSTUDXML_EXE = /usr/local/lib/libstudxml.a
LIBSTEM = libstemmer_c
EXEDIR = dist

STEM_EXE = $(LIBSTEM)/libstemmer.o
CXXFLAGS = -pthread

all: CXXFLAGS += -O3
debug: CXXFLAGS += -g -O0

debug: prep runner searcher postcomplete
all: prep runner searcher postcomplete

searcher: search/search.cpp
	$(CXX) $(CXXFLAGS) $(STEM_EXE) search/search.cpp -o $(EXEDIR)/search.sh

runner: parser/parser.cpp
	$(CXX) $(CXXFLAGS) $(LIBSTUDXML_EXE) $(STEM_EXE) parser/parser.cpp -o $(EXEDIR)/index.sh

prep:
	@mkdir -p $(EXEDIR)
	$(MAKE) -C $(LIBSTEM)

remake: clean all

# required as per assignment
postcomplete:
	ln -s $(EXEDIR)/search.sh search.sh
	ln -s $(EXEDIR)/index.sh index.sh

clean:
	rm -r $(EXEDIR)
	$(MAKE) -C $(LIBSTEM) clean
	
