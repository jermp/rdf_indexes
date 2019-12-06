import sys, gzip, mmh3
from rdf_parser import parse_nt
from rdf_parser import parse_nq
import numpy

input_filename = sys.argv[1] # N-triplets file .gz

subjects = {}
predicates = {}
objects = {}

use_hashes = False
if len(sys.argv) > 2 and sys.argv[2] == "--hash":
    use_hashes = True

def build_vocab(dictionary, filename):
    id = 0
    with open(filename) as f:
        for line in f:
            parsed = line.rstrip('\n')
            if use_hashes:
                parsed = numpy.uint64(mmh3.hash64(parsed, signed=False)[0])
            dictionary[parsed] = id
            id += 1

    print "assigned ids from 0 to " + str(id - 1)

# assume there are:
# 'input_filename'.subjects_vocab
# 'input_filename'.predicates_vocab
# 'input_filename'.objects_vocab
# in the same folder as the one for the input file
prefix_name = input_filename.split('.gz')[0]

print("building vocabularies...")
build_vocab(subjects, prefix_name + ".subjects_vocab")
build_vocab(predicates, prefix_name + ".predicates_vocab")
build_vocab(objects, prefix_name + ".objects_vocab")

lines = 0
print("mapping dataset...")

output_file = open(prefix_name + ".mapped.unsorted", 'w')
with gzip.open(input_filename, 'rb') as f:
    for line in f:

        # (s, p, o) = parse_nt(line)
        (s, p, o) = parse_nq(line)
        ms = 0
        ps = 0
        os = 0

        if use_hashes:
            hs = numpy.uint64(mmh3.hash64(s, signed=False)[0])
            hp = numpy.uint64(mmh3.hash64(p, signed=False)[0])
            ho = numpy.uint64(mmh3.hash64(o, signed=False)[0])
        else:
            hs = s.encode('utf-8')
            hp = p.encode('utf-8')
            ho = o.encode('utf-8')

        try:
            ms = subjects[hs]
            try:
                ps = predicates[hp]
                try:
                    os = objects[ho]

                    mapped = str(ms) + " " + str(ps) + " " + str(os)
                    output_file.write(mapped + "\n")

                except KeyError:
                    print "'" + o + "' not found in objects_vocab"
                    print line
                    print s, p, o

            except KeyError:
                print "'" + p + "' not found in predicates_vocab"
                print line
                print s, p, o

        except KeyError:
            print "'" + s + "' not found in subjects_vocab"
            print line
            print s, p, o

        lines += 1
        if lines % 1000000 == 0:
            print "processed " + str(lines) + " lines"

print "processed " + str(lines) + " lines"
output_file.close()
