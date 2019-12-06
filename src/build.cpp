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
        essentials::print_size(index);
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

    if (false) {
#define LOOP_BODY(R, DATA, T)              \
    }                                      \
    else if (type == STRINGIZE(T)) {       \
        build<T>(params, output_filename); \
        /**/

        BOOST_PP_SEQ_FOR_EACH(LOOP_BODY, , INDEXES);
#undef LOOP_BODY
    } else {
        building_util::unknown_type(type);
    }

    return 0;
}
