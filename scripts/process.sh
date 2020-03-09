#!/bin/bash

dataset=$1 # should be in 'nt' format and gzipped, e.g., foo.gz
basename="${dataset%.*}"
echo "processing dataset '$dataset'"
echo "basename '$basename'"
python3 extract_vocabs.py $dataset -S -P -O
python3 map_dataset.py $dataset
python3 sort.py $basename.mapped.unsorted $basename
python3 build_stats.py $basename.mapped.sorted