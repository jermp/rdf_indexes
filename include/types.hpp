#pragma once

#include "trie_level.hpp"
#include "trie.hpp"
#include "index_3t.hpp"
#include "index_2to.hpp"
#include "index_2tp.hpp"
#include "mappers.hpp"

#include "compact_vector.hpp"
#include "block_sequence.hpp"
#include "ef/ef_sequence.hpp"
#include "pef/pef_sequence.hpp"
#include "vb/vb.hpp"
#include "algorithms.hpp"

#include "util_types.hpp"

#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/cat.hpp>

namespace rdf {

struct compact_levels {
    typedef trie_level<compact_vector, ef::ef_sequence> first;
    typedef trie_level<compact_vector, ef::ef_sequence> second;
    typedef trie_level<compact_vector, ef::ef_sequence> third;
};
typedef trie<identity_mapper, compact_levels> compact_t;

typedef index_3t<compact_t, compact_t, compact_t> compact_3t;

struct pef_levels {
    typedef trie_level<pef::pef_sequence, ef::ef_sequence> first;
    typedef trie_level<pef::pef_sequence, ef::ef_sequence> second;
    typedef trie_level<pef::pef_sequence, ef::ef_sequence> third;
};

struct pef_compact_levels {
    typedef trie_level<pef::pef_sequence, ef::ef_sequence> first;
    typedef trie_level<pef::pef_sequence, ef::ef_sequence> second;
    typedef trie_level<compact_vector, ef::ef_sequence> third;
};

struct pef_compact_levels2 {
    typedef trie_level<pef::pef_sequence, ef::ef_sequence> first;
    typedef trie_level<compact_vector, ef::ef_sequence> second;
    typedef trie_level<pef::pef_sequence, ef::ef_sequence> third;
};

struct ef_levels {
    typedef trie_level<ef::ef_sequence, ef::ef_sequence> first;
    typedef trie_level<ef::ef_sequence, ef::ef_sequence> second;
    typedef trie_level<ef::ef_sequence, ef::ef_sequence> third;
};

typedef trie<identity_mapper, ef_levels> ef_t;
typedef trie<identity_mapper, pef_levels> pef_t;
typedef trie<identity_mapper, pef_compact_levels> pef_compact_t;
typedef trie<identity_mapper, pef_compact_levels2> pef_compact2_t;

typedef index_3t<ef_t, ef_t, ef_t> ef_3t;
// typedef index_3t<pef_t, pef_t, pef_t> pef_3t;
typedef index_3t<pef_compact_t, pef_t, pef_t> pef_3t;

// typedef index_2to<pef_t, pef_t> pef_2t;
typedef index_2to<pef_compact_t, pef_t> pef_2to;
typedef index_2tp<pef_compact_t, pef_t> pef_2tp;

typedef trie<sorted_array_mapper<pef_t>, pef_levels> pef_rt;
typedef trie<sorted_array_mapper<pef_compact2_t>, pef_levels> pef_rt2;

// typedef index_3t<pef_t, pef_rt, pef_t> pef_r_3t;
// typedef index_3t<pef_compact_t, pef_rt, pef_t> pef_r_3t;
typedef index_3t<pef_compact_t, pef_rt2, pef_compact2_t> pef_r_3t;

typedef block_sequence<
    // vb::vbyte_block
    vb::maskedvbyte_block  // SIMD
    >
    vb_sequence;

struct vb_levels {
    typedef trie_level<vb_sequence, ef::ef_sequence> first;
    typedef trie_level<vb_sequence, ef::ef_sequence> second;
    typedef trie_level<vb_sequence, ef::ef_sequence> third;
};
typedef trie<identity_mapper, vb_levels> vb_t;
typedef index_3t<vb_t, vb_t, vb_t> vb_3t;
typedef index_2tp<pef_compact_t, vb_t> vb_2tp;

#define PERMUTED (compact_3t)(ef_3t)(pef_3t)(vb_3t)(pef_r_3t)
#define INDEXES \
    (compact_3t)(ef_3t)(pef_3t)(vb_3t)(pef_r_3t)(pef_2to)(pef_2tp)(vb_2tp)
}  // namespace rdf
