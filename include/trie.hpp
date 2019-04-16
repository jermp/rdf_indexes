#pragma once

#include "util_types.hpp"
#include "parameters.hpp"
#include "util.hpp"

namespace rdf {

template <typename Mapper, typename Levels>
struct trie {
    typedef Levels levels_type;

    struct builder {
        builder() {}

        builder(int perm, parameters const& params) : m_perm(perm) {
            assert(perm > 0);

            resize(m_first.pointers,
                   params.num_nodes(perm, level_type::first) + 1,
                   params.num_nodes(perm, level_type::second));

            resize(m_second.nodes, params.num_nodes(perm, level_type::second),
                   params.num_types(perm, level_type::second));

            resize(m_second.pointers,
                   params.num_nodes(perm, level_type::second) + 1,
                   params.num_nodes(perm, level_type::third));

            resize(m_third.nodes, params.num_nodes(perm, level_type::third),
                   params.num_types(perm, level_type::third));
        }

        void build_first_and_second_level(trie<Mapper, Levels>& t,
                                          parameters const& params) {
            std::string filename(std::string(params.collection_basename) + "." +
                                 suffix(m_perm));
            std::ifstream input(filename.c_str(), std::ios_base::in);
            triplets_iterator input_it(input, m_perm);

            uint64_t pointer_first = 0;
            uint64_t pointer_second = 0;
            triplet prev;
            while (input) {
                triplet curr = *input_it;

                if (curr.first != prev.first) {
                    m_first.pointers.push_back(pointer_first);
                }

                if (curr.first != prev.first or curr.second != prev.second) {
                    ++pointer_first;
                    m_second.pointers.push_back(pointer_second);
                    m_second.nodes.push_back(curr.second);
                }

                ++pointer_second;
                prev = curr;

                ++input_it;
            }

            input.close();
            m_first.pointers.push_back(pointer_first);
            m_second.pointers.push_back(pointer_second);

            util::logger("compressing...");
            m_first.build_pointers(t.first.pointers, m_first.pointers);
            m_second.build_nodes(t.second.nodes, m_second.nodes,
                                 m_first.pointers);
            m_second.build_pointers(t.second.pointers, m_second.pointers);
            util::logger("DONE");
        }

        void build_third_level(trie<Mapper, Levels>& t,
                               parameters const& params) {
            std::ifstream input(
                std::string(params.collection_basename) + "." + suffix(m_perm),
                std::ios_base::in);
            triplets_iterator input_it(input, m_perm);

            while (input) {
                triplet curr = *input_it;
                uint64_t node = mapper.map(curr);
                m_third.nodes.push_back(node);
                ++input_it;
            }

            util::logger("compressing");
            m_third.build_nodes(t.third.nodes, m_third.nodes,
                                m_second.pointers);
            util::logger("DONE");
            t.m_perm = m_perm;
            builder().swap(*this);
        }

        void swap(builder& other) {
            std::swap(m_perm, other.m_perm);
            m_first.swap(other.m_first);
            m_second.swap(other.m_second);
            m_third.swap(other.m_third);
        }

        Mapper mapper;

    private:
        int m_perm;
        typename Levels::first::builder m_first;
        typename Levels::second::builder m_second;
        typename Levels::third::builder m_third;
    };

    trie()
        : first(level_type::first)
        , second(level_type::second)
        , third(level_type::third) {}

    struct iterator;
    iterator select_all();
    iterator select(triplet const& t);
    uint64_t is_member(triplet const& t);

    /* specializations */
    struct iterator_so;
    struct iterator_po;
    iterator_so select_so(triplet const& t);
    iterator_po select_p(triplet const& t);
    iterator_po select_o(triplet const& t);
    /*******************/

    void print_stats(essentials::json_lines& stats, size_t bytes);

    int id() const {
        return m_perm;
    }

    uint64_t size() const {
        return first.size() + second.size() + third.size();
    }

    uint64_t triplets() const {
        return third.size();
    }

    size_t bytes() const {
        return first.bytes() + second.bytes() + third.bytes() + sizeof(m_perm);
    }

    template <typename Visitor>
    void visit(Visitor& visitor) {
        visitor.visit(first);
        visitor.visit(second);
        visitor.visit(third);
        visitor.visit(m_perm);
    }

    Mapper mapper;
    typename Levels::first first;
    typename Levels::second second;
    typename Levels::third third;

private:
    int m_perm;
};
}  // namespace rdf
