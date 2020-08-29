# bash index.sh <path_to_wiki_dump> <path_to_invertedindex_output> <invertedindex_stat.txt>

rm -rf $2
mkdir $2
dist/index.sh $1 $2 $3
