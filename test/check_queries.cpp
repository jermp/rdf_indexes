#include <iostream>

#include "util.hpp"
#include "types.hpp"
#include "util_types.hpp"

using namespace rdf;

triplet prepare_query(triplet const& t, int perm, uint32_t num_wildcards) {
    assert(num_wildcards <= 3);

    if (num_wildcards == 3) {
        return triplet();
    }

    if (num_wildcards == 0) {
        return t;
    }

    triplet query;

    // specialization
    if (perm == permutation_type::ops) {
        query.first = t.third;
        query.third = t.first;

        if (num_wildcards == 1) {
            query.first = global::wildcard_symbol;
        } else if (num_wildcards == 2) {
            query.first = global::wildcard_symbol;
            query.second = global::wildcard_symbol;
        }

        return query;
    }

    // specialization
    if (perm == permutation_type::pso and num_wildcards == 2) {
        query.first = global::wildcard_symbol;
        query.second = t.first;
        query.third = global::wildcard_symbol;
        return query;
    }

    uint64_t* q = &(query.first);
    auto p = reinterpret_cast<uint64_t const*>(&t);

    uint32_t pos = 0;
    for (; pos != 3 - num_wildcards; ++pos) {
        q[(perm - 1 + pos) % 3] = p[pos];
    }
    for (; pos != 3; ++pos) {
        q[(perm - 1 + pos) % 3] = global::wildcard_symbol;
    }

    return query;
}

template <typename Permutation>
void check(parameters const& params, Permutation& permutation,
           const char* filename, int perm, int num_wildcards) {
    std::ifstream input(filename, std::ios_base::in);
    triplets_iterator input_it(input, perm);

    util::logger("checking queries over permutation " + suffix(perm));

    uint64_t n = 0;
    uint64_t quantum = 10000000;
    if (params.num_triplets < quantum) {
        quantum /= 10;
    }

    if (num_wildcards == 0) {
        while (true) {
            triplet expected = *input_it;
            triplet query = prepare_query(expected, perm, num_wildcards);
            uint64_t triplet_id = permutation.is_member(query);
            if (triplet_id == global::not_found) {
                std::cerr << expected << " not found." << std::endl;
            }

            ++n;
            if (n % quantum == 0) {
                std::cout << "checked " << n << "/" << params.num_triplets
                          << " triplets" << std::endl;
            }

            if (n == params.num_triplets) {
                break;
            }
            ++input_it;
        }

    } else {
        while (true) {
            triplet expected = *input_it;
            triplet query = prepare_query(expected, perm, num_wildcards);

            auto query_it = num_wildcards == 3 ? permutation.select_all()
                                               : permutation.select(query);

            while (query_it.has_next()) {
                triplet got = *query_it;
                triplet expected = *input_it;

                if (!util::check(n, params.num_triplets, got, expected)) {
                    return;
                }

                ++n;
                if (n % quantum == 0) {
                    std::cout << "checked " << n << "/" << params.num_triplets
                              << " triplets" << std::endl;
                }

                ++query_it;
                ++input_it;
            }

            if (n == params.num_triplets) {
                break;
            }
        }
    }

    util::logger("checked " + std::to_string(n) + "/" +
                 std::to_string(params.num_triplets) + " triplets");
    util::logger("OK");
    input.close();
}

template <typename Permutation>
void check(parameters const& params, Permutation& permutation, int perm,
           int num_wildcards) {
    std::string filename =
        std::string(params.collection_basename) + "." + suffix(perm);
    check(params, permutation, filename.c_str(), perm, num_wildcards);
}

template <typename Index>
void check(parameters const& params, char const* index_filename) {
    Index index;
    essentials::load(index, index_filename);

    for (int num_wildcards = 3; num_wildcards >= 0; --num_wildcards) {
        std::cout << std::endl;
        util::logger("checking queries with " + std::to_string(num_wildcards) +
                     " wildcards");
        if (num_wildcards == 0 or num_wildcards == 3) {
            check(params, index.spo(), permutation_type::spo, num_wildcards);
            check(params, index.pos(), permutation_type::pos, num_wildcards);
            check(params, index.osp(), permutation_type::osp, num_wildcards);
        } else {
            check(params, index, permutation_type::spo, num_wildcards);
            check(params, index, permutation_type::pos, num_wildcards);
            check(params, index, permutation_type::osp, num_wildcards);
        }
    }
}

// specialization
void check_2to(parameters const& params, char const* index_filename) {
    pef_2to index;
    essentials::load(index, index_filename);

    for (int num_wildcards = 3; num_wildcards >= 0; --num_wildcards) {
        std::cout << std::endl;
        util::logger("checking queries with " + std::to_string(num_wildcards) +
                     " wildcards");
        if (num_wildcards == 0 or num_wildcards == 3) {
            check(params, index.spo(), permutation_type::spo, num_wildcards);
            check(params, index.ops(), permutation_type::ops, num_wildcards);
        } else {
            check(params, index, permutation_type::spo, num_wildcards);
            check(params, index, permutation_type::ops, num_wildcards);
        }
    }

    std::cout << std::endl;
    util::logger("checking queries S?O");
    check(params, index, permutation_type::osp, 1);

    std::cout << std::endl;
    util::logger("checking queries ?P?");
    int perm = permutation_type::pso;
    std::string filename =
        std::string(params.collection_basename) + "." + suffix(perm);
    check(params, index, filename.c_str(), perm, 2);
}

// specialization
void check_2tp(parameters const& params, char const* index_filename) {
    pef_2tp index;
    essentials::load(index, index_filename);

    for (int num_wildcards = 3; num_wildcards >= 0; --num_wildcards) {
        std::cout << std::endl;
        util::logger("checking queries with " + std::to_string(num_wildcards) +
                     " wildcards");
        if (num_wildcards == 0 or num_wildcards == 3) {
            check(params, index.spo(), permutation_type::spo, num_wildcards);
            check(params, index.pos(), permutation_type::pos, num_wildcards);
        } else {
            check(params, index, permutation_type::spo, num_wildcards);
            check(params, index, permutation_type::pos, num_wildcards);
        }
    }

    std::cout << std::endl;
    util::logger("checking queries S?O");
    check(params, index, permutation_type::osp, 1);

    std::cout << std::endl;
    util::logger("checking queries ?O?");
    int perm = permutation_type::pos;
    std::string filename =
        std::string(params.collection_basename) + "." + suffix(perm);
    check(params, index, filename.c_str(), perm, 2);
}

int main(int argc, char** argv) {
    int mandatory = 4;
    if (argc < mandatory) {
        std::cout << argv[0] << " <type> <collection_basename> <index_filename>"
                  << std::endl;
        return 1;
    }

    std::string type(argv[1]);
    parameters params;
    params.collection_basename = argv[2];
    params.load();
    char const* index_filename = argv[3];

    if (type == "pef_2to") {
        check_2to(params, index_filename);
        return 0;
    }

    if (type == "pef_2tp") {
        check_2tp(params, index_filename);
        return 0;
    }

    if (false) {
#define LOOP_BODY(R, DATA, T)                 \
    }                                         \
    else if (type == BOOST_PP_STRINGIZE(T)) { \
        check<T>(params, index_filename);     \
        /**/

        BOOST_PP_SEQ_FOR_EACH(LOOP_BODY, , PERMUTED);
#undef LOOP_BODY
    } else {
        building_util::unknown_type(type);
    }

    return 0;
}
