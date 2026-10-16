// Production persona module: a small C API over persona-generator, for the
// npm package in package/ (the game imports that, not the demo page).
//
// Every call returns a JSON document, a persona or {"error": "..."}; the
// pointer stays valid until the next call. String inputs are UTF-8 and an
// empty string leaves the choice to the generator.

#include "cpp23_polyfill.hpp"
#include <dasmig/personagen.hpp>
#include <emscripten/emscripten.h>
#include "demo_support.hpp"

#include <cmath>
#include <exception>
#include <string>

namespace
{

std::string result_; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

const char* reply(std::string json)
{
    result_ = std::move(json);
    return result_.c_str();
}

// The libraries' lite data is compiled in; city's admin1 names are embedded
// in the module's file system (--embed-file), so nothing is fetched.
void load_once()
{
    auto& cg = dasmig::cg::instance();
    if (!cg.has_admin1()) cg.load_admin1("/resources/city/admin1CodesASCII.txt");
}

std::string hex(std::uint64_t value)
{
    static constexpr char digits[] = "0123456789abcdef";
    std::string out(16, '0');
    for (int i = 15; i >= 0; --i, value >>= 4) out[static_cast<std::size_t>(i)] = digits[value & 0xf];
    return out;
}

// The request the C API's string and number arguments describe; "" and -1
// leave a choice to the generator.
dasmig::persona_request make_request(const char* country, const char* sex, int age_min, int age_max, int traits,
                                     const char* reference_date)
{
    dasmig::persona_request request;
    if (*country != '\0') request.country = country;
    const std::string_view sex_text{sex};
    if (sex_text == "female") request.person_sex = dasmig::sex::female;
    else if (sex_text == "male") request.person_sex = dasmig::sex::male;
    else if (!sex_text.empty()) throw std::invalid_argument("sex must be \"male\", \"female\" or empty");
    if (age_min >= 0 || age_max >= 0)
    {
        if (age_min < 0 || age_max < 0 || age_min > 120 || age_max > 120)
            throw std::invalid_argument("ages must both be between 0 and 120");
        request.ages = dasmig::age_range{.min = static_cast<std::uint8_t>(age_min),
                                         .max = static_cast<std::uint8_t>(age_max)};
    }
    if (traits >= 0) request.traits = static_cast<std::size_t>(traits);
    if (*reference_date != '\0') request.reference_date = parse_reference_date(reference_date);
    return request;
}

} // namespace

extern "C"
{

/// @brief Generate a persona.
/// @param country ISO 3166-1 alpha-2 code (any case), or "" to draw one of
///        the supported countries, weighted by population.
/// @param sex "male", "female", or "" to draw it.
/// @param age_min, age_max Inclusive age range in completed years (0-120),
///        or -1 for both to draw from the country's age pyramid.
/// @param traits Number of personality traits, or -1 for the default (3).
/// @param reference_date The date ages are computed at, YYYY-MM-DD, or "" for
///        birth-generator's default (2023-07-01). Never the clock.
/// @param seed 1 to 16 hexadecimal digits, or "" for a random seed (the
///        persona's signature has the seed that replays it).
EMSCRIPTEN_KEEPALIVE const char* dasmig_persona_generate(const char* country, const char* sex, int age_min,
                                                         int age_max, int traits, const char* reference_date,
                                                         const char* seed)
{
    try
    {
        load_once();
        const auto request = make_request(country, sex, age_min, age_max, traits, reference_date);

        auto& generator = dasmig::psg::instance();
        return reply(*seed == '\0' ? generator.get_persona(request).to_json()
                                   : generator.get_persona(request, parse_demo_seed(seed)).to_json());
    }
    catch (const std::exception& error) { return reply(demo_error(error.what())); }
}

/// @brief Generate personas `first` to `first + count - 1` of a batch, as JSON
///        Lines (one persona per line, each line ending in a line feed).
///
/// Persona `i` is exactly `dasmig_persona_generate` with the seed
/// `core::item_seed(batch_seed, i)`, so a caller can split a batch into ranges
/// and join the results (the npm package does this across Web Workers).
/// Arguments as for dasmig_persona_generate, except:
/// @param seed The batch seed, 1 to 16 hexadecimal digits (required).
/// @param first Index of the first persona (a whole number below 2^53).
/// @param count Number of personas (0 or more).
EMSCRIPTEN_KEEPALIVE const char* dasmig_persona_generate_batch(const char* country, const char* sex, int age_min,
                                                               int age_max, int traits,
                                                               const char* reference_date, const char* seed,
                                                               double first, int count)
{
    try
    {
        load_once();
        const auto request = make_request(country, sex, age_min, age_max, traits, reference_date);
        if (*seed == '\0') throw std::invalid_argument("a batch needs a seed");
        if (count < 0) throw std::invalid_argument("count must not be negative");
        if (!(first >= 0.0 && first <= 9007199254740991.0) || first != std::floor(first))
            throw std::invalid_argument("first must be a whole number from 0 to 2^53 - 1");

        std::string lines;
        const dasmig::core::batch_options options{.count = static_cast<std::uint64_t>(count),
                                                  .first = static_cast<std::uint64_t>(first),
                                                  .threads = 1};
        dasmig::psg::instance().for_each_persona(request, options, parse_demo_seed(seed),
                                                 [&lines](std::uint64_t, const dasmig::persona& p) {
                                                     lines += p.to_json();
                                                     lines += '\n';
                                                 });
        return reply(std::move(lines));
    }
    catch (const std::exception& error) { return reply(demo_error(error.what())); }
}

/// @brief What this module can generate: the supported countries, the trait
///        catalogue's size (the most traits a request can ask for) and the
///        hash of the eight libraries' data.
EMSCRIPTEN_KEEPALIVE const char* dasmig_persona_info()
{
    try
    {
        load_once();
        dasmig::core::json_writer w;
        w.begin_object().key("countries").begin_array();
        for (const auto code : dasmig::psg::supported_countries()) w.value(code);
        w.end_array()
            .field("max_traits", dasmig::trtg::instance().trait_count())
            .field("data", hex(dasmig::psg::instance().data_hash()))
            .end_object();
        return reply(w.str());
    }
    catch (const std::exception& error) { return reply(demo_error(error.what())); }
}

} // extern "C"
