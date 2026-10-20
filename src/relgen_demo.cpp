#include <dasmig/relgen.hpp>
#include <emscripten/bind.h>
#include <algorithm>
#include <string>
#include "demo_support.hpp"

namespace
{

bool init()
{
    // The singletons load the compiled-in lite tiers (DASMIG_*_EMBED).
    return dasmig::relg::instance().has_family_data() && dasmig::bthg::instance().has_data()
           && dasmig::ng::instance().has_resources();
}

std::string lower(std::string text)
{
    std::ranges::transform(text, text.begin(),
                           [](char c) { return static_cast<char>(c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c); });
    return text;
}

// The countries with family data whose names the module has, so every family is named.
std::string supported_countries()
{
    dasmig::core::json_writer writer;
    writer.begin_array();
    for (const auto& code : dasmig::relg::instance().countries())
    {
        if (dasmig::ng::to_culture(dasmig::core::from_utf8(lower(code))) != dasmig::culture::any)
            writer.value(code);
    }
    writer.end_array();
    return writer.str();
}

// An empty country, sex or seed, or a negative age, is left to the library to draw.
std::string generate_family(const std::string& country, const std::string& sex, int age, int up, int down,
                            const std::string& seed)
{
    try
    {
        if (up < 0 || up > 2 || down < 0 || down > 2)
            return demo_error("Choose 0 to 2 generations above and below.");
        if (age > 100) return demo_error("Choose an age between 0 and 100.");
        dasmig::family_request request;
        if (!country.empty()) request.anchor.country_code = country;
        if (sex == "male" || sex == "female")
            request.anchor.bio_sex = sex == "female" ? dasmig::sex::female : dasmig::sex::male;
        else if (!sex.empty()) return demo_error("Sex must be male or female.");
        if (age >= 0) request.anchor.age = age;
        request.up = up;
        request.down = down;
        auto& gen = dasmig::relg::instance();
        return (seed.empty() ? gen.get_family(request) : gen.get_family(request, parse_demo_seed(seed))).to_json();
    }
    catch (const std::invalid_argument& error) { return demo_error(error.what()); }
    catch (const std::runtime_error& error) { return demo_error(error.what()); }
}

} // namespace

EMSCRIPTEN_BINDINGS(relgen_demo)
{
    emscripten::function("init", &init);
    emscripten::function("supportedCountries", &supported_countries);
    emscripten::function("generateFamily", &generate_family);
}
