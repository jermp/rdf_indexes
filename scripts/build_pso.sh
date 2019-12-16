#!/usr/bin/bash
dataset=$1
../build/build_permutation compact 5 ../test_data/$1.mapped.sorted
../build/build_permutation ef 5 ../test_data/$1.mapped.sorted
../build/build_permutation pef 5 ../test_data/$1.mapped.sorted
../build/build_permutation vb 5 ../test_data/$1.mapped.sorted
