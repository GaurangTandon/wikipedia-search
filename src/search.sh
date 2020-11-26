# Usage: <path to inverted index> <query in double quotes>

dist/search.sh "$1" "$(realpath $2)" "$(cat $2 | wc -l)"
