#pragma once

#include "compact_vector.hpp"
#include "util_types.hpp"
#include "util.hpp"

namespace rdf {

void resize(compact_vector::builder& cvb, uint64_t n, uint64_t max) {
    cvb.resize(n, util::ceil_log2(max + 1));
}

template <typename Nodes, typename Pointers>
struct trie_level {
    typedef Nodes nodes_type;
    typedef Pointers pointers_type;

    struct builder {
        builder() {}

        void build_nodes(Nodes& nodes, compact_vector::builder& from,
                         compact_vector::builder const& pointers) {
            nodes.build(from, pointers);
        }

        void build_pointers(Pointers& pointers,
                            compact_vector::builder const& from) {
            pointers.build(from, false);
        }

        void swap(builder& other) {
            nodes.swap(other.nodes);
            pointers.swap(other.pointers);
        }

        compact_vector::builder nodes;
        compact_vector::builder pointers;
    };

    trie_level(uint8_t level) : m_level(level) {}

    struct iterator {
        iterator() {}

        iterator(typename Nodes::iterator const& nodes_it,
                 typename Pointers::iterator const& pointers_it)
            : m_range_len(0)
            , m_pos_in_range(0)
            , m_node(0)
            , m_end(0)
            , m_nodes_it(nodes_it)
            , m_pointers_it(pointers_it)
            , m_switch_range(false) {
            m_end = m_pointers_it.next();
            next_pointer();
            read();
        }

        uint64_t range_size() const {
            return m_range_len;
        }

        uint64_t operator*() {
            return m_node;
        }

        range pointer() const {
            return {m_begin, m_end};
        }

        void next_pointer() {
            m_begin = m_end;
            m_end = m_pointers_it.next();
            m_range_len = m_end - m_begin;
        }

        bool operator++() {
            next();
            m_switch_range = false;

            if (m_pos_in_range == m_range_len) {
                m_pos_in_range = 0;
                next_pointer();
                m_switch_range = true;
            }

            read();
            return m_switch_range;
        }

    private:
        uint64_t m_range_len;
        uint64_t m_pos_in_range;
        uint64_t m_prev;
        uint64_t m_last;
        uint64_t m_node;
        uint64_t m_begin;
        uint64_t m_end;
        typename Nodes::iterator m_nodes_it;
        typename Pointers::iterator m_pointers_it;
        bool m_switch_range;

        inline void next() {
            ++m_pos_in_range;
            ++m_nodes_it;
        }

        inline void read() {
            if (m_switch_range) {
                m_nodes_it.switch_range();
            }
            m_node = m_nodes_it.value();
        }
    };

    uint64_t size() const {
        if (m_level == level_type::first or m_level == level_type::second) {
            return pointers.size() - 1;
        }
        return nodes.size();
    }

    uint64_t bytes() const {
        if (m_level == level_type::first) {
            return pointers.bytes() + 1;
        }
        if (m_level == level_type::second) {
            return pointers.bytes() + nodes.bytes() + 1;
        }
        return nodes.bytes() + 1;
    }

    template <typename Visitor>
    void visit(Visitor& visitor) {
        visitor.visit(m_level);
        switch (m_level) {
            case level_type::first:
                visitor.visit(pointers);
                break;
            case level_type::second:
                visitor.visit(pointers);
                visitor.visit(nodes);
                break;
            case level_type::third:
                visitor.visit(nodes);
                break;
            default:
                assert(false);
        }
    }

    Nodes nodes;
    Pointers pointers;

private:
    uint8_t m_level;
};
}  // namespace rdf
