#!/bin/bash

# Check if the correct number of arguments is provided
if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <file>"
    exit 1
fi

# Assign the file path provided as an argument
file="$1"

# Count the number of lines, words, and characters in the file
lines=$(wc -l < "$file")
words=$(wc -w < "$file")
characters=$(wc -c < "$file")

# Output the counts in the format: "<lines> <words> <characters>"
echo "$lines $words $characters"

