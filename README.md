Indexes for RDF data
--------------------

This is the C++ library used for the experiments in the paper [*Compressed Indexes for Fast Search of Semantic Data*](http://pages.di.unipi.it/pibiri/papers/RDF19.pdf) [1], by Raffaele Perego, Giulio Ermanno Pibiri and Rossano Venturini.

This guide is meant to provide a brief overview of the library and to illustrate its functionalities through some examples.
##### Table of contents
* [Compiling the code](#compiling-the-code)
* [Input data format](#input-data-format)
* [Preparing the data for inedxing](#preparing-the-data-for-indexing)
* [Building an index](#building-an-index)
* [Querying an index](#querying-an-index)
* [Testing](#testing)
* [Extending the software](#extending-the-software)
* [Authors](#authors)
* [References](#references)

Compiling the code
------------------

The code is tested on Linux with `gcc` 7.3.0 and on Mac 10.14 with `clang` 10.0.0.
To build the code, [`CMake`](https://cmake.org/) and [`Boost`](https://www.boost.org) are required.

The code has few external dependencies (for testing, serialization and memory-mapping facilities), so clone the repository with

	$ git clone --recursive https://github.com/jermp/rdf_indexes.git

If you have cloned the repository without `--recursive`, you will need to perform the following commands before
compiling:

    $ git submodule init
    $ git submodule update

To compile the code for a release environment (see file `CMakeLists.txt` for the used compilation flags), it is sufficient to do the following:

    $ mkdir build
    $ cd build
    $ cmake ..
    $ make

Hint: Use `make -j4` to compile the library in parallel using, e.g., 4 jobs.

For a testing environment, use the following instead:

    $ mkdir debug_build
    $ cd debug_build
    $ cmake .. -DCMAKE_BUILD_TYPE=Debug -DUSE_SANITIZERS=On
    $ make
    
Unless otherwise specified, for the rest of this guide we assume that we type the terminal commands of the following examples from the created directory `build`.

Input data format
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
2. distinct ubjects
3. distinct predicates
4. distinct objects
5. distinct S-P pairs
6. distinct P-O pairs
7. distinct O-S pairs

The next section details how this data format
can be created automatically from a given
RDF dataset in standard N-Triples (`.nt`) format.
(See also [https://www.w3.org/TR/n-triples](https://www.w3.org/TR/n-triples).)

Preparing the data for indexing
-------------------------------

The folder `scripts` contains all the python scripts needed to prepare the datasets for indexing.

Assume we have an RDF dataset in standard N-Triples format,
additionally compressed via gzip.
For the following example, assume to work with the dataset provided in the folder `test_data`: `wordnet31.nt.gz`.
(This dataset has been downloaded from [http://www.rdfhdt.org/datasets](http://www.rdfhdt.org/datasets) and extracted using
the HDT [2] software at [http://www.rdfhdt.org/downloads](http://www.rdfhdt.org/downloads).)


To prepare the data, it is sufficient to follow the following steps
from within the `scripts` folder.

1. Extract the vocabularies.

		python extract_vocabs.py ../test_data/wordnet31.nt.gz -S -P -O

	This script will produce the following files: `wordnet31.subjects_vocab`, `wordnet31.predicates_vocab` and `wordnet31.objects_vocab`.

2. Map the URIs to integer triples.

		python map_dataset.py ../test_data/wordnet31.nt.gz
	
	This script will map the dataset to integer triples,
	producing the file `wordnet31.mapped.unsorted`.

3. Sort the file `wordnet31.mapped.unsorted` materializing the needed permutations.

		python sort.py ../test_data/wordnet31.mapped.unsorted wordnet31
	
	This script will produce the four permutations, one per file:
	`wordnet31.mapped.sorted.spo`, `wordnet31.mapped.sorted.pos`, `wordnet31.mapped.sorted.osp` and `wordnet31.mapped.sorted.ops`.

4. Build the file with the statistics.

		python build_stats.py wordnet31.mapped.sorted

	This script will create the file `wordnet31.mapped.sorted.stats`.


Building the index
------------------


Querying the index
------------------


Testing
-------

Run the script `test/check_everything.py` from within the `./build`
directory to execute an exhaustive testing of every type of index.

	python ../test/check_everything.py ../test_data/wordnet31.mapped.sorted . wordnet
	
This script will check every triple selection pattern
for all the different types of indexes.

Extending the software
----------------------

The library is a flexible template library, allowing *any* encoder to be used on the nodes of the tries.

In order to use your custom encoder for a sequence of integers, the corresponding class must expose the following methods:

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

	

Authors
-------
* [Giulio Ermanno Pibiri](http://pages.di.unipi.it/pibiri/), <giulio.ermanno.pibiri@isti.cnr.it>


References
-------
* [1] Raffaele Perego, Giulio Ermanno Pibiri and Rossano Venturini. *Compressed Indexes for Fast Search of Semantic Data*. 2019. arXiv preprint. https://arxiv.org/abs/1904.07619
* [2] M. A. Martínez-Prieto, M. A. Gallego, and J. D. Fernández. *Exchange and consumption of huge rdf data* in Extended Semantic
Web Conference. Springer, 2012, pp. 437–452.