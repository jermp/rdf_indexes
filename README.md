Indexes for RDF data
--------------------

This is the C++ library used for the experiments in the paper [*Compressed Indexes for Fast Search of Semantic Data*](http://pages.di.unipi.it/pibiri/papers/RDF19.pdf) [1], by Raffaele Perego, Giulio Ermanno Pibiri and Rossano Venturini.

This guide is meant to provide a brief overview of the library and to illustrate its functionalities through some examples.
##### Table of contents
1. [Compiling the code](#compiling)
2. [Input data format](#input)
3. [Preparing the data for inedxing](#preparing)
4. [Building an index](#building)
5. [Querying an index](#querying)
6. [Statistics](#statistics)
7. [Testing](#testing)
8. [Extending the software](#extending)
9. [Authors](#authors)
10. [References](#references)

Compiling the code <a name="compiling"></a>
--------------------

The code is tested on Linux with `gcc` 7.3.0, 8.3.0, 9.2.0 and on Mac 10.14 with `clang` 10.0.0.
To build the code, [`CMake`](https://cmake.org/) and [`Boost`](https://www.boost.org) are required.

The code has few external dependencies (for benchmarking and serialization), so clone the repository with

	git clone --recursive https://github.com/jermp/rdf_indexes.git

If you have cloned the repository without `--recursive`, you will need to perform the following commands before
compiling:

    git submodule init
    git submodule update

To compile the code for a release environment (see file `CMakeLists.txt` for the used compilation flags), it is sufficient to do the following:

    mkdir build
    cd build
    cmake ..
    make -j

For a testing environment, use the following instead:

    mkdir debug_build
    cd debug_build
    cmake .. -DCMAKE_BUILD_TYPE=Debug -DUSE_SANITIZERS=On
    make -j

Unless otherwise specified, for the rest of this guide we assume that we type the terminal commands of the following examples from the created directory `build`.

Input data format <a name="input"></a>
-----------------

The library works exclusively with integer triples,
thus the data has to be prepared accordingly prior to indexing
and querying.
We assume the RDF triples have been mapped to integer identifiers
and sorted in different permutations of
the subject (S), predicate (P) and object (O)
components.

To build an index we need the following permutations:
SPO, POS and OSP. Also the permutation OPS may be
needed to build some types of indexes.
Each permutation is represented by a separate
file in plain format, having an integer triple
per line with integers separated by whitespaces.
As an example, if our dataset is named `RDF_dataset`,
we need the following files:

-  `RDF_dataset.spo`
-  `RDF_dataset.pos`
-  `RDF_dataset.osp`
-  `RDF_dataset.ops`

We also need a metadata file contaning some useful
statistics about the data. This file must be
named `RDF_dataset.stats`. Also this file is in
plain format and contains 7 integers, one per line:

1. total number of triples
2. distinct subjects
3. distinct predicates
4. distinct objects
5. distinct S-P pairs
6. distinct P-O pairs
7. distinct O-S pairs

The next section details how this data format
can be created automatically from a given
RDF dataset in standard N-Triples format.
(See also [https://www.w3.org/TR/n-triples](https://www.w3.org/TR/n-triples).)

Preparing the data for indexing <a name="preparing"></a>
-------------------------------

The folder `scripts` contains all the python scripts needed to prepare the datasets for indexing.

Assume we have an RDF dataset in standard N-Triples format,
additionally compressed via gzip.
For the following example, assume to work with the dataset provided in the folder `test_data`: `wordnet31.gz`.
(This dataset has been downloaded from [http://www.rdfhdt.org/datasets](http://www.rdfhdt.org/datasets) and extracted using
the HDT [2] software at [http://www.rdfhdt.org/downloads](http://www.rdfhdt.org/downloads).)


To prepare the data, it is sufficient to follow the following steps
from within the `scripts` folder.

**NOTE** - The scripts require the python module `mmh3` that can be easily
installed with `pip install mmh3`.

1. Extract the vocabularies.

		python extract_vocabs.py ../test_data/wordnet31.gz -S -P -O

	This script will produce the following files: `wordnet31.subjects_vocab`, `wordnet31.predicates_vocab` and `wordnet31.objects_vocab`.

2. Map the URIs to integer triples.

		python map_dataset.py ../test_data/wordnet31.gz

	This script will map the dataset to integer triples,
	producing the file `wordnet31.mapped.unsorted`.

3. Sort the file `wordnet31.mapped.unsorted` materializing the needed permutations.

		python sort.py ../test_data/wordnet31.mapped.unsorted wordnet31

	This script will produce the four permutations, one per file:
	`wordnet31.mapped.sorted.spo`, `wordnet31.mapped.sorted.pos`, `wordnet31.mapped.sorted.osp` and `wordnet31.mapped.sorted.ops`.

4. Build the file with the statistics.

		python build_stats.py wordnet31.mapped.sorted

	This script will create the file `wordnet31.mapped.sorted.stats`.

Finally, the bash script `scripts/process.sh` summarizes all the
steps described, therefore you can just run

	bash process.sh ../test_data/wordnet31.gz

to prepare the `wordnet31` collection for indexing.

Building an index <a name="building"></a>
------------------

With all the data prepared for indexing as explained in
[Section 3](#preparing),
building an index is as easy as:

	./build <type> <collection_basename> [-o output_filename]

For example, the command:

	./build pef_3t ../test_data/wordnet31.mapped.sorted -o wordnet31.pef_3t.bin

will build a 3T index (see Section 3.1 of [1]), compressed
with partitioned Elias-Fano (PEF), that is serialized to
the binary file `wordnet31.pef_3t.bin`.

See also the file `include/types.hpp` for all other index types.
At the moment we support the following types.
 					`compact_3t`
                `ef_3t`
                `vb_3t`
                `pef_3t`
                `pef_r_3t`
                `pef_2to`
                `pef_2tp`

Querying an index <a name="querying"></a>
------------------
A triple selection pattern is just an ordinary integer triple
with *k* wildcard symbols, for 0 ≤ *k* ≤ 3.
In the library, a wildcard is represented by the integer -1.
For example, the query pattern

	13 549 -1

asks for all triples where subject = 13 and predicate = 549.
Similary

	-1 -1 286

asks for all triples having object = 286.

If you do not have a querylog with some triple selection patterns
of this form, just sample randomly the input data with (use `gshuf` instead of `shuf` on Mac OSX)

	shuf -n 5000 ../../test_data/wordnet31.mapped.unsorted > ../../test_data/wordnet31.mapped.unsorted.queries.5000

that will create a querylog with 5000 triples selected at random.

Then, the executable `./queries` can be used to query an index, specifying a querylog, the number and position of the wildcards:

	./queries <type> <perm> <index_filename> [-q <query_filename> -n <num_queries> -w <num_wildcards>]

The arguments `<perm>` and `-w <num_wildcards>` are used to specify the triple selection patterns.
`<perm>` is an integer 1..3 indicating the S-P-O permutation where
`<num_wildcards>` symbols are set to wildcards (starting from the right).
We use the convention that `perm = 1` specifies SPO, `perm = 2` specifies POS and `perm = 3` specifies OSP.

Therefore we have:

- `perm = 1` and `-w 0` <=> SPO
- `perm = 1` and `-w 1` <=> SP?
- `perm = 1` and `-w 2` <=> S??
- `perm = 2` and `-w 1` <=> ?PO
- `perm = 2` and `-w 2` <=> ?P?
- `perm = 3` and `-w 1` <=> S?O
- `perm = 3` and `-w 2` <=> ??O
- any `perm` and `-w 3` <=> ???

For example

	./queries pef_3t 1 wordnet31.pef_3t.bin -q ../test_data/wordnet31.mapped.unsorted.queries.5000 -n 5000 -w 1

will execute 5000 SP? queries.

Statistics <a name="statistics"></a>
----------

The executable `./statistics` will print some useful statistics
about the nodes of the tries and their space occupancy:

	./statistics <type> <index_filename>

For example

	./statistics pef_2tp wordnet31.pef_2tp.bin

Testing <a name="testing"></a>
-------

Run the script `test/check_everything.py` from within the `./build`
directory to execute an exhaustive testing of every type of index.

	python ../test/check_everything.py ../test_data/wordnet31.mapped.sorted . wordnet

This script will check every triple selection pattern
for all the different types of indexes.

See also the directory `./test` for further testing executables.

Extending the software <a name="extending"></a>
----------------------

The library is a flexible template library, allowing *any* encoder to be used on the nodes of the tries.

In order to use your custom encoder for a sequence of integers, the corresponding class must implement the following interface.

```C++
struct iterator;
void build(compact_vector::builder const& from,
           compact_vector::builder const& pointers);
inline uint64_t access(uint64_t pos) const;
inline uint64_t access(range const& r, uint64_t pos);
iterator begin() const;
iterator end() const;
iterator at(range const& r, uint64_t pos) const;
uint64_t find(range const& r, uint64_t id);
uint64_t size() const;
size_t bytes() const;
void save(std::ostream& os) const;
void load(std::istream& is);
```


Authors <a name="authors"></a>
-------
* [Giulio Ermanno Pibiri](http://pages.di.unipi.it/pibiri/), <giulio.ermanno.pibiri@isti.cnr.it>


References <a name="references"></a>
-------
* [1] Raffaele Perego, Giulio Ermanno Pibiri and Rossano Venturini. *Compressed Indexes for Fast Search of Semantic Data*. 2019. arXiv preprint. https://arxiv.org/abs/1904.07619
* [2] M. A. Martínez-Prieto, M. A. Gallego, and J. D. Fernández. *Exchange and consumption of huge rdf data* in Extended Semantic
Web Conference. Springer, 2012, pp. 437–452.