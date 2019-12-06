#include <iostream>
#include <numeric>

#include "util.hpp"
#include "types.hpp"
#include "util_types.hpp"

using namespace rdf;
using namespace essentials;

struct query {
    range r;
    uint64_t id;
};

template <typename Permutation>
void queries(Permutation& permutation, char const* query_filename,
             uint32_t runs, uint64_t num_queries, uint64_t num_triplets,
             uint64_t whole_index_bytes, json_lines& stats,
             std::string const& type) {
    std::vector<double> query_timings;
    int perm = permutation.id();

    timer_type t;

    // second level
    {
        stats.new_line();
        stats.add("type", type);
        stats.add("trie", std::to_string(perm));
        stats.add("level", "2");
        double nodes_bpt =
            permutation.second.nodes.bytes() * 8.0 / num_triplets;
        std::cout << "nodes: " << nodes_bpt << " [bpt]" << std::endl;
        std::cout << permutation.second.nodes.bytes() * 100.0 /
                         whole_index_bytes
                  << "%" << std::endl;
        stats.add("nodes_bpt", std::to_string(nodes_bpt));

        // scan
        {
            auto it = typename Permutation::levels_type::second::iterator(
                permutation.second.nodes.begin(),
                permutation.first.pointers.begin());
            uint64_t n = permutation.second.nodes.size();
            t.start();
            for (uint64_t i = 0; i != n; ++i) {
                do_not_optimize_away(*it);
                ++it;
            }
            t.stop();
            double elapsed = t.average();
            double scan = elapsed * 1000 / n;
            std::cout << "scan: " << scan << " [ns/int]" << std::endl;
            stats.add("scan", std::to_string(scan));
        }

        // access
        {
            util::logger("loading queries for access");
            std::vector<query> queries;
            queries.reserve(num_queries);
            {
                std::ifstream input(query_filename, std::ios_base::in);
                triplets_iterator input_it(input);
                for (uint64_t i = 0; i != num_queries; ++i) {
                    if (!input_it.has_next()) {
                        break;
                    }
                    triplet t = *input_it;
                    util::permute(t, perm);
                    auto r = permutation.first.pointers[t.first];
                    query q;
                    q.r = r;
                    q.id = t.second;
                    uint64_t pos = permutation.second.nodes.find(q.r, q.id);
                    assert(pos != global::not_found);
                    q.id = pos;
                    queries.push_back(q);
                    ++input_it;
                }
                input.close();
                util::logger("loaded " + std::to_string(queries.size()) +
                             " queries");
            }
            assert(num_queries == queries.size());

            t.reset();

            util::logger("running queries");
            for (uint64_t run = 0; run != runs; ++run) {
                t.start();
                for (auto q : queries) {
                    do_not_optimize_away(
                        permutation.second.nodes.access(q.r, q.id));
                }
                t.stop();
            }

            t.discard_min_max();
            double avg = t.average();
            double access = avg / num_queries * 1000;
            std::cout << "access: " << access << " [ns/int]" << std::endl;
            stats.add("access", std::to_string(access));
        }

        // find
        {
            util::logger("loading queries for find");

            std::vector<query> queries;
            queries.reserve(num_queries);
            {
                std::ifstream input(query_filename, std::ios_base::in);
                triplets_iterator input_it(input);
                for (uint64_t i = 0; i != num_queries; ++i) {
                    if (!input_it.has_next()) {
                        break;
                    }
                    triplet t = *input_it;
                    util::permute(t, perm);
                    auto r = permutation.first.pointers[t.first];
                    query q;
                    q.id = t.second;
                    q.r = r;
                    queries.push_back(q);
                    ++input_it;
                }
                input.close();
                util::logger("loaded " + std::to_string(queries.size()) +
                             " queries");
            }
            assert(num_queries == queries.size());

            t.reset();

            util::logger("running queries");
            for (uint64_t run = 0; run != runs; ++run) {
                t.start();
                for (auto const& q : queries) {
                    do_not_optimize_away(
                        permutation.second.nodes.find(q.r, q.id));
                }
                t.stop();
            }

            t.discard_min_max();
            double avg = t.average();
            double find = avg / num_queries * 1000;
            std::cout << "find: " << find << " [ns/int]" << std::endl;
            stats.add("find", std::to_string(find));
        }
    }

    // third level
    {
        stats.new_line();
        stats.add("type", type);
        stats.add("trie", std::to_string(perm));
        stats.add("level", "3");
        double nodes_bpt = permutation.third.nodes.bytes() * 8.0 / num_triplets;
        std::cout << "nodes: " << nodes_bpt << " [bpt]" << std::endl;
        std::cout << permutation.third.nodes.bytes() * 100.0 / whole_index_bytes
                  << "%" << std::endl;
        stats.add("nodes_bpt", std::to_string(nodes_bpt));

        // scan
        {
            t.reset();
            auto it = typename Permutation::levels_type::third::iterator(
                permutation.third.nodes.begin(),
                permutation.second.pointers.begin());
            uint64_t n = permutation.third.nodes.size();
            t.start();
            for (uint64_t i = 0; i != n; ++i) {
                do_not_optimize_away(*it);
                ++it;
            }
            t.stop();
            double elapsed = t.average();
            double scan = elapsed * 1000 / n;
            std::cout << "scan: " << scan << " [ns/int]" << std::endl;
            stats.add("scan", std::to_string(scan));
        }

        // access
        {
            util::logger("loading queries for access");
            std::vector<query> queries;
            queries.reserve(num_queries);
            {
                std::ifstream input(query_filename, std::ios_base::in);
                triplets_iterator input_it(input);
                for (uint64_t i = 0; i != num_queries; ++i) {
                    if (!input_it.has_next()) {
                        break;
                    }

                    triplet t = *input_it;
                    util::permute(t, perm);
                    auto r = permutation.first.pointers[t.first];
                    uint64_t pos = permutation.second.nodes.find(r, t.second);
                    r = permutation.second.pointers[pos];

                    query q;
                    q.r = r;
                    q.id = t.third;
                    pos = permutation.third.nodes.find(q.r, q.id);
                    assert(pos != global::not_found);
                    q.id = pos;
                    queries.push_back(q);
                    ++input_it;
                }
                input.close();
                util::logger("loaded " + std::to_string(queries.size()) +
                             " queries");
            }
            assert(num_queries == queries.size());

            t.reset();

            util::logger("running queries");
            for (uint64_t run = 0; run != runs; ++run) {
                t.start();
                for (auto q : queries) {
                    do_not_optimize_away(
                        permutation.third.nodes.access(q.r, q.id));
                }
                t.stop();
            }

            t.discard_min_max();
            double avg = t.average();
            double access = avg / num_queries * 1000;
            std::cout << "access: " << access << " [ns/int]" << std::endl;
            stats.add("access", std::to_string(access));
        }

        // find
        {
            util::logger("loading queries for find");

            std::vector<query> queries;
            queries.reserve(num_queries);
            {
                std::ifstream input(query_filename, std::ios_base::in);
                triplets_iterator input_it(input);
                for (uint64_t i = 0; i != num_queries; ++i) {
                    if (!input_it.has_next()) {
                        break;
                    }

                    triplet t = *input_it;
                    util::permute(t, perm);
                    auto r = permutation.first.pointers[t.first];
                    uint64_t pos = permutation.second.nodes.find(r, t.second);
                    r = permutation.second.pointers[pos];

                    query q;
                    q.r = r;
                    q.id = t.third;
                    queries.push_back(q);
                    ++input_it;
                }
                input.close();
                util::logger("loaded " + std::to_string(queries.size()) +
                             " queries");
            }
            assert(num_queries == queries.size());

            t.reset();

            util::logger("running queries");
            for (uint64_t run = 0; run != runs; ++run) {
                t.start();
                for (auto const& q : queries) {
                    do_not_optimize_away(
                        permutation.third.nodes.find(q.r, q.id));
                }
                t.stop();
            }

            t.discard_min_max();
            double avg = t.average();
            double find = avg / num_queries * 1000;
            std::cout << "find: " << find << " [ns/int]" << std::endl;
            stats.add("find", std::to_string(find));
        }
    }
}

template <typename Index>
void queries(char const* binary_filename, char const* query_filename,
             uint32_t runs, uint64_t num_queries, json_lines& stats,
             std::string const& type) {
    Index index;
    load(index, binary_filename);

    queries(index.spo(), query_filename, runs, num_queries, index.triplets(),
            index.bytes(), stats, type);

    queries(index.pos(), query_filename, runs, num_queries, index.triplets(),
            index.bytes(), stats, type);

    queries(index.osp(), query_filename, runs, num_queries, index.triplets(),
            index.bytes(), stats, type);
}

int main(int argc, char** argv) {
    static const uint32_t runs = 5;
    int mandatory = 3;

    if (argc < mandatory) {
        std::cout
            << argv[0]
            << " <type> <index_filename> -q <query_filename> -n <num_queries>"
            << std::endl;
        return 1;
    }

    std::string type(argv[1]);  // compact_3t ef_3t pef_3t vb_3t
    char const* index_filename = argv[2];
    char const* query_filename = nullptr;
    uint64_t num_queries = 0;

    for (int i = 0; i != mandatory; ++i) {
        std::cout << argv[i] << " ";
    }

    for (int i = mandatory; i != argc; ++i) {
        std::cout << argv[i] << " ";
        if (std::string(argv[i]) == "-q") {
            ++i;
            query_filename = argv[i];
            std::cout << argv[i] << " ";
        } else if (std::string(argv[i]) == "-n") {
            ++i;
            num_queries = std::stoull(argv[i]);
            std::cout << argv[i] << " ";
        }
    }

    std::cout << std::endl;

    json_lines stats;

    if (type == "compact_3t") {
        queries<compact_3t>(index_filename, query_filename, runs, num_queries,
                            stats, type);
    } else if (type == "ef_3t") {
        queries<ef_3t>(index_filename, query_filename, runs, num_queries, stats,
                       type);
    } else if (type == "pef_3t") {
        queries<pef_3t>(index_filename, query_filename, runs, num_queries,
                        stats, type);
    } else if (type == "vb_3t") {
        queries<vb_3t>(index_filename, query_filename, runs, num_queries, stats,
                       type);
    } else if (type == "pef_r_3t") {
        queries<pef_r_3t>(index_filename, query_filename, runs, num_queries,
                          stats, type);
    } else {
        building_util::unknown_type(type);
    }

    stats.print();

    return 0;
}
