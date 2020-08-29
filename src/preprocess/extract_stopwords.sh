#!/bin/bash

# use ripgrep to print the first matching word of each line without line number
rg "^\s*([\w']+)" stopwords_with_comments.txt -N -o > stopwords.txt

# prepend number of lines :/ https://superuser.com/questions/246837
echo -e "$(cat stopwords.txt | wc -l)\n$(cat stopwords.txt)" > stopwords.txt
