#pragma once

#include <stdexcept>

#include "bit_vector.hpp"
#include "compact_ef.hpp"
#include "compact_vector.hpp"
#include "integer_codes.hpp"
#include "util.hpp"

namespace rdf {
namespace pef {

namespace global {
static const uint64_t linear_scan_threshold = 8;
static const uint64_t log_partition_size = 7;
}  // namespace global

struct pef_sequence {
    pef_sequence()
        : m_size(0), m_universe(0), m_partitions(0), m_log_partition_size(0) {}

    void build(compact_vector::builder const& from,
               compact_vector::builder const& pointers) {
        build(from.begin(), from.size(), pointers);
    }

    template <typename Iterator>
    void build(Iterator begin, uint64_t n,
               compact_vector::builder const& pointers) {
        std::vector<uint64_t> values;
        values.reserve(n);
        uint64_t prev_upper = 0;
        auto pointers_it = pointers.begin();
        uint64_t start = *pointers_it;
        ++pointers_it;
        uint64_t end = *pointers_it;
        uint64_t run = end - start;
        uint64_t within = 0;
        for (uint64_t i = 0; i < n; ++i, ++begin) {
            if (within == run) {
                within = 0;
                do {
                    start = end;
                    ++pointers_it;
                    end = *pointers_it;
                    run = end - start;
                } while (!run);
                prev_upper = values.size() ? values.back() : 0;
            }
            uint64_t v = *begin;
            values.push_back(v + prev_upper);
            ++within;
        }
        assert(values.size() == n);
        write(values.begin(), values.back(), n);
    }

    template <typename Iterator>
    void write(Iterator begin, uint64_t universe, uint64_t n) {
        assert(n > 0);
        m_size = n;
        m_universe = universe;
        m_log_partition_size = global::log_partition_size;

        pef_parameters params;
        uint64_t partition_size = uint64_t(1) << m_log_partition_size;
        size_t partitions = util::ceil_div(n, partition_size);

        m_partitions = partitions;

        rdf::bit_vector_builder data_bvb;
        rdf::compact_vector::builder upper_bounds_cvb;
        std::vector<uint64_t> cur_partition;

        uint64_t cur_base = 0;
        if (partitions == 1) {
            cur_base = *begin;
            Iterator it = begin;

            for (size_t i = 0; i < n; ++i, ++it) {
                cur_partition.push_back(*it - cur_base);
            }

            uint64_t universe_bits = util::ceil_log2(universe + 1);
            data_bvb.append_bits(cur_base, universe_bits);
            // write universe only if non-singleton and not tight
            if (n > 1) {
                if (cur_base + cur_partition.back() + 1 == universe) {
                    // tight universe
                    write_delta(data_bvb, 0);
                } else {
                    write_delta(data_bvb, cur_partition.back());
                }
            }

            compact_ef::write(data_bvb, cur_partition.begin(),
                              cur_partition.back() + 1, cur_partition.size(),
                              params);
        } else {
            rdf::bit_vector_builder bv_sequences;
            std::vector<uint64_t> endpoints;
            std::vector<uint64_t> upper_bounds;

            uint64_t cur_i = 0;
            Iterator it = begin;
            cur_base = *begin;
            upper_bounds.push_back(cur_base);

            for (size_t p = 0; p < partitions; ++p) {
                cur_partition.clear();
                uint64_t value = 0;
                for (; cur_i < ((p + 1) * partition_size) && cur_i < n;
                     ++cur_i, ++it) {
                    value = *it;
                    cur_partition.push_back(value - cur_base);
                }

                assert(cur_partition.size() <= partition_size);
                assert((p == partitions - 1) or
                       cur_partition.size() == partition_size);

                uint64_t upper_bound = value;
                assert(cur_partition.size() > 0);

                compact_ef::write(bv_sequences, cur_partition.begin(),
                                  cur_partition.back() + 1,
                                  cur_partition.size(), params);
                endpoints.push_back(bv_sequences.size());
                upper_bounds.push_back(upper_bound);
                cur_base = upper_bound;
            }

            upper_bounds_cvb.resize(upper_bounds.size(),
                                    util::ceil_log2(upper_bounds.back() + 1));
            for (auto u : upper_bounds) {
                upper_bounds_cvb.push_back(u);
            }

            uint64_t endpoint_bits = util::ceil_log2(bv_sequences.size() + 1);
            write_gamma(data_bvb, endpoint_bits);
            for (uint64_t p = 0; p < endpoints.size() - 1; ++p) {
                data_bvb.append_bits(endpoints[p], endpoint_bits);
            }

            data_bvb.append(bv_sequences);
        }

        upper_bounds_cvb.build(m_upper_bounds);
        m_data.build(&data_bvb);
        m_it = iterator(*this);
    }

    inline uint64_t access(uint64_t pos) {
        return m_it.move(pos).second;
    }

    inline uint64_t access(range const& r, uint64_t pos) {
        return access(pos) - previous_range_upperbound(r);
    }

    uint64_t next_geq(uint64_t lower_bound) {
        m_it.move(0);
        auto pos_value =
            m_it.next_geq(lower_bound, {0, size()}, num_partitions());
        return pos_value.first;  // position
    }

    uint64_t next_geq(range const& r, uint64_t id) {
        assert(r.end > r.begin);
        assert(r.end <= size());

        if (r.end - r.begin <= global::linear_scan_threshold) {
            auto it = at(r, r.begin);
            for (uint64_t pos = r.begin; pos != r.end; ++pos) {
                if (it.value() >= id) return pos;
                it.next();
            }
            return r.end - 1;
        }

        if (m_partitions > 1) {
            uint64_t partition_begin = r.begin >> m_log_partition_size;
            m_it.switch_partition(partition_begin);
        }

        uint64_t prev_upper = previous_range_upperbound(r);
        id += prev_upper;
        uint64_t partition_end = r.end >> m_log_partition_size;
        m_it.move(r.begin);
        auto pos_value = m_it.next_geq2(id, r, partition_end);
        // if (pos_value.second != rdf::global::not_found) {
        return pos_value.first;
        // }

        // return rdf::global::not_found;
    }

    uint64_t find(range const& r, uint64_t id) {
        assert(r.end > r.begin);
        assert(r.end <= size());

        // if (id == 0 or (r.end - r.begin == 1)) {
        //     uint64_t prev_upper = previous_range_upperbound(r);
        //     auto pos_value = m_it.move(r.begin);
        //     if (pos_value.second == id + prev_upper) {
        //         return pos_value.first;
        //     }
        //     return rdf::global::not_found;
        // }

        if (r.end - r.begin <= global::linear_scan_threshold) {
            auto it = at(r, r.begin);
            for (uint64_t pos = r.begin; pos != r.end; ++pos) {
                if (it.value() == id) {
                    return pos;
                }
                it.next();
            }
            return rdf::global::not_found;
        }

        if (m_partitions > 1) {
            uint64_t partition_begin = r.begin >> m_log_partition_size;
            m_it.switch_partition(partition_begin);
        }

        uint64_t prev_upper = previous_range_upperbound(r);
        id += prev_upper;
        uint64_t partition_end = r.end >> m_log_partition_size;
        m_it.move(r.begin);  // id must be found in range
        auto pos_value = m_it.next_geq(id, r, partition_end);
        if (pos_value.second == id) {
            return pos_value.first;
        }

        return rdf::global::not_found;
    }

    struct iterator {
        typedef std::pair<uint64_t, uint64_t> value_type;  // (position, value)

        iterator() {}

        iterator(pef_sequence& pef, range r = {0, 0}, uint64_t pos = 0) {
            m_partitions = pef.m_partitions;
            m_size = pef.m_size;
            m_universe = pef.m_universe;
            m_bv = &(pef.m_data);
            m_upper_bounds = &(pef.m_upper_bounds);
            m_log_partition_size = pef.m_log_partition_size;
            m_position = pos;

            m_prev_range_upper_bound = pef.previous_range_upperbound(r);

            pef_parameters params;
            rdf::bits_iterator<rdf::bit_vector> it(*m_bv);
            if (m_partitions == 1) {
                m_cur_partition = 0;
                m_cur_begin = 0;
                m_cur_end = m_size;

                uint64_t universe_bits = util::ceil_log2(m_universe + 1);
                m_cur_base = it.get_bits(universe_bits);
                auto ub = 0;
                if (m_size > 1) {
                    uint64_t universe_delta = read_delta(it);
                    ub = universe_delta ? universe_delta
                                        : (m_universe - m_cur_base - 1);
                }

                m_partition_enum = compact_ef::enumerator(
                    *m_bv, it.position(), ub + 1, m_size, params);
                m_cur_upper_bound = m_cur_base + ub;

            } else {
                m_endpoint_bits = read_gamma(it);
                uint64_t cur_offset = it.position();
                m_endpoints_offset = cur_offset;
                uint64_t endpoints_size = m_endpoint_bits * (m_partitions - 1);
                cur_offset += endpoints_size;
                m_sequences_offset = cur_offset;
                slow_move();
            }
        }

        value_type ALWAYSINLINE move(uint64_t position) {
            assert(position <= size());
            m_position = position;
            if (m_position >= m_cur_begin and m_position < m_cur_end) {
                uint64_t val =
                    m_cur_base +
                    m_partition_enum.move(m_position - m_cur_begin).second;
                return value_type(m_position, val);
            }
            return slow_move();
        }

        value_type ALWAYSINLINE next_geq(uint64_t lower_bound, range const& r,
                                         uint64_t partition_end) {
            if (LIKELY(lower_bound >= m_cur_base &&
                       lower_bound <= m_cur_upper_bound)) {
                auto val = m_partition_enum.next_geq(lower_bound - m_cur_base);
                m_position = m_cur_begin + val.first;
                if (m_position < r.begin or
                    m_position >= r.end) {  // not in range
                    return value_type(m_position, rdf::global::not_found);
                }
                return value_type(m_position, m_cur_base + val.second);
            }

            if (m_cur_partition >
                partition_end) {  // out of bounds form the right
                return value_type(m_position, rdf::global::not_found);
            }

            if (lower_bound < m_cur_base) {  // out of bounds form the left
                return value_type(m_position, rdf::global::not_found);
            }

            return slow_next_geq(lower_bound, r, partition_end);
        }

        value_type ALWAYSINLINE next_geq2(uint64_t lower_bound, range const& r,
                                          uint64_t partition_end) {
            if (LIKELY(lower_bound >= m_cur_base &&
                       lower_bound <= m_cur_upper_bound)) {
                auto val = m_partition_enum.next_geq(lower_bound - m_cur_base);
                m_position = m_cur_begin + val.first;

                if (m_position < r.begin) {
                    return move(r.begin);
                }

                if (m_position >= r.end) {
                    return move(r.end - 1);
                }

                return value_type(m_position, m_cur_base + val.second);
            }

            if (lower_bound < m_cur_base) {  // out of bounds form the left
                return move(r.begin);
            }

            if (m_cur_partition >
                partition_end) {  // out of bounds form the right
                return move(r.end - 1);
            }

            return slow_next_geq2(lower_bound, r, partition_end);
        }

        uint64_t size() const {
            return m_size;
        }

        uint64_t prev_value() {
            if (UNLIKELY(m_position == m_cur_begin)) {
                m_last = m_cur_partition ? m_cur_base : 0;
            } else {
                m_last = m_cur_base + m_partition_enum.prev_value();
            }
            return m_last;
        }

        template <typename T>
        uint64_t bsearch(T const* vec, uint64_t lower_bound,
                         uint64_t partition_begin, uint64_t partition_end) {
            if (LIKELY(partition_end - partition_begin <=
                       global::linear_scan_threshold)) {
                uint64_t id =
                    scan(vec, partition_begin, partition_end, lower_bound);
                return id;
            }

            uint64_t partition_id = partition_begin;
            uint64_t lo = partition_begin;
            uint64_t hi = partition_end;

            while (lo <= hi) {
                uint64_t mid = (lo + hi) / 2;
                uint64_t mid_value = vec->access(mid);

                if (mid_value > lower_bound) {
                    hi = mid != 0 ? mid - 1 : 0;
                    if (lower_bound > vec->access(hi)) {
                        return mid;
                    }
                } else if (mid_value < lower_bound) {
                    lo = mid + 1;
                    if (lower_bound < (lo != vec->size() - 1 ? vec->access(lo)
                                                             : vec->back())) {
                        partition_id = lo;
                    }
                } else {
                    return mid;
                }

                if (hi - lo <= global::linear_scan_threshold) {
                    return scan(vec, lo, hi, lower_bound);
                }
            }

            return partition_id;
        }

        template <typename T>
        uint64_t inline scan(T const* vec, uint64_t lo, uint64_t hi,
                             uint64_t lower_bound) {
            while (lo <= hi) {
                if (vec->access(lo) >= lower_bound) {
                    break;
                }
                ++lo;
            }
            return lo;
        }

        value_type NOINLINE slow_move() {
            if (m_position == size()) {
                if (m_partitions > 1) {
                    switch_partition(m_partitions - 1);
                }
                m_partition_enum.move(m_partition_enum.size());
                return value_type(m_position, m_universe);
            }
            uint64_t partition = m_position >> m_log_partition_size;
            switch_partition(partition);
            uint64_t val =
                m_cur_base +
                m_partition_enum.move(m_position - m_cur_begin).second;
            return value_type(m_position, val);
        }

        value_type NOINLINE slow_next_geq(uint64_t lower_bound, range const& r,
                                          uint64_t partition_end) {
            if (m_partitions == 1) {
                if (lower_bound < m_cur_base) {
                    return move(0);
                } else {
                    return move(size());
                }
            }

            uint64_t partition_id =
                bsearch(m_upper_bounds, lower_bound, m_cur_partition + 1,
                        partition_end + 1);

            if (partition_id == 0) {
                return move(0);
            }

            if (partition_id == m_upper_bounds->size()) {
                return move(size());
            }

            switch_partition(partition_id - 1);
            return next_geq(lower_bound, r, partition_end);
        }

        value_type NOINLINE slow_next_geq2(uint64_t lower_bound, range const& r,
                                           uint64_t partition_end) {
            if (m_partitions == 1) {
                if (lower_bound < m_cur_base) {
                    return move(0);
                } else {
                    return move(size());
                }
            }

            uint64_t partition_id =
                bsearch(m_upper_bounds, lower_bound, m_cur_partition + 1,
                        partition_end + 1);

            if (partition_id == 0) {
                return move(0);
            }

            if (partition_id == m_upper_bounds->size()) {
                return move(size());
            }

            switch_partition(partition_id - 1);
            return next_geq2(lower_bound, r, partition_end);
        }

        void switch_range() {
            m_prev_range_upper_bound = m_last;
        }

        uint64_t value() {
            uint64_t offset = m_partition_enum.value().second;
            m_last = m_cur_base + offset;
            return m_last - m_prev_range_upper_bound;
        }

        void operator++() {
            next();
        }

        uint64_t next() {
            ++m_position;
            if (LIKELY(m_position < m_cur_end)) {
                uint64_t offset = m_partition_enum.next().second;
                return m_cur_base + offset;
            }
            return slow_next();
        }

        uint64_t slow_next() {
            if (UNLIKELY(m_position == m_size)) {
                assert(m_cur_partition == m_partitions - 1);
                auto val = m_partition_enum.next();
                assert(val.first == m_partition_enum.size());
                (void)val;
                return m_universe;
            }
            switch_partition(m_cur_partition + 1);
            return m_cur_base + m_partition_enum.move(0).second;
        }

        void switch_partition(uint64_t partition) {
            assert(m_partitions > 1);

            uint64_t endpoint =
                partition
                    ? m_bv->get_bits(m_endpoints_offset +
                                         (partition - 1) * m_endpoint_bits,
                                     m_endpoint_bits)
                    : 0;

            m_cur_partition_begin = m_sequences_offset + endpoint;
            util::prefetch(m_bv->data().data() + m_cur_partition_begin / 64);

            m_cur_partition = partition;
            m_cur_begin = partition << m_log_partition_size;
            m_cur_end =
                std::min(size(), (partition + 1) << m_log_partition_size);

            m_cur_upper_bound = m_upper_bounds->access(partition + 1);
            m_cur_base = m_upper_bounds->access(partition);

            m_partition_enum =
                compact_ef::enumerator(*m_bv, m_cur_partition_begin,
                                       m_cur_upper_bound - m_cur_base + 1,
                                       m_cur_end - m_cur_begin, m_params);
        }

        uint8_t m_log_partition_size;

        pef_parameters m_params;
        uint64_t m_partitions;
        uint64_t m_endpoints_offset;
        uint64_t m_endpoint_bits;
        uint64_t m_sequences_offset;
        uint64_t m_size;
        uint64_t m_universe;

        uint64_t m_position;
        uint64_t m_cur_partition_begin;
        uint64_t m_cur_partition;
        uint64_t m_cur_begin;
        uint64_t m_cur_end;
        uint64_t m_cur_base;
        uint64_t m_cur_upper_bound;

        uint64_t m_prev_range_upper_bound;
        uint64_t m_last;

        rdf::bit_vector const* m_bv;
        compact_ef::enumerator m_partition_enum;
        rdf::compact_vector const* m_upper_bounds;
    };

    iterator begin() {
        return iterator(*this);
    }

    iterator at(range const& r, uint64_t pos) {
        return iterator(*this, r, pos);
    }

    uint64_t size() const {
        return m_size;
    }

    uint64_t universe() const {
        return m_universe;
    }

    uint64_t num_partitions() const {
        return m_partitions;
    }

    auto const& upper_bounds() const {
        return m_upper_bounds;
    }

    uint64_t bytes() const {
        return sizeof(m_size) + sizeof(m_universe) + sizeof(m_partitions) +
               m_upper_bounds.bytes() + m_data.bytes() +
               sizeof(m_log_partition_size);
    }

    template <typename Visitor>
    void visit(Visitor& visitor) {
        visitor.visit(m_size);
        visitor.visit(m_universe);
        visitor.visit(m_partitions);
        visitor.visit(m_upper_bounds);
        visitor.visit(m_data);
        visitor.visit(m_log_partition_size);
        if (m_size) m_it = begin();
    }

private:
    uint64_t m_size;
    uint64_t m_universe;
    uint64_t m_partitions;
    pef_parameters m_params;
    rdf::compact_vector m_upper_bounds;
    rdf::bit_vector m_data;
    uint8_t m_log_partition_size;
    iterator m_it;

    uint64_t previous_range_upperbound(range const& r) {
        uint64_t x = 0;
        if (LIKELY(r.begin)) x = access(r.begin - 1);
        return x;
    }
};

}  // namespace pef
}  // namespace rdf
