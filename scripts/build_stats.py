import sys

collection_basename = sys.argv[1]
output_file = open(collection_basename + ".stats", 'w')

permutations = [".spo", ".pos", ".osp"]
indexes = [[0,1], [1,2], [2,0]]

# num_triplets
# distinct_subjects
# distinct_predicates
# distinct_objects
# distinct_sp
# distinct_po
# distinct_os
quantities = [0, 0, 0, 0, 0, 0, 0]

def collect_stats(input_filename, first, second):

    print("scanning '" + input_filename + "'...")

    prev_x = prev_y = -1
    distinct_unigrams = 0
    distinct_bigrams = 0
    total_triplets = 0
    with open(input_filename) as f:
        for line in f:
            parsed = line.split(' ')
            x = int(parsed[first])
            y = int(parsed[second])

            if prev_x != x:
                distinct_unigrams += 1

            if prev_x != x or prev_y != y:
                distinct_bigrams += 1

            prev_x = x
            prev_y = y
            total_triplets += 1

    return (distinct_unigrams, distinct_bigrams, total_triplets)

for i in range(0, 3):
    p = permutations[i]
    index = indexes[i]
    input_filename = collection_basename + p
    (a, b, c) = collect_stats(input_filename, index[0], index[1])
    quantities[0] = c
    quantities[i + 1] = a
    quantities[i + 4] = b
    # print(a,b,c)

for q in quantities:
    output_file.write(str(q) + "\n")
output_file.close()
