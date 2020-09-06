# bash index.sh <path_to_wiki_dump> <path_to_invertedindex_output> <invertedindex_stat.txt>

rm -rf $2
rm -rf $3
mkdir $2
dist/indexer.sh $1 $2 $3
dist/merger.sh $2 $3
# rm -r output/i* # remove old index files
