#include <iostream>

#include "util.hpp"
#include "types.hpp"

using namespace rdf;

template <typename Trie>
void check_find(Trie& permutation) {
    typename Trie::levels_type::third::iterator it(
        permutation.third.nodes.begin(), permutation.second.pointers.begin());
    auto& nodes = permutation.third.nodes;

    uint64_t ranges = permutation.second.size();
    ranges /= 10000;
    util::logger("checking " + std::to_string(ranges) + " ranges");

    for (uint64_t i = 0; i != ranges; ++i) {
        auto r = it.pointer();

        // check every id in [nodes[r.begin], nodes[r.end - 1])
        uint64_t begin = nodes.access(r, r.begin);
        uint64_t end = nodes.access(r, r.end - 1);
        uint64_t j = r.begin;
        uint64_t in_range = begin;

        for (uint64_t k = begin; k != end; ++k) {
            bool present = in_range == k;
            if (present) {
                ++j;
                in_range = nodes.access(r, j);
            }

            uint64_t pos = nodes.find(r, k);
            if (present) {
                if (pos == global::not_found) {
                    std::cout << "Error: " << k
                              << " should have been found in range (" << r.begin
                              << " - " << r.end << ")" << std::endl;
                }
            } else {
                if (pos != global::not_found) {
                    std::cout << "Error: " << k << " is NOT in range ("
                              << r.begin << " - " << r.end
                              << "), but we found it!" << std::endl;
                }
            }
        }

        uint64_t k = 0;
        bool present = k == nodes.access(r, r.begin);
        uint64_t pos = nodes.find(r, k);
        if (present) {
            if (pos == global::not_found) {
                std::cerr << "Error: " << k
                          << " should have been found in range (" << r.begin
                          << " - " << r.end << ")" << std::endl;
            }
        } else {
            if (pos != global::not_found) {
                std::cerr << "Error: " << k << " is NOT in range (" << r.begin
                          << " - " << r.end << "), but we found it!"
                          << std::endl;
            }
        }

        it.next_pointer();
        std::cout << "range " << i + 1 << "/" << ranges << " checked"
                  << std::endl;
    }
}

template <typename Index>
void check(char const* index_filename) {
    Index index;
    essentials::load(index, index_filename);
    check_find(index.spo());
    check_find(index.pos());
    check_find(index.osp());
}

// specialization
void check_2to(char const* index_filename) {
    pef_2to index;
    essentials::load(index, index_filename);
    check_find(index.spo());
    check_find(index.ops());
}

// specialization
void check_2tp(char const* index_filename) {
    pef_2tp index;
    essentials::load(index, index_filename);
    check_find(index.spo());
    check_find(index.pos());
}

int main(int argc, char** argv) {
    int mandatory = 3;
    if (argc < mandatory) {
        std::cout << argv[0] << " <type> <index_filename>" << std::endl;
        return 1;
    }

    std::string type(argv[1]);
    char const* index_filename = argv[2];

    if (type == "pef_2to") {
        check_2to(index_filename);
        return 0;
    }

    if (type == "pef_2tp") {
        check_2tp(index_filename);
        return 0;
    }

    if (type == "compact_3t") {
        check<compact_3t>(index_filename);
    } else if (type == "ef_3t") {
        check<ef_3t>(index_filename);
    } else if (type == "pef_3t") {
        check<pef_3t>(index_filename);
    } else if (type == "vb_3t") {
        check<vb_3t>(index_filename);
    } else if (type == "pef_r_3t") {
        check<pef_r_3t>(index_filename);
    } else {
        building_util::unknown_type(type);
    }

    return 0;
}
