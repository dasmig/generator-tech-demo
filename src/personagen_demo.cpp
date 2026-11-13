// Persona Generator — persona-generator runs the pipeline (eight libraries
// composed through entity-generator fields, in dependency order, and a home
// from property-generator and relationship-generator); this module only turns
// the page's inputs into a persona_request and returns its JSON.

#include "cpp23_polyfill.hpp"
#include <dasmig/personagen.hpp>
#include <emscripten/bind.h>
#include "today.hpp"
#include "demo_support.hpp"

#include <exception>
#include <string>

namespace
{

bool init()
{
    // The libraries' singletons load their compiled-in lite tiers
    // (DASMIG_*_EMBED); only city's admin1 names are a preloaded file.
    auto& cg = dasmig::cg::instance();
    if (!cg.has_admin1()) cg.load_admin1("/resources/city/admin1CodesASCII.txt");
    return cg.has_admin1() && dasmig::psg::instance().has_data();
}

dasmig::persona_request make_request(const std::string& country, const std::string& sex, bool home, bool employer,
                                     std::chrono::year_month_day reference)
{
    dasmig::persona_request request;
    if (!country.empty()) request.country = country;
    if (sex == "female") request.person_sex = dasmig::sex::female;
    else if (sex == "male") request.person_sex = dasmig::sex::male;
    else if (!sex.empty()) throw std::invalid_argument("Sex must be empty, male or female.");
    request.reference_date = reference;
    // A home (property-generator) for a household drawn by relationship-generator,
    // costed by what everyone in it earns; it changes nothing else.
    if (home) request.home = dasmig::property_request{};
    // The company the persona works for (company-generator), drawn by its staff in an
    // industry that employs the persona's job; null for a persona who is not employed.
    if (employer) request.employer = dasmig::company_request{};
    return request;
}

std::string generate_persona(const std::string& country, const std::string& sex, bool home, bool employer)
{
    try
    {
        return dasmig::psg::instance().get_persona(make_request(country, sex, home, employer, today())).to_json();
    }
    catch (const std::exception& error) { return demo_error(error.what()); }
}

std::string generate_seeded(const std::string& country, const std::string& sex, bool home, bool employer,
                            const std::string& reference, const std::string& seed)
{
    try
    {
        return dasmig::psg::instance()
            .get_persona(make_request(country, sex, home, employer, parse_reference_date(reference)), parse_demo_seed(seed))
            .to_json();
    }
    catch (const std::exception& error) { return demo_error(error.what()); }
}

std::string supported_countries()
{
    dasmig::core::json_writer writer;
    writer.begin_array();
    for (const auto code : dasmig::psg::supported_countries()) writer.value(code);
    writer.end_array();
    return writer.str();
}

} // namespace

EMSCRIPTEN_BINDINGS(personagen_demo)
{
    emscripten::function("init", &init);
    emscripten::function("generatePersona", &generate_persona);
    emscripten::function("generatePersonaSeeded", &generate_seeded);
    emscripten::function("supportedCountries", &supported_countries);
}
