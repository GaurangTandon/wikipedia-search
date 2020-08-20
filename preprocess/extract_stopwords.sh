#!/bin/bash

# use ripgrep to print the first matching word of each line without line number
rg "^\s*([\w']+)" stopwords_with_comments.txt -N -o > stopwords.txt
