#pragma once

#include "util_types.hpp"
#include "parameters.hpp"

namespace rdf {

template <typename SPO, typename POS, typename OSP>
struct index_3t {
    typedef SPO spo_type;
    typedef POS pos_type;
    typedef OSP osp_type;

    struct builder {
        builder(parameters const& params)
            : m_params(params)
            , m_spo(permutation_type::spo, params)
            , m_pos(permutation_type::pos, params)
            , m_osp(permutation_type::osp, params) {}

        void build(index_3t<SPO, POS, OSP>& index) {
            util::logger("building first and second levels...");
            m_spo.build_first_and_second_level(index.m_spo, m_params);
            util::logger("SPO DONE");
            m_pos.build_first_and_second_level(index.m_pos, m_params);
            util::logger("POS DONE");
            m_osp.build_first_and_second_level(index.m_osp, m_params);
            util::logger("OSP DONE");

            m_spo.mapper.initialize(&(index.m_osp));
            m_pos.mapper.initialize(&(index.m_osp));

            util::logger("building third levels...");
            m_spo.build_third_level(index.m_spo, m_params);
            util::logger("SPO DONE");
            m_pos.build_third_level(index.m_pos, m_params);
            util::logger("POS DONE");
            m_osp.build_third_level(index.m_osp, m_params);
            util::logger("OSP DONE");
        }

    private:
        parameters const& m_params;
        typename SPO::builder m_spo;
        typename POS::builder m_pos;
        typename OSP::builder m_osp;
    };

    struct iterator {
        iterator(index_3t& index)
            : m_perm(permutation_type::spo), m_spo(index.m_spo.select_all()) {}

        iterator(triplet const& t, index_3t& index) {
            triplet permuted;
            m_perm = index_3t::permute(t, permuted);
            switch (m_perm) {
                case permutation_type::spo:
                    m_spo = index.m_spo.select(permuted);
                    break;
                case permutation_type::pos:
                    m_pos = index.m_pos.select(permuted);
                    break;
                case permutation_type::osp:
                    m_osp = index.m_osp.select(permuted);
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
            typename OSP::iterator m_osp;
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
        assert(m_spo.triplets() == m_pos.triplets());
        assert(m_pos.triplets() == m_osp.triplets());
        return m_spo.triplets();
    }

    uint64_t subjects() const {
        return m_spo.first.size();
    }

    uint64_t predicates() const {
        return m_pos.first.size();
    }

    uint64_t objects() const {
        return m_osp.first.size();
    }

    size_t bytes() const {
        return m_spo.bytes() + m_pos.bytes() + m_osp.bytes();
    }

    auto& spo() {
        return m_spo;
    }

    auto& pos() {
        return m_pos;
    }

    auto& osp() {
        return m_osp;
    }

    template <typename Visitor>
    void visit(Visitor& visitor) {
        visitor.visit(m_spo);
        visitor.visit(m_pos);
        visitor.visit(m_osp);
        m_spo.mapper.initialize(&m_osp);
        m_pos.mapper.initialize(&m_osp);
    }

    static int permute(triplet const& t, triplet& permuted) {
        // less code?

        if (t.first != global::wildcard_symbol) {
            if (t.second != global::wildcard_symbol or
                (t.second == global::wildcard_symbol and
                 t.third == global::wildcard_symbol)) {
                permuted = t;
                return permutation_type::spo;
            }
        }

        if (t.second != global::wildcard_symbol) {
            if (t.third != global::wildcard_symbol or
                (t.third == global::wildcard_symbol and
                 t.first == global::wildcard_symbol)) {
                permuted.first = t.second;
                permuted.second = t.third;
                permuted.third = t.first;
                return permutation_type::pos;
            }
        }

        permuted.first = t.third;
        permuted.second = t.first;
        permuted.third = t.second;
        return permutation_type::osp;
    }

private:
    SPO m_spo;
    POS m_pos;
    OSP m_osp;
};
}  // namespace rdf