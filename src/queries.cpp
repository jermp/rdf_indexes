#include <iostream>

#include "../external/essentials/include/essentials.hpp"
#include "types.hpp"
#include "util.hpp"
#include "util_types.hpp"

using namespace rdf;

uint32_t num_runs(uint32_t runs, uint32_t n) {
    static const uint32_t N = 10000;
    uint32_t r = 1;
    if (n * runs < N) {
        r = N / n;
    }
    return r;
}

template <typename Index>
void queries(char const* binary_filename, char const* query_filename, int perm,
             uint32_t runs, uint64_t num_queries, uint64_t num_wildcards,
             bool all) {
    Index index;
    essentials::load(index, binary_filename);
    // essentials::print_size(index);

    essentials::timer_type t;
    uint64_t num_triples = 0;
    double elapsed = 0.0;

    if (all) {
        util::logger("returning all triplets");
        num_queries = 1;

        for (uint64_t run = 0; run != runs; ++run) {
            num_triples = 0;
            auto query_it = index.select_all();
            t.start();
            while (query_it.has_next()) {
                auto t = *query_it;
                essentials::do_not_optimize_away(t.first);
                ++num_triples;
                ++query_it;
            }
            t.stop();
        }
        elapsed = t.average();

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

                auto q = *input_it;

                if (perm == permutation_type::spo) {
                    if (num_wildcards == 1) {
                        q.third = global::wildcard_symbol;
                    } else if (num_wildcards == 2) {
                        q.second = global::wildcard_symbol;
                        q.third = global::wildcard_symbol;
                    }
                } else if (perm == permutation_type::pos) {
                    if (num_wildcards == 1) {
                        q.first = global::wildcard_symbol;
                    } else if (num_wildcards == 2) {
                        q.first = global::wildcard_symbol;
                        q.third = global::wildcard_symbol;
                    }
                } else if (perm == permutation_type::osp) {
                    if (num_wildcards == 1) {
                        q.second = global::wildcard_symbol;
                    } else if (num_wildcards == 2) {
                        q.first = global::wildcard_symbol;
                        q.second = global::wildcard_symbol;
                    }
                }

                queries.push_back(q);
                ++input_it;
            }
            input.close();
            util::logger("loaded " + std::to_string(queries.size()) +
                         " queries");
        }
        assert(num_queries == queries.size());

        util::logger("running queries");

        if (num_wildcards == 0) {
            for (uint64_t run = 0; run != runs; ++run) {
                num_triples = num_queries;
                t.start();
                for (auto query : queries) {
                    essentials::do_not_optimize_away(index.is_member(query));
                }
                t.stop();
            }
            elapsed = t.average();
        } else {
            // for (uint64_t run = 0; run != runs; ++run) {
            //     num_triples = 0;
            //     t.start();
            //     for (auto query : queries) {
            //         auto query_it = index.select(query);
            //         while (query_it.has_next()) {
            //             auto t = *query_it;
            //             essentials::do_not_optimize_away(t.first);
            //             ++num_triples;
            //             ++query_it;
            //         }
            //     }
            //     t.stop();
            // }

            for (auto query : queries) {
                uint64_t n = 0;
                {
                    auto query_it = index.select(query);
                    while (query_it.has_next()) {
                        auto t = *query_it;
                        essentials::do_not_optimize_away(t.first);
                        ++n;
                        ++query_it;
                    }
                }

                uint32_t r = num_runs(runs, n);

                t.start();
                for (uint32_t run = 0; run != r; ++run) {
                    auto query_it = index.select(query);
                    while (query_it.has_next()) {
                        auto t = *query_it;
                        essentials::do_not_optimize_away(t.first);
                        ++query_it;
                    }
                }
                t.stop();
                double avg_per_query = t.elapsed() / r;
                t.reset();
                elapsed += avg_per_query;
                num_triples += n;
            }
        }
    }

    double musecs_per_query = elapsed / num_queries;
    double nanosecs_per_triplet = elapsed / num_triples * 1000;

    std::cout << "\tReturned triples: " << num_triples << "\n";
    std::cout << "\tMean per query: " << musecs_per_query << " [musec]\n ";
    std::cout << "\tMean per triple: " << nanosecs_per_triplet << " [ns]";
    std::cout << std::endl;
}

int main(int argc, char** argv) {
    int mandatory = 4;
    if (argc < mandatory) {
        std::cout << argv[0]
                  << " <type> <perm> <index_filename> [-q <query_filename> -n "
                     "<num_queries> -w <num_wildcards>]"
                  << std::endl;
        return 1;
    }

    bool all = true;
    std::string type(argv[1]);
    int perm = std::stoi(argv[2]);
    char const* index_filename = argv[3];
    char const* query_filename = nullptr;
    uint64_t num_queries = 0;
    uint64_t num_wildcards = 0;

    for (int i = mandatory; i != argc; ++i) {
        if (std::string(argv[i]) == "-q") {
            ++i;
            query_filename = argv[i];
            all = false;
        } else if (!all and std::string(argv[i]) == "-n") {
            ++i;
            num_queries = std::stoull(argv[i]);
        } else if (!all and std::string(argv[i]) == "-w") {
            ++i;
            num_wildcards = std::stoull(argv[i]);
        }
    }

    static const uint32_t runs = 5;

    if (false) {
#define LOOP_BODY(R, DATA, T)                                               \
    }                                                                       \
    else if (type == BOOST_PP_STRINGIZE(T)) {                               \
        queries<T>(index_filename, query_filename, perm, runs, num_queries, \
                   num_wildcards, all);                                     \
        /**/

        BOOST_PP_SEQ_FOR_EACH(LOOP_BODY, , INDEXES);
#undef LOOP_BODY
    } else {
        building_util::unknown_type(type);
    }

    return 0;
}
