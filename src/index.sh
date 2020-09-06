# bash index.sh <path_to_wiki_dump> <path_to_invertedindex_output> <invertedindex_stat.txt>

rm -rf $1
rm -rf $2
mkdir $1
dist/indexer.sh $1 $2
dist/merger.sh $1 $2
rm -r output/i* # remove old index files
