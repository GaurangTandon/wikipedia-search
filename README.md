# wikipedia-search

Blazingly fast search for wikipedia, faster than google, deployed on aws

## Build

1. Install libstudxml from [official source code](https://www.codesynthesis.com/projects/libstudxml/) (this project uses version 1.0.1)
2. Locate the file `libstudxml.a` from previous step, it's most likely present in `/usr/local/lib`. If not, you can check the output of the previous installation and locate the directory. Once done, change its path in Makefile variable `$(LIBSTUDXML_EXE)`. 
3. Makefile:

  - `make clean`: to clean all binaries
  - `make`: compiles the stemmer, indexer and search binary.
  - After the make step, the exe are symlinked to the present working directory.

## Running



## Stats

On the 680MB dump, this takes roughly 50secs to do the complete indexing.
