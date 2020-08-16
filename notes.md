1. Dump seems to be English only. How to download? https://en.wikipedia.org/wiki/Wikipedia:Database_download#Offline_Wikipedia_readers and https://dumps.wikimedia.org/ 
2. Porter2stemmer in C: https://snowballstem.org/download.html; C++ alternative: https://github.com/smassung/porter2_stemmer; Multilingual stemmer: https://github.com/OleanderSoftware/OleanderStemmingLibrary
3. Interesting alternatives to inverted index: Okapi BM25
4. Previous year papers:

https://github.com/ayushidalmia/Wikipedia-Search-Engine BEST
https://github.com/dhruvkhattar/Wikipedia-Search-Engine
https://github.com/mitesh1612/Wiki-Search-Engine
https://github.com/adityat77/Wiki-Search-Engine
https://github.com/darshank15/wikipedia-search-engine
https://github.com/VatsalSoni301/Wikipedia_Search_Engine

5. XML parser of choice: rapidxml. manual: http://rapidxml.sourceforge.net/manual.html. close second: https://pugixml.org/docs/quickstart.html
6. For tokenization, use: https://github.com/OpenNMT/Tokenizer
7. c++ NLP libraries list: https://www.linuxlinks.com/excellent-c-plus-plus-natural-language-processing-tools/
8. can also do multi-threading, see ayushidalmia stuff

## parsing

Is of two types: DOM and SAX. DOM parser first builds the complete in-memory representation of the XML file, and then allows you to iterate through its nodes. This consumes a lot of memory if the file is large. Examples: RapidXML, TinyXML, PugiXML.

SAX parser (Simple API for XML) is an event-driven online approach where handlers keep getting called as the parser is iterating through the file, like startElement, endElement, seenCharacter, etc. They consume much less memory. Examples: expat2

See: https://stackoverflow.com/questions/4684440, https://stackoverflow.com/questions/31858466, https://stackoverflow.com/questions/1006543/

Tutorial on libstudxml: https://www.codesynthesis.com/projects/libstudxml/doc/intro.xhtml#3
