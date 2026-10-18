#include <dasmig/phonegen.hpp>
#include <emscripten/bind.h>
#include <cstdint>
#include <format>
#include <stdexcept>
#include <string>
#include "demo_support.hpp"

namespace
{

bool init()
{
    // The singleton loads the compiled-in lite tier (DASMIG_*_EMBED).
    return dasmig::phg::instance().has_data();
}

// country: alpha-2 code or empty (random); type: "mobile", "fixed" or empty (drawn).
dasmig::phone_request make_request(const std::string& country, const std::string& type, bool safe)
{
    dasmig::phone_request request;
    if (!country.empty()) request.country = country;
    if (type == "mobile") request.type = dasmig::phone_type::mobile;
    else if (type == "fixed") request.type = dasmig::phone_type::fixed;
    else if (!type.empty()) throw std::invalid_argument("Type must be mobile, fixed or empty.");
    request.safe = safe;
    return request;
}

// The result as the demo's table shows it.
std::string describe(const dasmig::phone& phone)
{
    dasmig::core::json_writer writer;
    writer.begin_object()
        .field("Country", phone.country_code)
        .field("Type", phone.type == dasmig::phone_type::mobile ? "mobile" : "fixed line")
        .field("International", phone.international())
        .field("National", phone.national())
        .field("E.164", phone.e164())
        .field("RFC 3966", phone.rfc3966())
        .field("Reserved for fiction", phone.reserved)
        .field("Seed (hex)", std::format("{:016x}", phone.signature().seed))
        .end_object();
    return writer.str();
}

std::string generate_phone(const std::string& country, const std::string& type, bool safe)
{
    try
    {
        return describe(dasmig::phg::instance().get_phone(make_request(country, type, safe)));
    }
    catch (const std::invalid_argument& error) { return demo_error(error.what()); }
    catch (const std::runtime_error& error) { return demo_error(error.what()); }
}

std::string generate_phone_seeded(const std::string& country, const std::string& type, bool safe,
                                  const std::string& seed)
{
    try
    {
        return dasmig::phg::instance().get_phone(make_request(country, type, safe), parse_demo_seed(seed)).to_json();
    }
    catch (const std::invalid_argument& error) { return demo_error(error.what()); }
    catch (const std::runtime_error& error) { return demo_error(error.what()); }
}

// The same call seed with and without the safe option.
std::string compare_safe(const std::string& country, const std::string& type)
{
    try
    {
        auto& gen = dasmig::phg::instance();
        auto request = make_request(country, type, false);
        const auto anchor = gen.get_phone(request);
        request.country = anchor.country_code;
        dasmig::core::json_writer writer;
        writer.begin_array();
        for (const bool safe : {false, true})
        {
            request.safe = safe;
            writer.begin_object().field("safe", safe).key("phone");
            gen.get_phone(request, anchor.seed()).to_json(writer);
            writer.end_object();
        }
        writer.end_array();
        return writer.str();
    }
    catch (const std::invalid_argument& error) { return demo_error(error.what()); }
    catch (const std::runtime_error& error) { return demo_error(error.what()); }
}

// The country codes the library can generate for, as a JSON array.
std::string supported_countries()
{
    dasmig::core::json_writer writer;
    writer.begin_array();
    for (const auto& code : dasmig::phg::instance().countries()) writer.value(code);
    writer.end_array();
    return writer.str();
}

} // namespace

EMSCRIPTEN_BINDINGS(phonegen_demo)
{
    emscripten::function("init", &init);
    emscripten::function("supportedCountries", &supported_countries);
    emscripten::function("generatePhone", &generate_phone);
    emscripten::function("generatePhoneSeeded", &generate_phone_seeded);
    emscripten::function("compareSafe", &compare_safe);
}
