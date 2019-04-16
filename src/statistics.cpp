#include <iostream>

#include "stats.hpp"
#include "types.hpp"
#include "util.hpp"

using namespace rdf;

template <typename Index>
void statistics(char const* index_filename) {
    Index index;
    essentials::load<Index>(index, index_filename);
    essentials::json_lines stats;
    index.print_stats(stats);
    stats.save_to_file((std::string(index_filename) + ".stats").c_str());
}

int main(int argc, char** argv) {
    int mandatory = 3;
    if (argc < mandatory) {
        std::cout << argv[0] << " <type> <index_filename>" << std::endl;
        return 1;
    }

    std::string type(argv[1]);
    char const* index_filename = argv[2];

    if (false) {
#define LOOP_BODY(R, DATA, T)                 \
    }                                         \
    else if (type == BOOST_PP_STRINGIZE(T)) { \
        statistics<T>(index_filename);        \
        /**/

        BOOST_PP_SEQ_FOR_EACH(LOOP_BODY, , INDEXES);
#undef LOOP_BODY
    } else {
        building_util::unknown_type(type);
    }

    return 0;
}
