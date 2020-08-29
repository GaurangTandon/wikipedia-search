# built thanks to https://stackoverflow.com/a/20830354

EXEDIR = dist
CXXFLAGS = -pthread

debug: CXXFLAGS += -g -O0
run: CXXFLAGS += -O3

all: prep runner searcher postcomplete

searcher: search/search.cpp
	$(CXX) $(CXXFLAGS) search/search.cpp -o $(EXEDIR)/search.sh

runner: 
	$(CXX) $(CXXFLAGS) parser/parser.cpp -o $(EXEDIR)/index.sh

prep:
	@mkdir -p $(EXEDIR)

remake: clean all

# required as per assignment
postcomplete:
	ln -s $(EXEDIR)/search.sh search.sh
	ln -s $(EXEDIR)/index.sh index.sh

clean:
	rm -r $(EXEDIR)
	
