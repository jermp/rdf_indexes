#!/bin/bash

dataset=$1 # should be in 'nt' format and gzipped, e.g., foo.gz
basename="${dataset%.*}"
echo "processing dataset '$dataset'"
echo "basename '$basename'"
python extract_vocabs.py $dataset -S -P -O
python map_dataset.py $dataset
python sort.py $basename.mapped.unsorted $basename
python build_stats.py $basename.mapped.sorted