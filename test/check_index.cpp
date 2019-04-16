#include <iostream>

#include "util.hpp"
#include "types.hpp"

using namespace rdf;

template <typename Trie>
void check_permutation(Trie& permutation, int perm, parameters const& params) {
    std::string filename =
        std::string(params.collection_basename) + "." + suffix(perm);
    std::ifstream input(filename.c_str(), std::ios_base::in);
    triplets_iterator input_it(input, perm);

    util::logger("checking permutation " + suffix(perm));
    uint64_t quantum = 10000000;
    if (params.num_triplets < quantum) {
        quantum /= 10;
    }

    auto begin = permutation.select_all();

    uint64_t n = 0;
    while (true) {
        triplet expected = *input_it;
        triplet got = *begin;

        if (!util::check(n, params.num_triplets, got, expected)) {
            return;
        }

        ++n;
        if (n % quantum == 0) {
            std::cout << "checked " << n << "/" << params.num_triplets
                      << " triplets" << std::endl;
        }

        if (n == params.num_triplets) {
            break;
        }

        ++begin;
        ++input_it;
    }

    input.close();
    util::logger("checked " + std::to_string(n) + "/" +
                 std::to_string(params.num_triplets) + " triplets");
    util::logger("OK");
}

template <typename Index>
void check(parameters const& params, char const* index_filename) {
    Index index;
    essentials::load(index, index_filename);
    check_permutation(index.spo(), permutation_type::spo, params);
    check_permutation(index.pos(), permutation_type::pos, params);
    check_permutation(index.osp(), permutation_type::osp, params);
}

// specialization
void check_2to(parameters const& params, char const* index_filename) {
    pef_2to index;
    essentials::load(index, index_filename);
    check_permutation(index.spo(), permutation_type::spo, params);
    check_permutation(index.ops(), permutation_type::ops, params);
}

// specialization
void check_2tp(parameters const& params, char const* index_filename) {
    pef_2tp index;
    essentials::load(index, index_filename);
    check_permutation(index.spo(), permutation_type::spo, params);
    check_permutation(index.pos(), permutation_type::pos, params);
}

int main(int argc, char** argv) {
    int mandatory = 4;
    if (argc < mandatory) {
        std::cout << argv[0] << " <type> <collection_basename> <index_filename>"
                  << std::endl;
        return 1;
    }

    std::string type(argv[1]);
    parameters params;
    params.collection_basename = argv[2];
    params.load();
    char const* index_filename = argv[3];

    if (type == "pef_2to") {
        check_2to(params, index_filename);
        return 0;
    }

    if (type == "pef_2tp") {
        check_2tp(params, index_filename);
        return 0;
    }

    if (false) {
#define LOOP_BODY(R, DATA, T)                 \
    }                                         \
    else if (type == BOOST_PP_STRINGIZE(T)) { \
        check<T>(params, index_filename);     \
        /**/

        BOOST_PP_SEQ_FOR_EACH(LOOP_BODY, , PERMUTED);
#undef LOOP_BODY
    } else {
        building_util::unknown_type(type);
    }

    return 0;
}
