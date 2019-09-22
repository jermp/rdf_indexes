#pragma once

#include "util_types.hpp"

namespace rdf {

struct identity_mapper {
    typedef void mapper_index_type;

    void initialize(mapper_index_type* /* mapper */) {}

    inline uint64_t map(triplet const& t) {
        return t.third;
    }

    inline uint64_t unmap(triplet const& t) {
        return t.third;
    }
};

template <typename Index>
struct sorted_array_mapper {
    typedef Index mapper_index_type;

    inline uint64_t get_parent(triplet const& t) {
        return t.second;
    }

    void initialize(mapper_index_type* mapper) {
        m_mapper = mapper;
    }

    inline uint64_t map(triplet const& t) {
        auto r = (m_mapper->first).pointers[get_parent(t)];
        return (m_mapper->second).nodes.find(r, t.third) - r.begin;
    }

    inline uint64_t unmap(triplet const& t) {
        auto r = (m_mapper->first).pointers[get_parent(t)];
        return (m_mapper->second).nodes.access(r, t.third + r.begin);
    }

private:
    mapper_index_type* m_mapper;
};
}  // namespace rdf
