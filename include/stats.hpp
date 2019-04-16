#pragma once

#include "trie.hpp"
#include "trie_level.hpp"
#include "index_3t.hpp"
#include "index_2to.hpp"
#include "index_2tp.hpp"

namespace rdf {

template <typename SPO, typename POS, typename OSP>
void index_3t<SPO, POS, OSP>::print_stats(essentials::json_lines& stats) {
    std::cout << bytes() * 8.0 / triplets() << " [bpt]" << std::endl;
    std::cout << "================================" << std::endl;
    m_spo.print_stats(stats, bytes());
    std::cout << "================================" << std::endl;
    m_pos.print_stats(stats, bytes());
    std::cout << "================================" << std::endl;
    m_osp.print_stats(stats, bytes());
    std::cout << "================================" << std::endl;
}

template <typename SPO, typename OPS>
void index_2to<SPO, OPS>::print_stats(essentials::json_lines& stats) {
    std::cout << bytes() * 8.0 / triplets() << " [bpt]" << std::endl;
    std::cout << "================================" << std::endl;
    m_spo.print_stats(stats, bytes());
    std::cout << "================================" << std::endl;
    m_ops.print_stats(stats, bytes());
    std::cout << "================================" << std::endl;
}

template <typename SPO, typename POS>
void index_2tp<SPO, POS>::print_stats(essentials::json_lines& stats) {
    std::cout << bytes() * 8.0 / triplets() << " [bpt]" << std::endl;
    std::cout << "================================" << std::endl;
    m_spo.print_stats(stats, bytes());
    std::cout << "================================" << std::endl;
    m_pos.print_stats(stats, bytes());
    std::cout << "================================" << std::endl;
}

template <typename T>
void print_stat(T const& sequence, uint64_t bytes, uint64_t num_triplets) {
    std::cout << "    " << sequence.bytes() * 8.0 / sequence.size()
              << " [bpi]\n"
              << "    " << sequence.bytes() * 8.0 / num_triplets << " [bpt] "
              << "(" << sequence.bytes() * 100.0 / bytes << "%"
              << " - " << sequence.bytes() << " bytes"
              << ")" << std::endl;
}

template <typename T>
void print_size(T const& level, uint64_t bytes, uint64_t num_integers,
                uint64_t num_triplets) {
    std::cout << "integers " << level.size() << "/" << num_integers << " ("
              << level.size() * 100.0 / num_integers << "%)" << std::endl;
    std::cout << "space " << level.bytes() << "/" << bytes << " ("
              << level.bytes() * 100.0 / bytes << "%) - "
              << level.bytes() * 8.0 / num_triplets << " [bpt]" << std::endl;
}

template <typename Pointers>
void collect_ranges_distribution(Pointers const& pointers, int perm,
                                 int level) {
    std::vector<range> ranges;
    ranges.reserve(pointers.size() - 1);
    {
        auto it = pointers.begin();
        uint64_t begin = it.next();
        uint64_t end = 0;
        uint64_t sum = 0;
        for (uint64_t i = 0; i != pointers.size() - 1; ++i) {
            end = it.next();
            ranges.push_back({begin, end});
            sum += end - begin;
            begin = end;
        }
        std::cout << "sum = " << sum << std::endl;
    }

    std::sort(ranges.begin(), ranges.end(), [](auto const& x, auto const& y) {
        return (x.end - x.begin) < (y.end - y.begin);
    });

    std::cout << "max range len = " << ranges.back().end - ranges.back().begin
              << std::endl;

    std::vector<std::pair<uint32_t, uint64_t>  // < range length, frequency >
                >
        data;

    uint32_t len = ranges.front().end - ranges.front().begin;
    uint64_t freq = 0;
    for (auto const& r : ranges) {
        uint32_t renge_len = r.end - r.begin;
        if (renge_len == len) {
            ++freq;
        } else {
            data.emplace_back(len, freq);
            freq = 1;
            len = renge_len;
        }
    }
    data.emplace_back(len, freq);

    std::cout << "num. distinct ranges = " << data.size() << std::endl;
    std::ofstream out("range_distribution." + std::to_string(perm) + "." +
                      std::to_string(level));
    for (auto const& d : data) {
        out << d.first << " " << d.second << "\n";
    }
    out.close();
}

template <typename Nodes, typename Pointers>
void write_level_to_binary(Nodes& nodes, Pointers const& pointers, int perm) {
    auto iterator = typename trie_level<Nodes, Pointers>::iterator(
        nodes.begin(), pointers.begin());
    std::ofstream out("./second_level." + std::to_string(perm) + ".bin",
                      std::ios_base::out | std::ios_base::binary);

    uint64_t n = 0;
    while (true) {
        uint32_t val = *iterator;
        out.write(reinterpret_cast<char const*>(&val), sizeof(uint32_t));
        ++n;
        if (n == nodes.size()) {
            break;
        }
        ++iterator;
    }

    out.close();
}

template <typename Mapper, typename Levels>
void trie<Mapper, Levels>::print_stats(essentials::json_lines& stats,
                                       size_t bytes) {
    stats.new_line();
    assert(id() > 0);
    stats.add("trie", std::to_string(id() - 1));
    util::logger(suffix(id()) + " trie statistics");
    double perc = (*this).bytes() * 100.0 / bytes;
    stats.add("total_space_in_perc", std::to_string(perc));
    std::cout << (*this).bytes() << " [bytes] -- "
              << (*this).bytes() * 8.0 / triplets() << " [bpt]"
              << " (" << perc << "%)" << std::endl;

    {
        std::cout << "first level" << std::endl;
        std::cout << "-----------" << std::endl;
        double bpt = first.bytes() * 8.0 / triplets();
        double perc = first.bytes() * 100.0 / bytes;
        stats.add("level_1_total_space_in_bpt", std::to_string(bpt));
        stats.add("level_1_total_space_in_perc", std::to_string(perc));
        print_size(first, bytes, size(), triplets());
        std::cout << "  pointers" << std::endl;
        print_stat(first.pointers, bytes, triplets());
    }

    {
        std::cout << "second level" << std::endl;
        std::cout << "------------" << std::endl;
        double bpt = second.bytes() * 8.0 / triplets();
        double perc = second.bytes() * 100.0 / bytes;
        stats.add("level_2_total_space_in_bpt", std::to_string(bpt));
        stats.add("level_2_total_space_in_perc", std::to_string(perc));
        print_size(second, bytes, size(), triplets());
        std::cout << "  pointers" << std::endl;
        print_stat(second.pointers, bytes, triplets());
        std::cout << "  nodes" << std::endl;
        print_stat(second.nodes, bytes, triplets());
        collect_ranges_distribution(first.pointers, id(), 2);
    }

    {
        std::cout << "third level" << std::endl;
        std::cout << "-----------" << std::endl;
        double bpt = third.bytes() * 8.0 / triplets();
        double perc = third.bytes() * 100.0 / bytes;
        stats.add("level_3_total_space_in_bpt", std::to_string(bpt));
        stats.add("level_3_total_space_in_perc", std::to_string(perc));
        print_size(third, bytes, size(), triplets());
        std::cout << " nodes" << std::endl;
        print_stat(third.nodes, bytes, triplets());
        collect_ranges_distribution(second.pointers, id(), 3);
    }
}
}  // namespace rdf
