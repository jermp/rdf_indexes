#pragma once

#include "trie.hpp"

namespace rdf {

template <typename Mapper, typename Levels>
struct trie<Mapper, Levels>::iterator {
    iterator(uint64_t first, typename Levels::second::iterator const& second,
             typename Levels::third::iterator const& third,
             uint64_t num_triplets, Mapper const& mapper)
        : m_i(0)
        , m_size(num_triplets)
        , m_second(second)
        , m_third(third)
        , m_mapper(mapper) {
        m_val.first = first;
        m_val.second = *m_second;
        m_val.third = *m_third;
        m_val.third = m_mapper.unmap(m_val);
    }

    uint64_t size() const {
        return m_size;
    }

    inline bool has_next() {
        return m_i != m_size;
    }

    void operator++() {
        ++m_i;
        if (has_next()) {
            bool switch_range = ++m_third;
            m_val.third = *m_third;

            if (switch_range) {
                switch_range = ++m_second;
                m_val.second = *m_second;
                if (switch_range) {
                    ++m_val.first;
                }
            }
            m_val.third = m_mapper.unmap(m_val);
        }
    }

    triplet operator*() {
        return m_val;
    }

private:
    triplet m_val;
    uint64_t m_i;
    uint64_t m_size;
    typename Levels::second::iterator m_second;
    typename Levels::third::iterator m_third;
    Mapper m_mapper;
};

template <typename Mapper, typename Levels>
typename trie<Mapper, Levels>::iterator trie<Mapper, Levels>::select_all() {
    return typename trie<Mapper, Levels>::iterator(
        0,
        typename Levels::second::iterator(second.nodes.begin(),
                                          first.pointers.begin()),
        typename Levels::third::iterator(third.nodes.begin(),
                                         second.pointers.begin()),
        triplets(), mapper);
}

template <typename Mapper, typename Levels>
typename trie<Mapper, Levels>::iterator trie<Mapper, Levels>::select(
    triplet const& t) {
    assert(t.first != global::wildcard_symbol);
    assert(t.third == global::wildcard_symbol);

    range r;
    uint64_t i = t.first;
    uint64_t j;
    uint64_t num_triplets = 0;

    r = first.pointers[i];
    j = r.begin;

    if (t.second == global::wildcard_symbol and
        t.third == global::wildcard_symbol) {
        num_triplets =
            second.pointers[r.end - 1].end - second.pointers[r.begin].begin;
    }

    if (t.second != global::wildcard_symbol) {
        j = second.nodes.find(r, t.second);
    }

    typename Levels::second::iterator second_level_iterator(
        second.nodes.at(r, j), first.pointers.at(i));

    i = j;

    r = second.pointers[i];
    j = r.begin;

    if (t.second != global::wildcard_symbol) {
        num_triplets = r.end - r.begin;
    }

    typename Levels::third::iterator third_level_iterator(
        third.nodes.at(r, j), second.pointers.at(i));

    return iterator(t.first, second_level_iterator, third_level_iterator,
                    num_triplets, mapper);
}

template <typename Mapper, typename Levels>
template <typename Dictionary>
typename trie<Mapper, Levels>::iterator trie<Mapper, Levels>::select_range(
    triplet const& t, uint64_t lower_bound, uint64_t upper_bound,
    Dictionary& dictionary) {
    assert(t.first != global::wildcard_symbol);
    assert(t.second == global::wildcard_symbol);
    assert(t.third == global::wildcard_symbol);

    uint64_t lower_bound_id = dictionary.next_geq(lower_bound);
    uint64_t upper_bound_id = dictionary.next_geq(upper_bound);

    uint64_t i = t.first;
    range r = first.pointers[i];
    uint64_t j = second.nodes.find(r, lower_bound_id);
    uint64_t k = second.nodes.find(r, upper_bound_id);
    uint64_t num_triplets =
        second.pointers[k - 1].end - second.pointers[j].begin;

    typename Levels::second::iterator second_level_iterator(
        second.nodes.at(r, j), first.pointers.at(i));

    i = j;
    r = second.pointers[i];
    j = r.begin;

    typename Levels::third::iterator third_level_iterator(
        third.nodes.at(r, j), second.pointers.at(i));

    return iterator(t.first, second_level_iterator, third_level_iterator,
                    num_triplets, mapper);
}

template <typename Mapper, typename Levels>
struct trie<Mapper, Levels>::iterator_so {
    iterator_so(uint64_t first,
                typename Levels::second::iterator const& second_it,
                typename Levels::third::iterator const& third_it,
                typename Levels::third::nodes_type& nodes, uint64_t third,
                uint64_t num_triplets)
        : m_i(0)
        , m_size(num_triplets)
        , m_second(second_it)
        , m_third(third_it)
        , m_nodes(&nodes) {
        // permute (s,p,o) in (o,s,p)
        m_val.first = third;
        m_val.second = first;
        m_val.third = *m_second;
    }

    bool has_next() {
        while (m_i < m_size) {
            m_val.third = *m_second;
            auto r = m_third.pointer();
            uint64_t pos = m_nodes->find(r, m_val.first);
            if (pos != global::not_found) return true;
            this->operator++();
        }
        return false;
    }

    void operator++() {
        m_third.next_pointer();
        ++m_second;
        ++m_i;
    }

    triplet operator*() {
        return m_val;
    }

private:
    triplet m_val;
    uint64_t m_i;
    uint64_t m_size;
    typename Levels::second::iterator m_second;
    typename Levels::third::iterator m_third;
    typename Levels::third::nodes_type* m_nodes;
};

template <typename Mapper, typename Levels>
typename trie<Mapper, Levels>::iterator_so trie<Mapper, Levels>::select_so(
    triplet const& t) {
    assert(id() == permutation_type::spo);
    assert(t.first != global::wildcard_symbol);
    assert(t.second == global::wildcard_symbol);
    assert(t.third != global::wildcard_symbol);

    range r;
    uint64_t i = t.first;
    uint64_t j;

    r = first.pointers[i];
    j = r.begin;

    uint64_t num_triplets = r.end - r.begin;  // at most

    typename Levels::second::iterator second_level_iterator(
        second.nodes.at(r, j), first.pointers.at(i));

    i = j;

    r = second.pointers[i];
    j = r.begin;

    typename Levels::third::iterator third_level_iterator(
        third.nodes.at(r, j), second.pointers.at(i));

    return iterator_so(t.first, second_level_iterator, third_level_iterator,
                       third.nodes, t.third, num_triplets);
}

template <typename Mapper, typename Levels>
struct trie<Mapper, Levels>::iterator_po {
    iterator_po(uint64_t second, trie<Mapper, Levels>* data)
        : m_i(0)
        , m_j(0)
        , m_size((data->first).size())
        , m_range_len(0)
        , m_trie(data) {
        // permute (s,p,o) in (p,s,o)
        m_val.first = second;
        m_val.second = 0;
        m_val.third = 0;

        m_second_it = typename Levels::second::iterator(
            (m_trie->second).nodes.begin(), (m_trie->first).pointers.begin());
    }

    bool has_next() {
        while (m_j < m_range_len) {
            return true;
        }

        bool found = false;
        while (!found && m_i < m_size) {
            auto r = m_second_it.pointer();
            uint64_t pos = (m_trie->second).nodes.find(r, m_val.first);
            if (pos != global::not_found) {
                m_val.second = m_i;
                auto r = (m_trie->second).pointers[pos];
                m_j = 0;
                m_range_len = r.end - r.begin;
                m_third_it = typename Levels::third::iterator(
                    (m_trie->third).nodes.at(r, r.begin),
                    (m_trie->second).pointers.at(pos));
                found = true;
            }

            m_second_it.next_pointer();
            ++m_i;
        }

        return found;
    }

    void operator++() {
        ++m_j;
        ++m_third_it;
    }

    triplet operator*() {
        m_val.third = *m_third_it;
        return m_val;
    }

private:
    triplet m_val;
    uint64_t m_i;
    uint64_t m_j;
    uint64_t m_size;
    uint64_t m_range_len;
    trie<Mapper, Levels>* m_trie;
    typename Levels::second::iterator m_second_it;
    typename Levels::third::iterator m_third_it;
};

template <typename Mapper, typename Levels>
typename trie<Mapper, Levels>::iterator_po trie<Mapper, Levels>::select_p(
    triplet const& t) {
    assert(id() == permutation_type::spo);
    assert(t.first == global::wildcard_symbol);
    assert(t.second != global::wildcard_symbol);
    assert(t.third == global::wildcard_symbol);
    return iterator_po(t.second, this);
}

template <typename Mapper, typename Levels>
typename trie<Mapper, Levels>::iterator_po trie<Mapper, Levels>::select_o(
    triplet const& t) {
    assert(id() == permutation_type::pos);
    assert(t.first == global::wildcard_symbol);
    assert(t.second != global::wildcard_symbol);
    assert(t.third == global::wildcard_symbol);
    return iterator_po(t.second, this);
}

template <typename Mapper, typename Levels>
uint64_t trie<Mapper, Levels>::is_member(triplet const& t) {
    assert(t.first != global::wildcard_symbol);
    assert(t.second != global::wildcard_symbol);
    assert(t.third != global::wildcard_symbol);

    uint64_t i = t.first;
    range r = first.pointers[i];
    uint64_t j = second.nodes.find(r, t.second);
    if (j == global::not_found) {
        return j;
    }

    i = j;
    r = second.pointers[i];
    uint64_t mapped = mapper.map(t);
    j = third.nodes.find(r, mapped);
    return j;
}
}  // namespace rdf
