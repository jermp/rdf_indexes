#include <iostream>

#include "../external/essentials/include/essentials.hpp"
#include "util.hpp"
#include "types.hpp"

using namespace rdf;

template <typename Index>
void build(parameters const& params, char const* output_filename) {
    typename Index::builder builder(params);
    Index index;
    builder.build(index);
    double giga = essentials::convert(index.bytes(), essentials::GB);
    if (giga < 0.1) {
        std::cout << essentials::convert(index.bytes(), essentials::MB)
                  << " [MB]" << std::endl;
    } else {
        std::cout << giga << " [GB]" << std::endl;
    }

    double bits_per_triplet = index.bytes() * 8.0 / params.num_triplets;
    std::cout << bits_per_triplet << " [bpt]" << std::endl;

    if (output_filename) {
        // essentials::print_size(index);
        util::logger("saving data structure to disk...");
        essentials::save<Index>(index, output_filename);
        util::logger("DONE");
    }
}

int main(int argc, char** argv) {
    int mandatory = 3;
    if (argc < mandatory) {
        std::cout << argv[0]
                  << " <type> <collection_basename> [-o output_filename]"
                  << std::endl;
        return 1;
    }

    std::string type(argv[1]);
    parameters params;
    params.collection_basename = argv[2];
    params.load();

    char const* output_filename = nullptr;

    for (int i = mandatory; i != argc; ++i) {
        if (std::string(argv[i]) == "-o") {
            ++i;
            output_filename = argv[i];
        }
    }

    // Yes...many ifs are ugly but this avoids the dependency from
    // BOOST_PP_SEQ_FOR_EACH.

    if (type == "compact_3t") {
        build<compact_3t>(params, output_filename);
    } else if (type == "ef_3t") {
        build<ef_3t>(params, output_filename);
    } else if (type == "pef_3t") {
        build<pef_3t>(params, output_filename);
    } else if (type == "vb_3t") {
        build<vb_3t>(params, output_filename);
    } else if (type == "pef_r_3t") {
        build<pef_r_3t>(params, output_filename);
    } else if (type == "pef_2to") {
        build<pef_2to>(params, output_filename);
    } else if (type == "pef_2tp") {
        build<pef_2tp>(params, output_filename);
    } else if (type == "vb_2tp") {
        build<vb_2tp>(params, output_filename);
    } else {
        building_util::unknown_type(type);
    }

    return 0;
}
