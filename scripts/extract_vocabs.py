import sys, gzip, mmh3
from rdf_parser import parse_nt
from rdf_parser import parse_nq
import numpy
from sets import Set

input_filename = sys.argv[1] # N-triplets file .gz

extract_subjects = False
extract_predicates = False
extract_objects = False
use_hashes = False

for i in range(2, len(sys.argv)):
    if sys.argv[i] == "-S":
        extract_subjects = True
        print "extracting vocab for subjects"
    elif sys.argv[i] == "-P":
        extract_predicates = True
        print "extracting vocab for predicates"
    elif sys.argv[i] == "-O":
        extract_objects = True
        print "extracting vocab for objects"
    elif sys.argv[i] == "--hash":
        use_hashes = True
    else:
        print "ivalid argument"
        exit()

subjects = {}
predicates = {}
objects = {}

objects_strings = {}
objects_numbers = Set({})
objects_dates = Set({})

def count_freq(dictionary, key, extract):
    if extract:
        if key in dictionary:
            dictionary[key] += 1
        else:
            dictionary[key] = 1

lines = 0
print("parsing input file...")


# NOTE: it works for WatDiv but it's not general of course...
def parse_date(string):
    if len(string) == 10 and string[4] == '-' and string[7] == '-':
        return True
    return False

with gzip.open(input_filename, 'rb') as f:
    for line in f:

        # (s, p, o) = parse_nt(line)
        (s, p, o) = parse_nq(line)

        # print s
        # print p
        # print o

        if use_hashes:
            s = numpy.uint64(mmh3.hash64(s, signed=False)[0])
            p = numpy.uint64(mmh3.hash64(p, signed=False)[0])
            o = numpy.uint64(mmh3.hash64(o, signed=False)[0])

        count_freq(subjects, s, extract_subjects)
        count_freq(predicates, p, extract_predicates)
        # count_freq(objects, o, extract_objects)


        if extract_objects:
            if parse_date(o):
                # print("date: '" + o + "'")
                objects_dates.add(o)
            else:
                try:
                    number_o = int(o)
                    # print("integer: '" + o + "'")
                    objects_numbers.add(number_o)
                except Exception as e:
                    # o should be a string
                    # print("string: '" + o + "'")
                    count_freq(objects_strings, o, extract_objects)

        lines += 1
        if lines % 1000000 == 0:
            print "processed " + str(lines) + " lines"

print "processed " + str(lines) + " lines"

def write_dictionary(dictionary, file, use_hashes, sort_by_freq = True):
    print "dictionary has " + str(len(dictionary)) + " keys"
    if use_hashes:
        for key, value in sorted(dictionary.iteritems(), key = lambda kv: kv[1] if sort_by_freq else kv[0], reverse = True if sort_by_freq else False):
            file.write(str(key) + "\n")
    else:
        for key, value in sorted(dictionary.iteritems(), key = lambda kv: kv[1] if sort_by_freq else kv[0], reverse = True if sort_by_freq else False):
            file.write(key.encode('utf-8') + "\n")

dictionary_filename_prefix = input_filename.split('.gz')[0]

print("sorting and writing...")

if extract_subjects:
    subjects_dict_file = open(dictionary_filename_prefix + ".subjects_vocab", 'w')
    write_dictionary(subjects, subjects_dict_file, use_hashes, False)
    subjects_dict_file.close()

if extract_predicates:
    predicates_dict_file = open(dictionary_filename_prefix + ".predicates_vocab", 'w')
    write_dictionary(predicates, predicates_dict_file, use_hashes, False)
    predicates_dict_file.close()

if extract_objects:

    objects_dict_file = open(dictionary_filename_prefix + ".objects_vocab", 'w')
    objects_numbers_dict_file = open(dictionary_filename_prefix + ".objects_numbers_vocab", 'w')
    objects_dates_dict_file = open(dictionary_filename_prefix + ".objects_dates_vocab", 'w')

    # write_dictionary(objects, objects_dict_file, use_hashes, False)

    # custom version that writes numbers and dates first, then strings
    for key in sorted(objects_numbers):
        objects_dict_file.write(str(key) + "\n")
        objects_numbers_dict_file.write(str(key) + "\n")

    for key in sorted(objects_dates):
        objects_dict_file.write(key.encode('utf-8') + "\n")
        objects_dates_dict_file.write(key.encode('utf-8') + "\n")

    write_dictionary(objects_strings, objects_dict_file, use_hashes, False)

    objects_dict_file.close()
    objects_numbers_dict_file.close()
    objects_dates_dict_file.close()
