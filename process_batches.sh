#!/bin/bash

# Get total number of lines in the input file
total_lines=$(wc -l < Histograms_to_be_combined_Full.txt)

# Split into batches of 500 lines
batch_size=500
count=0
start=1

while [ $start -le $total_lines ]; do
    end=$((start + batch_size - 1))
    if [ $end -gt $total_lines ]; then
        end=$total_lines
    fi
    sed -n "${start},${end}p" Histograms_to_be_combined_Full.txt > "filelist_${start}_${end}.txt"
    echo "Created filelist_${start}_${end}.txt"
    start=$((start + batch_size))
    count=$((count + 1))
done

echo "Created $count batch files"