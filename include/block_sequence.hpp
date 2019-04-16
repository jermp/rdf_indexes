#pragma once

namespace rdf {

template <typename Block>
struct block_sequence {
    typedef uint64_t endpoint_type;
    typedef uint32_t upperbound_type;

    void build(compact_vector::builder const& from,
               compact_vector::builder const& pointers) {
        uint64_t n = from.size();
        m_size = n;
        const static uint64_t block_size = Block::block_size;
        uint64_t blocks = util::ceil_div(n, block_size);
        std::cout << "blocks " << blocks << std::endl;
        size_t begin_endpoints = sizeof(upperbound_type) * blocks;
        size_t begin_data =
            begin_endpoints + sizeof(endpoint_type) * (blocks - 1);
        m_data.resize(begin_data);

        std::vector<uint32_t> buf;
        buf.reserve(block_size);
        uint32_t last = -1;
        auto from_it = from.begin();
        auto pointers_it = pointers.begin();
        uint64_t range_begin = *pointers_it;
        ++pointers_it;
        uint64_t range_end = *pointers_it;
        uint64_t range_len = range_end - range_begin;
        uint64_t within = 0;
        uint32_t b = 0;
        uint32_t curr_block_size =
            (block_size <= n) ? block_size : (n % block_size);

        for (uint64_t i = 0; i != n; ++i, ++from_it) {
            if (within == range_len) {
                within = 0;
                range_begin = range_end;
                ++pointers_it;
                range_end = *pointers_it;
                range_len = range_end - range_begin;
                last =
                    -1;  // first element of a range is always stored as it is
            }

            uint32_t v = *from_it;
            buf.push_back(v - last - 1);
            last = v;
            ++within;

            if (buf.size() == curr_block_size) {
                *(reinterpret_cast<upperbound_type*>(
                    &m_data[sizeof(upperbound_type) * b])) =
                    last;                    // write upper bound
                Block::encode(m_data, buf);  // write block

                if (b != blocks - 1) {
                    *(reinterpret_cast<endpoint_type*>(
                        &m_data[begin_endpoints + sizeof(endpoint_type) * b])) =
                        m_data.size() - begin_data;  // write endpoint
                }

                ++b;
                curr_block_size =
                    ((b + 1) * block_size <= n) ? block_size : (n % block_size);
                buf.clear();
            }
        }
    }

    struct iterator {
        iterator() {}

        iterator(block_sequence const& seq, uint64_t pos = 0)
            : m_size(seq.size())
            , m_blocks(util::ceil_div(m_size, Block::block_size))
            , m_upperbounds(seq.m_data.data())
            , m_endpoints(m_upperbounds + sizeof(upperbound_type) * m_blocks)
            , m_data(m_endpoints + sizeof(endpoint_type) * (m_blocks - 1))
            , m_cur_block(-1) {
            uint64_t block = pos / Block::block_size;
            decode_block(block);
        }

        void operator++() {
            next();
        }

        inline uint64_t operator*() {
            return value();
        }

        inline uint64_t value() {
            return m_val;
        }

        inline void switch_range() {
            m_val = m_buffer[m_pos_in_block];
        }

        inline void switch_range(uint64_t pos_in_block) {
            m_pos_in_block = pos_in_block;
            switch_range();
        }

        bool operator==(iterator const& other) const {
            return position() == other.position();
        }

        bool operator!=(iterator const& other) const {
            return !(*this == other);
        }

        void next() {
            ++m_pos_in_block;
            if (UNLIKELY(m_pos_in_block == m_cur_block_size)) {
                if (m_cur_block + 1 == m_blocks) {
                    m_val += m_buffer[m_cur_block_size - 1] + 1;
                    return;
                }
                decode_block(m_cur_block + 1);
            } else {
                m_val += m_buffer[m_pos_in_block] + 1;
            }
        }

        inline uint64_t find(range const& r, uint64_t lower_bound) {
            uint64_t block_begin = r.begin / Block::block_size;
            uint64_t block_end = r.end / Block::block_size;

            if (UNLIKELY(block_begin != m_cur_block)) {
                decode_block(block_begin);
            }

            uint64_t pos_in_block = r.begin % Block::block_size;
            switch_range(pos_in_block);

            if (UNLIKELY(block_begin != block_end and
                         lower_bound > m_cur_upperbound)) {
                uint64_t block = m_cur_block + 1;
                while (block_upperbound(block) < lower_bound and
                       block != block_end) {
                    ++block;
                }
                assert(block <= block_end);
                decode_block(block);
            }

            while (m_val < lower_bound) {
                m_val += m_buffer[++m_pos_in_block] + 1;
                assert(m_pos_in_block < m_cur_block_size);
            }

            return position();
        }

        uint64_t access(uint64_t pos) {
            uint64_t block = pos / Block::block_size;
            if (UNLIKELY(block != m_cur_block)) {
                decode_block(block);
            }

            while (position() < pos) {
                m_val += m_buffer[++m_pos_in_block] + 1;
            }

            return value();
        }

        void sync(uint64_t pos) {
            m_val = m_buffer[pos];
        }

        uint64_t position() const {
            return m_cur_block * Block::block_size + m_pos_in_block;
        }

        upperbound_type block_upperbound(uint32_t block) const {
            return reinterpret_cast<upperbound_type const*>(
                m_upperbounds)[block];
        }

        void decode_block(uint64_t block) {
            static const uint64_t block_size = Block::block_size;
            endpoint_type endpoint =
                block ? (reinterpret_cast<endpoint_type const*>(
                            m_endpoints))[block - 1]
                      : 0;

            uint8_t const* block_data = m_data + endpoint;

            m_cur_block_size = ((block + 1) * block_size <= m_size)
                                   ? block_size
                                   : (m_size % block_size);

            Block::decode(block_data, &(m_buffer[0]), m_cur_block_size);

            m_cur_upperbound = block_upperbound(block);
            m_cur_block = block;
            m_pos_in_block = 0;
            uint32_t prev = block ? block_upperbound(block - 1) : uint32_t(-1);
            m_val = m_buffer[0] + prev + 1;
        }

    private:
        uint64_t m_size;
        uint64_t m_blocks;
        uint8_t const* m_upperbounds;
        uint8_t const* m_endpoints;
        uint8_t const* m_data;

        uint32_t m_cur_block;
        uint32_t m_pos_in_block;
        uint32_t m_cur_block_size;
        uint32_t m_cur_upperbound;

        uint32_t m_val;
        uint32_t m_buffer[Block::block_size];
    };

    uint64_t size() const {
        return m_size;
    }

    iterator begin() const {
        return iterator(*this);
    }

    iterator end() const {
        return iterator(*this, size());
    }

    iterator at(range const& r, uint64_t pos) const {
        assert(pos >= r.begin);
        iterator it(*this, r.begin);
        uint64_t pos_in_block = r.begin % Block::block_size;
        it.switch_range(pos_in_block);
        it.access(pos);
        return it;
    }

    inline uint64_t access(range const& r, uint64_t pos) {
        uint64_t block = r.begin / Block::block_size;
        uint64_t pos_in_block = r.begin % Block::block_size;
        m_it.decode_block(block);
        m_it.switch_range(pos_in_block);
        return m_it.access(pos);
    }

    uint64_t find(range const& r, uint64_t id) {
        assert(r.end > r.begin);
        assert(r.end <= size());

        if (r.end - r.begin <= global::linear_scan_threshold) {
            auto it = at(r, r.begin);
            for (uint64_t pos = r.begin; pos != r.end; ++pos, ++it) {
                if (*it == id) {
                    return pos;
                }
            }
        }

        return m_it.find(r, id);
    }

    size_t bytes() const {
        return sizeof(m_size) + essentials::vec_bytes(m_data);
    }

    template <typename Visitor>
    void visit(Visitor& visitor) {
        visitor.visit(m_size);
        visitor.visit(m_data);

        if (m_size) {
            m_it = begin();
        }
    }

private:
    uint64_t m_size;
    std::vector<uint8_t> m_data;
    iterator m_it;
};

}  // namespace rdf