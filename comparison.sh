#!/bin/bash

# Check if the correct number of arguments is provided
if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <file1> <file2>"
    exit 1
fi

# Assign the file paths provided as arguments
file1="$1"
file2="$2"

# Get information about the first file
file1_info=$(stat -c "%Y %i %s %A" "$file1")

# Get information about the second file
file2_info=$(stat -c "%Y %i %s %A" "$file2")

# Compare the attributes of the two files
if [ "$file1_info" == "$file2_info" ]; then
    exit 0  # no difference found
else
    exit 1  # difference found
fi

