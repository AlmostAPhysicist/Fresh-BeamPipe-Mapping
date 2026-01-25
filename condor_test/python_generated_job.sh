#!/bin/bash

echo "Job started on $(hostname)"
echo "Process ID: $1"
echo "Cluster ID: $2"
echo "Proc ID: $3"

INPUT_FILE="/eos/user/a/amalhotr/condor_test/input.txt"
OUTPUT_FILE="/eos/user/a/amalhotr/condor_test/output.$2.$3.txt"

echo "Reading input file..."
INPUT_CONTENT=$(cat "$INPUT_FILE")

echo "Writing output file..."
echo "Input was:" > "$OUTPUT_FILE"
echo "$INPUT_CONTENT" >> "$OUTPUT_FILE"
echo "Processed by job $1" >> "$OUTPUT_FILE"

sleep 30

echo "Job finished successfully"
