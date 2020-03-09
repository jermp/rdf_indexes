import sys, gzip, mmh3
from rdf_parser import parse_nt
from rdf_parser import parse_nq
import numpy

input_filename = sys.argv[1] # N-triplets file .gz

extract_subjects = False
extract_predicates = False
extract_objects = False
use_hashes = False

for i in range(2, len(sys.argv)):
    if sys.argv[i] == "-S":
        extract_subjects = True
        print("extracting vocab for subjects")
    elif sys.argv[i] == "-P":
        extract_predicates = True
        print("extracting vocab for predicates")
    elif sys.argv[i] == "-O":
        extract_objects = True
        print("extracting vocab for objects")
    elif sys.argv[i] == "--hash":
        use_hashes = True
    else:
        print("ivalid argument")
        exit()

subjects = {}
predicates = {}
objects = {}

def count_freq(dictionary, key, extract):
    if extract:
        if key in dictionary:
            dictionary[key] += 1
        else:
            dictionary[key] = 1

lines = 0
print("parsing input file...")

with gzip.open(input_filename, 'rb') as f:
    for line in f:

        # (s, p, o) = parse_nt(line.decode("utf-8"))
        (s, p, o) = parse_nq(line.decode("utf-8"))

        if use_hashes:
            s = numpy.uint64(mmh3.hash64(s, signed=False)[0])
            p = numpy.uint64(mmh3.hash64(p, signed=False)[0])
            o = numpy.uint64(mmh3.hash64(o, signed=False)[0])

        count_freq(subjects, s, extract_subjects)
        count_freq(predicates, p, extract_predicates)
        count_freq(objects, o, extract_objects)

        lines += 1
        if lines % 1000000 == 0:
            print("processed " + str(lines) + " lines")

print("processed " + str(lines) + " lines")

def write_dictionary(dictionary, file, use_hashes):
    print("dictionary has " + str(len(dictionary)) + " keys")
    for key, value in sorted(dictionary.items(), key = lambda kv: kv[1], reverse = True):
        file.write(str(key) + "\n")

dictionary_filename_prefix = input_filename.split('.gz')[0]
print("sorting and writing...")

if extract_subjects:
    subjects_dict_file = open(dictionary_filename_prefix + ".subjects_vocab", 'w')
    write_dictionary(subjects, subjects_dict_file, use_hashes)
    subjects_dict_file.close()

if extract_predicates:
    predicates_dict_file = open(dictionary_filename_prefix + ".predicates_vocab", 'w')
    write_dictionary(predicates, predicates_dict_file, use_hashes)
    predicates_dict_file.close()

if extract_objects:
    objects_dict_file = open(dictionary_filename_prefix + ".objects_vocab", 'w')
    write_dictionary(objects, objects_dict_file, use_hashes)
    objects_dict_file.close()
