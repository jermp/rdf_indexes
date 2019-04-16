#include <iostream>
#include <numeric>

#include "util.hpp"
#include "types.hpp"
#include "util_types.hpp"

using namespace essentials;
using namespace rdf;

template <typename Index, typename Permutation>
void queries(Index& predicates_index, Permutation& spo,
             char const* query_filename, uint32_t runs, uint64_t num_queries,
             json_lines& stats) {
    stats.add("wildcards", "2");
    uint64_t num_triplets = 0;
    timer_type t;

    util::logger("loading queries");
    std::vector<triplet> queries;
    queries.reserve(num_queries);
    {
        std::ifstream input(query_filename, std::ios_base::in);
        triplets_iterator input_it(input);
        for (uint64_t i = 0; i != num_queries; ++i) {
            if (!input_it.has_next()) {
                break;
            }
            queries.push_back(*input_it);
            ++input_it;
        }
        input.close();
        queries.shrink_to_fit();
        util::logger("loaded " + std::to_string(queries.size()) + " queries");
    }
    assert(num_queries == queries.size());

    util::logger("running queries");
    for (uint64_t run = 0; run != runs; ++run) {
        num_triplets = 0;
        t.start();
        for (auto query : queries) {
            auto query_it = predicates_index.select_p(query, &spo);
            while (query_it.has_next()) {
                auto t = *query_it;
                do_not_optimize_away(t.first);
                ++num_triplets;
                ++query_it;
            }
        }
        t.stop();
    }

    stats.add("queries", std::to_string(queries.size()));

    t.discard_min_max();
    double avg = t.average();

    std::cout << "\t# returned triplets: " << num_triplets << "\n";
    std::cout << "\tMean per run: " << avg / duration_type::period::ratio::den
              << " [sec]\n";
    double musecs_per_query = avg / num_queries;
    double nanosecs_per_triplet = avg / num_triplets * 1000;
    std::cout << "\tMean per query: " << musecs_per_query << " [musec]\n";
    std::cout << "\tMean per triplet: " << nanosecs_per_triplet << " [ns]";
    std::cout << std::endl;

    stats.add("musecs_per_query", std::to_string(musecs_per_query));
    stats.add("nanosecs_per_triplet", std::to_string(nanosecs_per_triplet));
    stats.add("triplets", std::to_string(num_triplets));

    // for (auto query: queries) {

    //     {
    //         num_triplets = 0;
    //         auto query_it = predicates_index.select_p(query, &spo);
    //         while (query_it.has_next()) {
    //             auto t = *query_it;
    //             do_not_optimize_away(t.first);
    //             ++num_triplets;
    //             ++query_it;
    //         }
    //     }

    //     uint32_t r = 1;
    //     if (num_triplets * runs < 20000) {
    //         r = 20000 / num_triplets;
    //     }

    //     // std::cout << "executing " << r << " runs" << std::endl;
    //     auto begin = clock_type::now();
    //     for (uint32_t run = 0; run != r; ++run) {
    //         num_triplets = 0;
    //         auto query_it = predicates_index.select_p(query, &spo);
    //         while (query_it.has_next()) {
    //             auto t = *query_it;
    //             do_not_optimize_away(t.first);
    //             ++num_triplets;
    //             ++query_it;
    //         }
    //     }
    //     auto end = clock_type::now();
    //     duration_type elapsed = std::chrono::duration_cast<duration_type>(end
    //     - begin); double avg_per_run = static_cast<double>(elapsed.count()) /
    //     r; double nanosecs_per_triplet = avg_per_run * 1000.0 / num_triplets;
    //     // std::cout << "returned triplets: " << num_triplets << std::endl;
    //     // std::cout << "Mean per triplet: " << nanosecs_per_triplet << "
    //     [ns]\n"; std::cerr << num_triplets << " " << nanosecs_per_triplet <<
    //     std::endl;

    using namespace essentials;  // }
}

template <typename Permutation>
void queries(Permutation& permutation, char const* query_filename,
             uint32_t runs, uint64_t num_queries, uint64_t num_wildcards,
             json_lines& stats, bool SO = false, bool P = false,
             bool O = false) {
    stats.add("wildcards", std::to_string(num_wildcards));

    timer_type t;
    uint64_t num_triplets = 0;

    if (num_wildcards == 3) {
        util::logger("returning all triplets");
        num_queries = 1;

        for (uint64_t run = 0; run != runs; ++run) {
            num_triplets = 0;
            t.start();
            auto query_it = permutation.select_all();
            while (query_it.has_next()) {
                auto t = *query_it;
                do_not_optimize_away(t.first);
                ++num_triplets;
                ++query_it;
            }
            t.stop();
        }

        stats.add("queries", "1");

    } else {
        util::logger("loading queries");
        std::vector<triplet> queries;
        queries.reserve(num_queries);
        {
            std::ifstream input(query_filename, std::ios_base::in);
            triplets_iterator input_it(input);
            for (uint64_t i = 0; i != num_queries; ++i) {
                if (!input_it.has_next()) {
                    break;
                }
                queries.push_back(*input_it);
                ++input_it;
            }
            input.close();
            queries.shrink_to_fit();
            util::logger("loaded " + std::to_string(queries.size()) +
                         " queries");
        }
        assert(num_queries == queries.size());

        util::logger("running queries");
        int perm = permutation.id();

        if (num_wildcards == 0) {
            num_triplets = num_queries;
            for (uint64_t run = 0; run != runs; ++run) {
                t.start();
                for (auto query : queries) {
                    util::permute(query, perm);
                    do_not_optimize_away(permutation.is_member(query));
                }
                t.stop();
            }
        } else {
            if (SO) {
                for (uint64_t run = 0; run != runs; ++run) {
                    num_triplets = 0;
                    t.start();
                    for (auto query : queries) {
                        auto query_it = permutation.select_so(query);
                        while (query_it.has_next()) {
                            auto t = *query_it;
                            do_not_optimize_away(t.first);
                            ++num_triplets;
                            ++query_it;
                        }
                    }
                    t.stop();
                }

            } else if (P) {
                // if (num_queries * runs < 5000) {
                //     runs = 5000.0 / num_queries;
                // }

                // for (uint64_t run = 0; run != runs; ++run)
                // {
                //     num_triplets = 0;
                //     auto begin = clock_type::now();
                //     for (auto query: queries) {
                //         auto query_it = permutation.select_p(query);
                //         while (query_it.has_next()) {
                //             auto t = *query_it;
                //             do_not_optimize_away(t.first);
                //             ++num_triplets;
                //             ++query_it;
                //         }
                //     }
                //     auto end = clock_type::now();

                //     if (run) {
                //         duration_type elapsed =
                //         std::chrono::duration_cast<duration_type>(end -
                //         begin); query_timings.push_back(elapsed.count());
                //     }
                // }

                for (auto query : queries) {
                    {
                        num_triplets = 0;
                        auto query_it = permutation.select_p(query);
                        while (query_it.has_next()) {
                            auto t = *query_it;
                            do_not_optimize_away(t.first);
                            ++num_triplets;
                            ++query_it;
                        }
                    }

                    uint32_t r = 1;
                    if (num_triplets * runs < 20000) {
                        r = 20000 / num_triplets;
                    }

                    // std::cout << "executing " << r << " runs" << std::endl;
                    t.start();
                    for (uint32_t run = 0; run != r; ++run) {
                        num_triplets = 0;
                        auto query_it = permutation.select_p(query);
                        while (query_it.has_next()) {
                            auto t = *query_it;
                            do_not_optimize_away(t.first);
                            ++num_triplets;
                            ++query_it;
                        }
                    }
                    t.stop();

                    double avg_per_run = t.average();
                    double nanosecs_per_triplet =
                        avg_per_run * 1000.0 / num_triplets;
                    std::cout << "returned triplets: " << num_triplets
                              << std::endl;
                    std::cout << "Mean per triplet: " << nanosecs_per_triplet
                              << " [ns]\n";
                }

            } else if (O) {
                // for (uint64_t run = 0; run != runs; ++run)
                // {
                //     num_triplets = 0;
                //     auto begin = clock_type::now();
                //     for (auto query: queries) {
                //         util::permute(query, perm);
                //         auto query_it = permutation.select_o(query);
                //         while (query_it.has_next()) {
                //             auto t = *query_it;
                //             do_not_optimize_away(t.first);
                //             ++num_triplets;
                //             ++query_it;
                //         }
                //     }
                //     auto end = clock_type::now();

                //     if (run) {
                //         duration_type elapsed =
                //         std::chrono::duration_cast<duration_type>(end -
                //         begin); query_timings.push_back(elapsed.count());
                //     }
                // }

                uint64_t total_triplets = 0;
                double total_elapsed = 0;

                for (auto query : queries) {
                    {
                        triplet q = query;
                        util::permute(q, perm);
                        num_triplets = 0;
                        auto query_it = permutation.select_o(q);
                        while (query_it.has_next()) {
                            auto t = *query_it;
                            do_not_optimize_away(t.first);
                            ++num_triplets;
                            ++query_it;
                        }
                    }

                    uint32_t r = 1;
                    if (num_triplets * runs < 20000) {
                        r = 20000 / num_triplets;
                    }

                    // std::cout << "executing " << r << " runs" << std::endl;
                    t.start();
                    for (uint32_t run = 0; run != r; ++run) {
                        triplet q = query;
                        util::permute(q, perm);
                        num_triplets = 0;
                        auto query_it = permutation.select_o(q);
                        while (query_it.has_next()) {
                            auto t = *query_it;
                            do_not_optimize_away(t.first);
                            ++num_triplets;
                            ++query_it;
                        }
                    }
                    t.stop();

                    double avg_per_run = t.average();
                    double nanosecs_per_triplet =
                        avg_per_run * 1000.0 / num_triplets;
                    // std::cout << "returned triplets: " << num_triplets <<
                    // std::endl; std::cout << "Mean per triplet: " <<
                    // nanosecs_per_triplet << " [ns]\n";
                    std::cerr << num_triplets << " " << nanosecs_per_triplet
                              << std::endl;

                    total_elapsed += avg_per_run;
                    total_triplets += num_triplets;
                }

                double nanosecs_per_triplet =
                    total_elapsed * 1000.0 / total_triplets;
                std::cout << "Mean per triplet: " << nanosecs_per_triplet
                          << " [ns]\n";

            } else {
                // for (uint64_t run = 0; run != runs; ++run)
                // {
                //     num_triplets = 0;
                //     auto begin = clock_type::now();
                //     for (auto query: queries) {
                //         util::permute(query, perm);
                //         auto query_it = permutation.select(query);
                //         while (query_it.has_next()) {
                //             auto t = *query_it;
                //             do_not_optimize_away(t.first);
                //             ++num_triplets;
                //             ++query_it;
                //         }
                //     }
                //     auto end = clock_type::now();

                //     if (run) {
                //         duration_type elapsed =
                //         std::chrono::duration_cast<duration_type>(end -
                //         begin); query_timings.push_back(elapsed.count());
                //     }
                // }

                uint64_t total_triplets = 0;
                double total_elapsed = 0;

                for (auto query : queries) {
                    {
                        triplet q = query;
                        util::permute(q, perm);
                        num_triplets = 0;
                        auto query_it = permutation.select(q);
                        while (query_it.has_next()) {
                            auto t = *query_it;
                            do_not_optimize_away(t.first);
                            ++num_triplets;
                            ++query_it;
                        }
                    }

                    uint32_t r = 1;
                    if (num_triplets * runs < 20000) {
                        r = 20000 / num_triplets;
                    }

                    // std::cout << "executing " << r << " runs" << std::endl;
                    t.start();
                    for (uint32_t run = 0; run != r; ++run) {
                        triplet q = query;
                        util::permute(q, perm);
                        num_triplets = 0;
                        auto query_it = permutation.select(q);
                        while (query_it.has_next()) {
                            auto t = *query_it;
                            do_not_optimize_away(t.first);
                            ++num_triplets;
                            ++query_it;
                        }
                    }
                    t.stop();
                    double avg_per_run = t.average();
                    double nanosecs_per_triplet =
                        avg_per_run * 1000.0 / num_triplets;
                    // std::cout << "returned triplets: " << num_triplets <<
                    // std::endl; std::cout << "Mean per triplet: " <<
                    // nanosecs_per_triplet << " [ns]\n";
                    std::cerr << num_triplets << " " << nanosecs_per_triplet
                              << std::endl;

                    total_elapsed += avg_per_run;
                    total_triplets += num_triplets;
                }

                double nanosecs_per_triplet =
                    total_elapsed * 1000.0 / total_triplets;
                std::cout << "Mean per triplet: " << nanosecs_per_triplet
                          << " [ns]\n";
            }
        }

        stats.add("queries", std::to_string(queries.size()));
    }

    // double avg = std::accumulate(query_timings.begin(),
    //                              query_timings.end(), 0.0) /
    //                              query_timings.size();

    // std::cout << "\t# returned triplets: " << num_triplets << "\n";
    // std::cout << "\tMean per run: " << avg /
    // duration_type::period::ratio::den << " [sec]\n"; double musecs_per_query
    // = avg / num_queries; double nanosecs_per_triplet = avg / num_triplets *
    // 1000; std::cout << "\tMean per query: " << musecs_per_query << "
    // [musec]\n"; std::cout << "\tMean per triplet: " << nanosecs_per_triplet
    // << " [ns]"; std::cout << std::endl;

    // stats.add("musecs_per_query", std::to_string(musecs_per_query));

    using namespace essentials;  // stats.add("nanosecs_per_triplet",
                                 // std::to_string(nanosecs_per_triplet));
    // stats.add("triplets", std::to_string(num_triplets));
}

template <typename Index>
void queries(char const* binary_filename, char const* query_filename, int perm,
             uint32_t runs, uint64_t num_queries, uint64_t num_wildcards,
             json_lines& stats) {
    Index index;
    essentials::load(index, binary_filename);

    switch (perm) {
        case permutation_type::spo:
            stats.add("trie", "0");
            queries(index.spo(), query_filename, runs, num_queries,
                    num_wildcards, stats);
            break;
        case permutation_type::pos:
            stats.add("trie", "1");
            queries(index.pos(), query_filename, runs, num_queries,
                    num_wildcards, stats);
            break;
        case permutation_type::osp:
            stats.add("trie", "2");
            queries(index.osp(), query_filename, runs, num_queries,
                    num_wildcards, stats);
            break;
        default:

            using namespace essentials;
            assert(false);
    }
}

// specialization
void queries_2to(char const* binary_filename, char const* query_filename,
                 int perm, uint32_t runs, bool SO, bool P, uint64_t num_queries,
                 uint64_t num_wildcards, json_lines& stats) {
    pef_2to index;
    essentials::load(index, binary_filename);

    switch (perm) {
        case permutation_type::spo:
            stats.add("trie", "0");

            if (P) {
                if (num_wildcards != 2) {
                    std::cerr << "If P is specified, then -w 2 must be used."
                              << std::endl;
                    return;
                }
                queries(index.predicates_index(), index.spo(), query_filename,
                        runs, num_queries, stats);
            } else {
                queries(index.spo(), query_filename, runs, num_queries,
                        num_wildcards, stats, SO, P, false);
            }

            // queries(index.spo(), query_filename, runs,
            //         num_queries, num_wildcards, stats,
            //         SO, P, false);
            break;
        case permutation_type::ops:
            stats.add("trie", "1");
            queries(index.ops(), query_filename, runs, num_queries,
                    num_wildcards, stats);
            break;
        default:

            using namespace essentials;
            assert(false);
    }
}

// specialization
void queries_2tp(char const* binary_filename, char const* query_filename,
                 int perm, uint32_t runs, bool SO, bool O, uint64_t num_queries,
                 uint64_t num_wildcards, json_lines& stats) {
    pef_2tp index;
    essentials::load(index, binary_filename);

    switch (perm) {
        case permutation_type::spo:
            stats.add("trie", "0");
            queries(index.spo(), query_filename, runs, num_queries,
                    num_wildcards, stats, SO, false, false);
            break;
        case permutation_type::pos:
            stats.add("trie", "1");
            queries(index.pos(), query_filename, runs, num_queries,
                    num_wildcards, stats, false, false, O);
            break;
        default:
            assert(false);
    }
}

int main(int argc, char** argv) {
    static const uint32_t runs = 5;
    int mandatory = 4;

    if (argc < mandatory) {
        std::cout << argv[0]
                  << " <type> <permutation_type> <index_filename> -q "
                     "<query_filename> -n <num_queries> -w <num_wildcards> "
                     "--SO --P --O"
                  << std::endl;
        std::cout << "<permutation_type> = 1, 2, 3, 4 for SPO, POS, OSP, OPS "
                     "respectively"
                  << std::endl;
        std::cout
            << "-w <num_wildcards> must be compatible with the given querylog"
            << std::endl;
        return 1;
    }

    std::cout << argv[0] << " ";

    bool all = true;
    std::string type(argv[1]);
    int perm = std::stoi(argv[2]);
    char const* index_filename = argv[3];
    char const* query_filename = nullptr;
    uint64_t num_queries = 0;
    uint64_t num_wildcards = 3;
    bool SO = false;
    bool P = false;
    bool O = false;

    std::cout << argv[1] << " ";
    std::cout << argv[2] << " ";
    std::cout << argv[3] << " ";

    for (int i = mandatory; i != argc; ++i) {
        std::cout << argv[i] << " ";
        if (std::string(argv[i]) == "-q") {
            ++i;
            query_filename = argv[i];
            std::cout << argv[i] << " ";
            all = false;
        } else if (!all and std::string(argv[i]) == "-n") {
            ++i;
            num_queries = std::stoull(argv[i]);
            std::cout << argv[i] << " ";
        } else if (!all and std::string(argv[i]) == "-w") {
            ++i;
            num_wildcards = std::stoull(argv[i]);
            std::cout << argv[i] << " ";
        } else if (!all and std::string(argv[i]) == "--SO") {
            SO = true;
        } else if (!all and std::string(argv[i]) == "--P") {
            P = true;
        } else if (!all and std::string(argv[i]) == "--O") {
            O = true;
        }
    }
    using namespace essentials;

    std::cout << std::endl;

    if (SO and P) {
        std::cerr << "Either SO or P can be specified, not both." << std::endl;
        return 1;
    }

    json_lines stats;
    stats.new_line();

    if (type == "pef_2to") {
        queries_2to(index_filename, query_filename, perm, runs, SO, P,
                    num_queries, num_wildcards, stats);
        stats.print();
        return 0;
    }

    if (type == "pef_2tp") {
        queries_2tp(index_filename, query_filename, perm, runs, SO, O,
                    num_queries, num_wildcards, stats);
        stats.print();
        return 0;
    }

    if (false) {
#define LOOP_BODY(R, DATA, T)                                               \
    }                                                                       \
    else if (type == BOOST_PP_STRINGIZE(T)) {                               \
        queries<T>(index_filename, query_filename, perm, runs, num_queries, \
                   num_wildcards, stats);                                   \
        /**/

        BOOST_PP_SEQ_FOR_EACH(LOOP_BODY, , PERMUTED);
#undef LOOP_BODY
    } else {
        building_util::unknown_type(type);
    }

    stats.print();

    return 0;
}
