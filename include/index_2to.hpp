#pragma once

#include "compact_vector.hpp"
#include "parameters.hpp"
#include "util_types.hpp"

namespace rdf {

template <typename SPO, typename OPS>
struct index_2to {
    typedef SPO spo_type;
    typedef OPS ops_type;

    template <typename Nodes = typename SPO::levels_type::first::nodes_type,
              typename Pointers =
                  typename SPO::levels_type::first::pointers_type>
    struct p_index {
        struct builder {
            builder() {}

            builder(parameters const& params) {
                uint64_t n = params.num_elements[3];  // (s,p) pairs
                resize(pointers, params.predicates() + 1, n);
                resize(nodes, n, params.subjects());
            }

            void build(p_index<Nodes, Pointers>& index,
                       parameters const& params) {
                // std::cout << "  num. predicates = " << params.predicates()
                //           << std::endl;

                std::vector<uint64_t> offsets(params.predicates() + 1, 0);
                int perm = permutation_type::spo;
                std::string filename(std::string(params.collection_basename) +
                                     "." + suffix(perm));

                // 1. scan the whole file to build the offsets (pointers)
                {
                    std::ifstream input(filename.c_str(), std::ios_base::in);
                    triplets_iterator input_it(input, perm);

                    triplet prev;
                    while (input) {
                        triplet curr = *input_it;
                        if (curr.first != prev.first or
                            curr.second != prev.second) {
                            ++offsets[curr.second + 1];  // shifted by 1
                        }
                        prev = curr;
                        ++input_it;
                    }
                    input.close();

                    // transform the counts in offsets
                    for (uint64_t i = 2; i < offsets.size(); ++i) {
                        offsets[i] += offsets[i - 1];
                    }

                    std::cout << "  offsets.back() = " << offsets.back()
                              << std::endl;
                    std::cout << "  (s,p) pairs = " << params.num_elements[3]
                              << std::endl;
                    assert(offsets.back() == params.num_elements[3]);
                    pointers.fill(offsets.begin(), offsets.size());
                    index.pointers.build(pointers, false);
                }

                // 2. scan the whole file to build the sequence
                {
                    std::ifstream input(filename.c_str(), std::ios_base::in);
                    triplets_iterator input_it(input, perm);

                    triplet prev;
                    while (input) {
                        triplet curr = *input_it;
                        if (curr.first != prev.first or
                            curr.second != prev.second) {
                            uint64_t& pos = offsets[curr.second];
                            nodes.set(pos, curr.first);
                            ++pos;
                        }
                        prev = curr;
                        ++input_it;
                    }
                    input.close();
                }

                index.nodes.build(nodes, pointers);

                std::cout << "  pointers take: "
                          << index.pointers.bytes() * 8.0 / params.triplets()
                          << " [bpt]" << std::endl;
                std::cout << "  nodes take: "
                          << index.nodes.bytes() * 8.0 / params.triplets()
                          << " [bpt]" << std::endl;
            }

            compact_vector::builder pointers;
            compact_vector::builder nodes;
        };

        struct iterator_p {
            iterator_p(uint64_t second, p_index<Nodes, Pointers>* data,
                       SPO* spo)
                : m_i(0), m_j(0), m_objects(0), m_spo(spo) {
                // permute (s,p,o) in (p,s,o)
                m_val.first = second;
                m_val.second = 0;
                m_val.third = 0;

                auto r = (data->pointers)[second];
                m_subjects = r.end - r.begin;
                m_subjects_it = typename SPO::levels_type::first::iterator(
                    (data->nodes).at(r, r.begin), (data->pointers).at(second));
            }

            bool has_next() {
                while (m_j < m_objects) {
                    return true;
                }

                while (m_i < m_subjects) {
                    uint64_t s = *m_subjects_it;
                    auto r = (m_spo->first).pointers[s];
                    uint64_t pos = (m_spo->second).nodes.find(r, m_val.first);
                    assert(pos != global::not_found);

                    m_val.second = s;
                    r = (m_spo->second).pointers[pos];
                    m_j = 0;
                    m_objects = r.end - r.begin;
                    m_objects_it = typename SPO::levels_type::third::iterator(
                        (m_spo->third).nodes.at(r, r.begin),
                        (m_spo->second).pointers.at(pos));

                    ++m_subjects_it;
                    ++m_i;

                    return true;
                }

                return false;
            }

            void operator++() {
                ++m_j;
                ++m_objects_it;
            }

            triplet operator*() {
                m_val.third = *m_objects_it;
                return m_val;
            }

        private:
            triplet m_val;
            uint64_t m_i;
            uint64_t m_j;
            uint64_t m_subjects;
            uint64_t m_objects;
            SPO* m_spo;
            typename SPO::levels_type::third::iterator m_objects_it;
            typename SPO::levels_type::first::iterator m_subjects_it;
        };

        iterator_p select_p(triplet const& t, SPO* spo) {
            assert(t.first == global::wildcard_symbol);
            assert(t.second != global::wildcard_symbol);
            assert(t.third == global::wildcard_symbol);
            return iterator_p(t.second, this, spo);
        }

        size_t bytes() const {
            return pointers.bytes() + nodes.bytes();
        }

        template <typename Visitor>
        void visit(Visitor& visitor) {
            visitor.visit(pointers);
            visitor.visit(nodes);
        }

        Pointers pointers;
        Nodes nodes;
    };

    struct builder {
        builder(parameters const& params)
            : m_params(params)
            , m_spo(permutation_type::spo, params)
            , m_ops(permutation_type::ops, params)
            , m_p_index(params) {}

        void build(index_2to<SPO, OPS>& index) {
            util::logger("building first and second levels...");
            m_spo.build_first_and_second_level(index.m_spo, m_params);
            util::logger("SPO DONE");
            m_ops.build_first_and_second_level(index.m_ops, m_params);
            util::logger("OPS DONE");

            util::logger("building third levels...");
            m_spo.build_third_level(index.m_spo, m_params);
            util::logger("SPO DONE");
            m_ops.build_third_level(index.m_ops, m_params);
            util::logger("OPS DONE");

            util::logger("building index on predicates...");
            m_p_index.build(index.m_p_index, m_params);
            util::logger("DONE");
        }

    private:
        parameters const& m_params;
        typename SPO::builder m_spo;
        typename OPS::builder m_ops;

        typename p_index<>::builder m_p_index;
    };

    struct iterator {
        iterator(index_2to& index)
            : m_perm(permutation_type::spo), m_spo(index.m_spo.select_all()) {}

        iterator(triplet const& t, index_2to& index) {
            triplet permuted;
            m_perm = index_2to::permute(t, permuted);
            switch (m_perm) {
                case permutation_type::spo:
                    m_spo = index.m_spo.select(permuted);
                    break;
                case permutation_type::ops:
                    m_ops = index.m_ops.select(permuted);
                    break;
                case permutation_type::osp:
                    m_osp = index.m_spo.select_so(permuted);
                    break;
                case permutation_type::pos:
                    // m_pos = index.m_spo.select_p(permuted);
                    m_pos = index.m_p_index.select_p(permuted, &(index.m_spo));
                    break;
                default:
                    assert(false);
            }
        }

#define ITERATOR_METHOD(RETURN_TYPE, METHOD, FORMALS, ACTUALS) \
    RETURN_TYPE METHOD FORMALS {                               \
        switch (m_perm) {                                      \
            case permutation_type::spo:                        \
                return m_spo.METHOD ACTUALS;                   \
            case permutation_type::ops:                        \
                return m_ops.METHOD ACTUALS;                   \
            case permutation_type::osp:                        \
                return m_osp.METHOD ACTUALS;                   \
            case permutation_type::pos:                        \
                return m_pos.METHOD ACTUALS;                   \
            default:                                           \
                assert(false);                                 \
                __builtin_unreachable();                       \
        }                                                      \
    }                                                          \
    /**/

        ITERATOR_METHOD(bool, has_next, (), ());
        ITERATOR_METHOD(void, operator++,(), ());
        ITERATOR_METHOD(triplet, operator*,(), ());

#undef ITERATOR_METHOD

    private:
        int m_perm;
        union {
            typename SPO::iterator m_spo;
            typename SPO::iterator_so m_osp;
            typename OPS::iterator m_ops;
            typename p_index<>::iterator_p m_pos;
            // typename SPO::iterator_po m_pos;
        };
    };

    iterator select(triplet const& t) {
        return iterator(t, *this);
    }

    iterator select_all() {
        return iterator(*this);
    }

    uint64_t is_member(triplet const& t) {
        return m_spo.is_member(t);
    }

    void print_stats(essentials::json_lines& stats);

    uint64_t triplets() const {
        assert(m_spo.triplets() == m_ops.triplets());
        return m_spo.triplets();
    }

    size_t bytes() const {
        return m_spo.bytes() + m_ops.bytes() + m_p_index.bytes();
    }

    auto& spo() {
        return m_spo;
    }

    auto& ops() {
        return m_ops;
    }

    auto& predicates_index() {
        return m_p_index;
    }

    template <typename Visitor>
    void visit(Visitor& visitor) {
        visitor.visit(m_spo);
        visitor.visit(m_ops);
        visitor.visit(m_p_index);
    }

    static int permute(triplet const& t, triplet& permuted) {
        if (t.first != global::wildcard_symbol) {
            permuted = t;
            if (t.second == global::wildcard_symbol and
                t.third != global::wildcard_symbol) {
                return permutation_type::osp;
            }
            return permutation_type::spo;
        } else {
            permuted = t;
            if (t.second != global::wildcard_symbol and
                t.third == global::wildcard_symbol) {
                return permutation_type::pos;
            }
        }

        permuted.first = t.third;
        permuted.third = t.first;
        return permutation_type::ops;
    }

private:
    SPO m_spo;
    OPS m_ops;
    p_index<> m_p_index;
};
}  // namespace rdf