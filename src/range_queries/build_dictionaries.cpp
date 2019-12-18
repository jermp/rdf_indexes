#include <iostream>

#include "../external/essentials/include/essentials.hpp"
#include "util.hpp"
#include "types.hpp"
#include "range_queries_common.hpp"

using namespace rdf;

#define COMPRESS_AND_WRITE                                             \
    T dict;                                                            \
    dict.write(numbers.begin(), numbers.back() + 1, numbers.size());   \
    double giga = essentials::convert(dict.bytes(), essentials::GB);   \
    if (giga < 0.1) {                                                  \
        std::cout << essentials::convert(dict.bytes(), essentials::MB) \
                  << " [MB]" << std::endl;                             \
    } else {                                                           \
        std::cout << giga << " [GB]" << std::endl;                     \
    }                                                                  \
    double bits_per_value = (dict.bytes() * 8.0) / numbers.size();     \
    std::cout << bits_per_value << " [bits per value]" << std::endl;   \
    util::logger("saving data structure to disk...");                  \
    essentials::saver visitor((filename + ".bin").c_str());            \
    visitor.visit(dict_type);                                          \
    visitor.visit(dict);                                               \
    util::logger("DONE");

template <typename T>
void build_numbers_dictionary(std::string const& collection_basename,
                              int dict_type) {
    std::string filename = collection_basename + ".objects_numbers_vocab";
    std::vector<uint64_t> numbers;
    {
        std::ifstream input(filename.c_str());
        if (!input.good()) {
            throw std::runtime_error(
                "Error in opening dictionary file: expected file '" + filename +
                "', but not found.");
        }
        while (input) {
            uint64_t x = 0;
            input >> x;
            numbers.push_back(x);
        }
        input.close();

        std::cout << "read " << numbers.size() << " values" << std::endl;
        assert(std::is_sorted(numbers.begin(), numbers.end()));
    }

    COMPRESS_AND_WRITE
}

// NOTE: assume date is formatted as "yyyy-mm-dd"
uint64_t parse_and_convert_to_uint64(std::string const& date) {
    size_t current = 0, previous = 0;
    current = date.find('-');

    auto substr = date.substr(previous, current - previous);
    uint64_t year = std::stoull(substr);
    previous = current + 1;
    current = date.find('-', previous);

    substr = date.substr(previous, current - previous);
    uint64_t month = std::stoull(substr);
    previous = current + 1;
    current = date.find('-', previous);

    substr = date.substr(previous, current - previous);
    uint64_t day = std::stoull(substr);

    return year * 365 + month * 31 + day;
}

template <typename T>
void build_dates_dictionary(std::string const& collection_basename,
                            int dict_type) {
    std::string filename = collection_basename + ".objects_dates_vocab";
    std::vector<uint64_t> numbers;
    {
        std::ifstream input(filename.c_str());
        if (!input.good()) {
            throw std::runtime_error(
                "Error in opening dictionary file: expected file '" + filename +
                "', but not found.");
        }
        std::string date;
        while (input) {
            input >> date;
            uint64_t x = parse_and_convert_to_uint64(date);
            numbers.push_back(x);
        }
        input.close();

        std::cout << "read " << numbers.size() << " values" << std::endl;
        assert(std::is_sorted(numbers.begin(), numbers.end()));
    }

    COMPRESS_AND_WRITE
}

int main(int argc, char** argv) {
    int mandatory = 3;
    if (argc < mandatory) {
        std::cout << argv[0] << " <type> <collection_basename>" << std::endl;
        return 1;
    }

    std::string type(argv[1]);
    std::string collection_basename(argv[2]);

    if (type == "pef") {
        build_numbers_dictionary<pef::pef_sequence>(collection_basename,
                                                    dictionary_type::pef_type);
        build_dates_dictionary<pef::pef_sequence>(collection_basename,
                                                  dictionary_type::pef_type);
    } else {
        building_util::unknown_type(type);
    }

    return 0;
}
