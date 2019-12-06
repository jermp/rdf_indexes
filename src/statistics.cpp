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

    if (type == "compact_3t") {
        statistics<compact_3t>(index_filename);
    } else if (type == "ef_3t") {
        statistics<ef_3t>(index_filename);
    } else if (type == "pef_3t") {
        statistics<pef_3t>(index_filename);
    } else if (type == "vb_3t") {
        statistics<vb_3t>(index_filename);
    } else if (type == "pef_r_3t") {
        statistics<pef_r_3t>(index_filename);
    } else if (type == "pef_2to") {
        statistics<pef_2to>(index_filename);
    } else if (type == "pef_2tp") {
        statistics<pef_2tp>(index_filename);
    } else if (type == "vb_2tp") {
        statistics<vb_2tp>(index_filename);
    } else {
        building_util::unknown_type(type);
    }

    return 0;
}
