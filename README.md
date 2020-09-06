# wikipedia-search

Blazingly fast search for wikipedia, faster than google, deployed on aws

## Build

1. Install libstudxml from [official source code](https://www.codesynthesis.com/projects/libstudxml/) (this project uses version 1.0.1)
2. Locate the file `libstudxml.a` from previous step, it's most likely present in `/usr/local/lib`. If not, you can check the output of the previous installation and locate the directory. Once done, change its path in Makefile variable `$(LIBSTUDXML_EXE)`
3. Remaining dependencies are directly installed via the Makefile. So, if you already have any of them installed, feel free to edit the Makefile accordingly.

  - `make`: compiles bzip2, stemmer, indexer and searcher.
  - `make clean`: to clean all binaries from this project.
  - `make clean_full`: (DANGER) will clean all dependencies' binaries as well.

## Running

Use src/index.sh and src/search.sh. Usage is in file comments.

## How to search

These categories are currently supported: Title, Infobox, Body, Category, Links, and References.
To refer to these categories, use their 1st character in lower case. So, the following query: `"t:egypt i:nile"` searches for pages with `egypt` in their title and `nile` in their infobox. 

If you do not specify a category, it is assumed to be the body by default.

1. `i:egypt`
2. `i:sachin`
3. `people`
4. `t:Arabic`
5. `c:cricket`
6. `e:anarchism`

## Stats

On the 680MB dump, this takes roughly 60secs to do the complete indexing, and around 6secs per search query. The complete inverted index size is roughly 67M.

## Information

Document id starts from zero. Every thread gets assigned `[BLOCK * tI, (BLOCK + 1) * tI]` document ids, where tI is the thread index (starting from zero).

## Dependencies

1. `bzip2`: v1.0.8 ([link](https://www.sourceware.org/bzip2))
2. `libstemmer_c`: v2.0.0 ([link](https://snowballstem.org))
3. `libstudxml`: v1.0.1 ([link](https://www.codesynthesis.com/projects/libstudxml/))
