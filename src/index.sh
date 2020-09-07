#!/bin/bash

# bash index.sh <path_to_invertedindex_output> <invertedindex_stat.txt>

# rm -rf $1
# rm -rf $2
# mkdir $1
# 
# dist/indexer.sh $1 $2

if [[ $? -gt 0 ]] 
then
    exit 1
fi

# dist/merger.sh $1 $2

if [[ $? -gt 0 ]] 
then
    exit 2
fi

# rm -r output/i* # remove old index files
