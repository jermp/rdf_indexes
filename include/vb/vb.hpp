#pragma once

#include "../external/MaskedVByte/include/varintdecode.h"
#include "../external/MaskedVByte/include/varintencode.h"

namespace rdf {

namespace global {
static const uint64_t block_size = 128;  // 256
}

namespace vb {

struct maskedvbyte_block {
    static const uint64_t block_size = global::block_size;

    static void encode(std::vector<uint8_t>& out, std::vector<uint32_t>& in) {
        std::vector<uint8_t> buf(2 * in.size() * sizeof(uint32_t));
        size_t bytes = vbyte_encode(in.data(), in.size(), buf.data());
        out.insert(out.end(), buf.data(), buf.data() + bytes);
    }

    static uint8_t const* decode(uint8_t const* in, uint32_t* out, uint64_t n) {
        auto read = masked_vbyte_decode(in, out, n);
        return in + read;
    }
};

struct vbyte_block {
    static const uint64_t block_size = global::block_size;

    static void encode(std::vector<uint8_t>& out, std::vector<uint32_t>& in) {
        std::vector<uint8_t> buf(2 * in.size() * sizeof(uint32_t));
        uint8_t* buf_ptr = buf.data();
        for (size_t k = 0; k < in.size(); ++k) {
            const uint32_t val(in[k]);
            if (val < (1U << 7)) {
                *buf_ptr = static_cast<uint8_t>(val | (1U << 7));
                ++buf_ptr;
            } else if (val < (1U << 14)) {
                *buf_ptr = extract7bits<0>(val);
                ++buf_ptr;
                *buf_ptr = extract7bitsmaskless<1>(val) | (1U << 7);
                ++buf_ptr;
            } else if (val < (1U << 21)) {
                *buf_ptr = extract7bits<0>(val);
                ++buf_ptr;
                *buf_ptr = extract7bits<1>(val);
                ++buf_ptr;
                *buf_ptr = extract7bitsmaskless<2>(val) | (1U << 7);
                ++buf_ptr;
            } else if (val < (1U << 28)) {
                *buf_ptr = extract7bits<0>(val);
                ++buf_ptr;
                *buf_ptr = extract7bits<1>(val);
                ++buf_ptr;
                *buf_ptr = extract7bits<2>(val);
                ++buf_ptr;
                *buf_ptr = extract7bitsmaskless<3>(val) | (1U << 7);
                ++buf_ptr;
            } else {
                *buf_ptr = extract7bits<0>(val);
                ++buf_ptr;
                *buf_ptr = extract7bits<1>(val);
                ++buf_ptr;
                *buf_ptr = extract7bits<2>(val);
                ++buf_ptr;
                *buf_ptr = extract7bits<3>(val);
                ++buf_ptr;
                *buf_ptr = extract7bitsmaskless<4>(val) | (1U << 7);
                ++buf_ptr;
            }
        }

        out.insert(out.end(), buf.data(), buf_ptr);
    }

    static uint8_t const* decode(uint8_t const* in, uint32_t* out, uint64_t n) {
        const uint8_t* read = in;
        for (size_t i = 0; i < n; ++i) {
            unsigned int shift = 0;
            for (uint32_t v = 0;; shift += 7) {
                uint8_t c = *read++;
                v += ((c & 127) << shift);
                if ((c & 128)) {
                    *out++ = v;
                    break;
                }
            }
        }
        return read;
    }

private:
    template <uint32_t i>
    static uint8_t extract7bits(const uint32_t val) {
        return static_cast<uint8_t>((val >> (7 * i)) & ((1U << 7) - 1));
    }

    template <uint32_t i>
    static uint8_t extract7bitsmaskless(const uint32_t val) {
        return static_cast<uint8_t>((val >> (7 * i)));
    }
};

}  // namespace vb
}  // namespace rdf