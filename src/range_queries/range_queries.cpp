#include <iostream>

#include "../external/essentials/include/essentials.hpp"
#include "types.hpp"
#include "util.hpp"
#include "util_types.hpp"
#include "range_queries_common.hpp"

using namespace rdf;

uint32_t num_runs(uint32_t runs, uint32_t n) {
    static const uint32_t N = 10000;
    uint32_t r = 1;
    if (n * runs < N) r = N / n;
    return r;
}

struct range_query_type {
    uint64_t predicate;
    uint64_t lower_bound, upper_bound;
};

int read_dictionary_type(essentials::loader& visitor) {
    int dict_type = 0;
    visitor.visit(dict_type);
    return dict_type;
}

template <typename Dictionary>
void range_queries(std::string const& index_filename,
                   std::string const& query_filename, uint32_t runs,
                   uint32_t num_queries, Dictionary& dictionary) {
    pef_2tp index;
    essentials::load(index, index_filename.c_str());

    essentials::timer_type timer;
    uint64_t num_triples = 0;
    double elapsed = 0.0;

    util::logger("loading queries");
    std::vector<range_query_type> queries;
    queries.reserve(num_queries);
    {
        std::ifstream input(query_filename, std::ios_base::in);
        for (uint32_t i = 0; i != num_queries; ++i) {
            if (!input) break;
            range_query_type rqt;
            input >> rqt.predicate;
            input >> rqt.lower_bound;
            input >> rqt.upper_bound;
            queries.push_back(rqt);
        }
        input.close();
        util::logger("loaded " + std::to_string(queries.size()) + " queries");
        assert(num_queries == queries.size());

        util::logger("running queries");

        for (auto query : queries) {
            triplet t;
            t.second = query.predicate;
            uint64_t n = 0;
            {
                auto query_it = index.select_range(
                    t, query.lower_bound, query.upper_bound, dictionary);
                while (query_it.has_next()) {
                    ++n;
                    ++query_it;
                }
            }

            uint32_t r = num_runs(runs, n);

            timer.start();
            for (uint32_t run = 0; run != r; ++run) {
                auto query_it = index.select_range(
                    t, query.lower_bound, query.upper_bound, dictionary);
                while (query_it.has_next()) {
                    auto res = *query_it;
                    essentials::do_not_optimize_away(res.first);
                    ++query_it;
                }
            }
            timer.stop();

            double avg_per_query = timer.elapsed() / r;
            timer.reset();
            elapsed += avg_per_query;
            num_triples += n;
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
                  << " <pef_2tp_index_filename> <dictionary_filename>"
                     " <query_filename> <num_queries>"
                  << std::endl;
        return 1;
    }

    std::string index_filename(argv[1]);
    std::string dictionary_filename(argv[2]);
    std::string query_filename(argv[3]);
    uint32_t num_queries = std::atoi(argv[4]);

    static const uint32_t runs = 5;

    essentials::loader visitor(dictionary_filename.c_str());
    int dict_type = read_dictionary_type(visitor);

    if (dict_type == dictionary_type::pef_type) {
        pef::pef_sequence dictionary;
        visitor.visit(dictionary);
        range_queries(index_filename, query_filename, runs, num_queries,
                      dictionary);
    } else {
        return 1;
    }

    return 0;
}
