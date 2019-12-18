#pragma once

#include "util_types.hpp"
#include "parameters.hpp"

namespace rdf {

template <typename SPO, typename POS>
struct index_2tp {
    typedef SPO spo_type;
    typedef POS pos_type;

    struct builder {
        builder(parameters const& params)
            : m_params(params)
            , m_spo(permutation_type::spo, params)
            , m_pos(permutation_type::pos, params) {}

        void build(index_2tp<SPO, POS>& index) {
            util::logger("building first and second levels...");
            m_spo.build_first_and_second_level(index.m_spo, m_params);
            util::logger("SPO DONE");
            m_pos.build_first_and_second_level(index.m_pos, m_params);
            util::logger("POS DONE");
            util::logger("building third levels...");
            m_spo.build_third_level(index.m_spo, m_params);
            util::logger("SPO DONE");
            m_pos.build_third_level(index.m_pos, m_params);
            util::logger("POS DONE");
        }

    private:
        parameters const& m_params;
        typename SPO::builder m_spo;
        typename POS::builder m_pos;
    };

    struct iterator {
        iterator(index_2tp& index)
            : m_perm(permutation_type::spo), m_spo(index.m_spo.select_all()) {}

        iterator(triplet const& t, index_2tp& index) {
            triplet permuted;
            m_perm = index_2tp::permute(t, permuted);
            switch (m_perm) {
                case permutation_type::spo:
                    m_spo = index.m_spo.select(permuted);
                    break;
                case permutation_type::pos:
                    m_pos = index.m_pos.select(permuted);
                    break;
                case permutation_type::osp:
                    m_osp = index.m_spo.select_so(permuted);
                    break;
                case permutation_type::ops:
                    m_ops = index.m_pos.select_o(permuted);
                    break;
                default:
                    assert(false);
            }
        }

        template <typename Dictionary>
        iterator(triplet const& t, index_2tp& index, uint64_t lower_bound,
                 uint64_t upper_bound, Dictionary& dictionary) {
            triplet permuted;
            m_perm = index_2tp::permute(t, permuted);
            switch (m_perm) {
                case permutation_type::spo:
                    // in the case where also the subject is specified: NOT
                    // SUPPORTED YET...
                    // m_spo = index.m_spo.select_range(permuted, lower_bound,
                    // upper_bound);
                    // break;
                    assert(false);
                case permutation_type::pos:
                    m_pos = index.m_pos.select_range(permuted, lower_bound,
                                                     upper_bound, dictionary);
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
            case permutation_type::pos:                        \
                return m_pos.METHOD ACTUALS;                   \
            case permutation_type::osp:                        \
                return m_osp.METHOD ACTUALS;                   \
            case permutation_type::ops:                        \
                return m_ops.METHOD ACTUALS;                   \
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
            typename POS::iterator m_pos;
            typename SPO::iterator_so m_osp;
            typename POS::iterator_po m_ops;
        };
    };

    iterator select(triplet const& t) {
        return iterator(t, *this);
    }

    iterator select_all() {
        return iterator(*this);
    }

    template <typename Dictionary>
    iterator select_range(triplet const& t, uint64_t lower_bound,
                          uint64_t upper_bound, Dictionary& dictionary) {
        return iterator(t, *this, lower_bound, upper_bound, dictionary);
    }

    uint64_t is_member(triplet const& t) {
        return m_spo.is_member(t);
    }

    void print_stats(essentials::json_lines& stats);

    uint64_t triplets() const {
        assert(m_spo.triplets() == m_pos.triplets());
        return m_spo.triplets();
    }

    size_t bytes() const {
        return m_spo.bytes() + m_pos.bytes();
    }

    auto& spo() {
        return m_spo;
    }

    auto& pos() {
        return m_pos;
    }

    template <typename Visitor>
    void visit(Visitor& visitor) {
        visitor.visit(m_spo);
        visitor.visit(m_pos);
    }

    static int permute(triplet const& t, triplet& permuted) {
        permuted = t;
        if (t.first != global::wildcard_symbol) {
            if (t.second == global::wildcard_symbol and
                t.third != global::wildcard_symbol) {
                return permutation_type::osp;
            }
            return permutation_type::spo;
        } else {
            if (t.second != global::wildcard_symbol) {
                util::permute(permuted, permutation_type::pos);
                return permutation_type::pos;
            }
        }

        util::permute(permuted, permutation_type::pos);
        return permutation_type::ops;
    }

private:
    SPO m_spo;
    POS m_pos;
};

}  // namespace rdf