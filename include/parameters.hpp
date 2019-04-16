#pragma once

#include <vector>

#include "util_types.hpp"

namespace rdf {

struct parameters {
    parameters()
        : num_triplets(0)
        , num_elements(6, 0)
        , collection_basename(nullptr)
        , associativity(1) {}

    void load() {
        std::string filename = std::string(collection_basename) + ".stats";
        std::ifstream input(filename.c_str(), std::ios_base::in);
        input >> num_triplets;
        for (int i = 0; i != 6; ++i) {
            input >> num_elements[i];
        }
        // for (auto x: num_elements) {
        //     std::cout << x << std::endl;
        // }
    }

    uint64_t num_types(int perm, int level) const {
        // specialization
        if (perm == 4) {
            if (level == level_type::first) {
                return num_elements[2];
            } else if (level == level_type::second) {
                return num_elements[1];
            } else {
                return num_elements[0];
            }
        }

        return num_elements[(perm - 1 + level - 1) % 3];
    }

    uint64_t num_nodes(int perm, int level) const {
        // specialization
        if (perm == 4) {
            if (level == level_type::first) {
                return num_elements[2];
            } else if (level == level_type::second) {
                return num_elements[4];
            }
            return num_triplets;
        }

        if (level == level_type::first) {
            return num_elements[perm - 1];
        }
        if (level == level_type::second) {
            return num_elements[perm - 1 + 3];
        }
        return num_triplets;
    }

    uint64_t num_triplets;

    uint64_t triplets() const {
        return num_triplets;
    }

    uint64_t subjects() const {
        assert(num_elements.size() > 0);
        return num_elements[0];
    }

    uint64_t predicates() const {
        assert(num_elements.size() > 1);
        return num_elements[1];
    }

    uint64_t objects() const {
        assert(num_elements.size() > 2);
        return num_elements[2];
    }

    // num_elements[0] = num. of distinct subjects
    // num_elements[1] = num. of distinct predicates
    // num_elements[2] = num. of distinct objects
    // num_elements[3] = num. of distinct pairs (s,p)
    // num_elements[4] = num. of distinct pairs (p,o)
    // num_elements[5] = num. of distinct pairs (o,s)
    std::vector<uint64_t> num_elements;
    char const* collection_basename;
    uint32_t associativity;
};
}  // namespace rdf