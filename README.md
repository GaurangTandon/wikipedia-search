# wikipedia-search

Blazingly fast search for wikipedia, faster than google, deployed on aws

## Build

1. Install libstudxml from [official source code](https://www.codesynthesis.com/projects/libstudxml/) (this project uses version 1.0.1)
2. Locate the file `libstudxml.a` from previous step, it's most likely present in `/usr/local/lib`. If not, you can check the output of the previous installation and locate the directory. Once done, change its path in Makefile variable `$(LIBSTUDXML_EXE)`. 
3. Makefile:

  - `make clean`: to clean all binaries
  - `make`: compiles the stemmer, indexer and search binary.

## Running

Use src/index.sh and src/search.sh. Usage is in file comments.

## Useful search queries

1. `i:egypt`
2. `i:sachin`
3. `people`
4. `t:Arabic`
5. `c:cricket`
6. `e:anarchism`

## Stats

On the 680MB dump, this takes roughly 50secs to do the complete indexing.
