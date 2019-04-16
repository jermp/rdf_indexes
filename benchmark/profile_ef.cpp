#include <iostream>
#include <numeric>
#include <random>

#include "util.hpp"
#include "pef/pef_parameters.hpp"
#include "pef/compact_ef.hpp"

const static uint64_t runs = 5;
const static uint64_t num_queries = 100000;
using namespace rdf;
using namespace essentials;

std::vector<uint64_t> random_sequence(uint64_t n, uint64_t u) {
    std::vector<uint64_t> vec;
    vec.reserve(n);
    uniform_int_rng<uint64_t> r(0, u);
    for (uint64_t i = 0; i != n; ++i) {
        vec.push_back(r.gen());
    }
    return vec;
}

void test_random_access(std::vector<uint64_t> const& sequence,
                        std::vector<uint64_t> const& access_queries,
                        std::vector<uint64_t> const& successor_queries,
                        json_lines& stats, pef::pef_parameters const& params) {
    bit_vector bv;
    {
        bit_vector_builder bvb;
        pef::compact_ef::write(bvb, sequence.begin(), sequence.back() + 1,
                               sequence.size(), params);
        bv.build(&bvb);
    }

    pef::compact_ef::enumerator e(bv, 0, sequence.back() + 1, sequence.size(),
                                  params);
    auto const& offsets = e.of();
    uint64_t data_bits =
        offsets.higher_bits_length + sequence.size() * offsets.lower_bits;
    std::cout << "  data bits " << data_bits << std::endl;
    std::cout << "  total bits " << offsets.end << std::endl;
    std::cout << "  bits x pointer " << offsets.pointer_size << std::endl;
    std::cout << "  num. pointers for access " << offsets.pointers1
              << std::endl;
    std::cout << "  num. pointers for next_geq " << offsets.pointers0
              << std::endl;
    uint64_t pointers1_bits = offsets.pointers1 * offsets.pointer_size;
    uint64_t pointers0_bits = offsets.pointers0 * offsets.pointer_size;
    double perc = pointers1_bits * 100.0 / data_bits;
    stats.add("extra_space_for_access", std::to_string(perc));
    std::cout << "extra space for access pointers " << perc << "%" << std::endl;
    perc = pointers0_bits * 100.0 / data_bits;
    stats.add("extra_space_for_next_geq", std::to_string(perc));
    std::cout << "extra space for next_geq pointers " << perc << "%"
              << std::endl;

    timer_type t;

    {
        for (uint64_t run = 0; run != runs; ++run) {
            t.start();
            for (auto q : access_queries) {
                auto x = e.move(q);
                do_not_optimize_away(x.first);
            }
            t.stop();
        }

        t.discard_min_max();
        double avg = t.average();
        double ns_int = avg / num_queries * 1000;
        std::cout << "  access: " << ns_int << " [ns/int]" << std::endl;
        stats.add("access_time_ns", std::to_string(ns_int));
    }

    t.reset();

    {
        for (uint64_t run = 0; run != runs; ++run) {
            t.start();
            for (auto q : successor_queries) {
                auto x = e.next_geq(q);
                do_not_optimize_away(x.first);
            }
            t.stop();
        }

        t.discard_min_max();
        double avg = t.average();
        double ns_int = avg / num_queries * 1000;
        std::cout << "  next_geq: " << ns_int << " [ns/int]" << std::endl;
        stats.add("next_geq_time_ns", std::to_string(ns_int));
    }
}

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << argv[0] << " <num_integers> <avg_gap>" << std::endl;
        return 1;
    }

    uint64_t n = std::stoull(argv[1]);
    double g = std::stod(argv[2]);
    if (g == 0.0) {
        return 1;
    }

    uint64_t u = n * g;
    std::cout << "universe " << u << std::endl;

    auto sequence = random_sequence(n, u);
    std::sort(sequence.begin(), sequence.end());
    auto access_queries = random_sequence(num_queries, n);
    auto successor_queries = random_sequence(num_queries, u);
    pef::pef_parameters params;
    json_lines stats;

    for (uint64_t q = 1; q <= 10; ++q) {
        std::cout << "testing with q = " << (uint64_t(1) << q) << std::endl;
        params.ef_log_sampling0 = q;
        params.ef_log_sampling1 = q;
        stats.new_line();
        stats.add("q", std::to_string(q));
        test_random_access(sequence, access_queries, successor_queries, stats,
                           params);
    }

    stats.print();
    return 0;
}
