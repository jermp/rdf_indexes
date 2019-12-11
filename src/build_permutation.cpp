#include <iostream>

#include "../external/essentials/include/essentials.hpp"
#include "stats.hpp"
#include "util.hpp"
#include "types.hpp"

using namespace rdf;

template <typename Permutation>
void build(parameters const& params, int perm, char const* output_filename) {
    typename Permutation::builder builder(perm, params);
    Permutation permutation;
    builder.build_first_and_second_level(permutation, params);
    builder.build_third_level(permutation, params);

    double giga = essentials::convert(permutation.bytes(), essentials::GB);
    if (giga < 0.1) {
        std::cout << essentials::convert(permutation.bytes(), essentials::MB)
                  << " [MB]" << std::endl;
    } else {
        std::cout << giga << " [GB]" << std::endl;
    }

    double bits_per_triplet = permutation.bytes() * 8.0 / params.num_triplets;
    std::cout << bits_per_triplet << " [bpt]" << std::endl;

    essentials::json_lines stats;
    permutation.print_stats(stats, permutation.bytes());
    stats.print();

    if (output_filename) {
        // essentials::print_size(permutation);
        util::logger("saving data structure to disk...");
        essentials::save<Permutation>(permutation, output_filename);
        util::logger("DONE");
    }
}

int main(int argc, char** argv) {
    int mandatory = 3;
    if (argc < mandatory) {
        std::cout << argv[0]
                  << " <type> <perm> <collection_basename> [-o output_filename]"
                  << std::endl;
        std::cout
            << "<type> is either 'compact' (bit-packing), 'ef' (elias-fano), "
               "'pef' (partitioned elias-fano) or 'vb' (variable-byte). "
            << std::endl;
        std::cout << "<perm> should be an integer in [1,5] according to "
                     "permutation_type. (See file util_types.hpp)"
                  << std::endl;
        return 1;
    }

    std::string type(argv[1]);
    int perm = std::atoi(argv[2]);
    parameters params;
    params.collection_basename = argv[3];
    params.load();

    char const* output_filename = nullptr;

    for (int i = mandatory; i != argc; ++i) {
        if (std::string(argv[i]) == "-o") {
            ++i;
            output_filename = argv[i];
        }
    }

    if (type == "compact") {
        build<compact_t>(params, perm, output_filename);
    } else if (type == "ef") {
        build<ef_t>(params, perm, output_filename);
    } else if (type == "pef") {
        build<pef_t>(params, perm, output_filename);
    } else if (type == "vb") {
        build<vb_t>(params, perm, output_filename);
    } else {
        building_util::unknown_type(type);
        return 1;
    }

    return 0;
}
