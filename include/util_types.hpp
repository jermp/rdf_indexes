#pragma once

#include <algorithm>
#include <chrono>
#include <numeric>

#include "../external/essentials/include/essentials.hpp"

namespace rdf {

namespace global {
static const uint64_t wildcard_symbol = uint64_t(-1);
}

struct range {
    uint64_t begin, end;
};

struct triplet {
    triplet()
        : first(global::wildcard_symbol)
        , second(global::wildcard_symbol)
        , third(global::wildcard_symbol) {}

    bool operator==(triplet const& rhs) const {
        return (*this).first == rhs.first and (*this).second == rhs.second and
               (*this).third == rhs.third;
    }

    bool operator!=(triplet const& rhs) const {
        return !((*this) == rhs);
    }

    friend std::ostream& operator<<(std::ostream& os, triplet const& rhs) {
        os << "(" << rhs.first << "," << rhs.second << "," << rhs.third << ")";
        return os;
    }

    uint64_t first, second, third;
};

enum permutation_type {
    spo = 1,
    pos = 2,
    osp = 3,

    ops = 4,
    pso = 5
};

enum level_type { first = 1, second = 2, third = 3 };

struct triplets_iterator {
    triplets_iterator(std::ifstream& in, int perm = permutation_type::spo)
        : m_perm(perm), m_in(in) {
        if (!m_in.good()) {
            throw std::runtime_error(
                "Error in opening file, it may not exist or be malformed.");
        }
        read_next();
    }

    bool has_next() {
        return !m_in.eof();
    }

    void operator++() {
        read_next();
    }

    triplet operator*() {
        return m_val;
    }

private:
    int m_perm;
    triplet m_val;
    std::ifstream& m_in;

    inline void read_next() {
        switch (m_perm) {
            case permutation_type::spo:
                m_in >> m_val.first;
                m_in >> m_val.second;
                m_in >> m_val.third;
                break;
            case permutation_type::pos:
                m_in >> m_val.third;
                m_in >> m_val.first;
                m_in >> m_val.second;
                break;
            case permutation_type::osp:
                m_in >> m_val.second;
                m_in >> m_val.third;
                m_in >> m_val.first;
                break;
            case permutation_type::ops:
                m_in >> m_val.third;
                m_in >> m_val.second;
                m_in >> m_val.first;
                break;
            case permutation_type::pso:
                m_in >> m_val.second;
                m_in >> m_val.first;
                m_in >> m_val.third;
                break;
            default:
                assert(false);
        }
    }
};

}  // namespace rdf
