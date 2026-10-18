#include <dasmig/emailgen.hpp>
#include <emscripten/bind.h>
#include <array>
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
    return dasmig::emg::instance().has_data();
}

// country: alpha-2 code or empty (random); birth_year: 0 or less for unknown.
dasmig::email_request make_request(const std::string& country, const std::string& given,
                                   const std::string& family, const std::string& nickname,
                                   int birth_year, bool safe)
{
    dasmig::email_request request;
    if (!country.empty()) request.country = country;
    request.given = given;
    request.family = family;
    request.nickname = nickname;
    if (birth_year > 0) request.birth_year = birth_year;
    request.safe = safe;
    return request;
}

// The result as the demo's table shows it.
std::string describe(const dasmig::email& email)
{
    dasmig::core::json_writer writer;
    writer.begin_object()
        .field("Country", email.country_code)
        .field("Address", email.address())
        .field("Local part", email.local)
        .field("Domain", email.domain)
        .field("Domain kind", email.kind.empty() ? std::string{"-"} : email.kind)
        .field("Reserved for fiction", email.reserved)
        .field("Seed (hex)", std::format("{:016x}", email.signature().seed))
        .end_object();
    return writer.str();
}

std::string generate_email(const std::string& country, const std::string& given, const std::string& family,
                           const std::string& nickname, int birth_year, bool safe)
{
    try
    {
        return describe(dasmig::emg::instance().get_email(make_request(country, given, family, nickname, birth_year, safe)));
    }
    catch (const std::invalid_argument& error) { return demo_error(error.what()); }
    catch (const std::runtime_error& error) { return demo_error(error.what()); }
}

std::string generate_email_seeded(const std::string& country, const std::string& given, const std::string& family,
                                  const std::string& nickname, int birth_year, bool safe, const std::string& seed)
{
    try
    {
        return dasmig::emg::instance().get_email(
            make_request(country, given, family, nickname, birth_year, safe), parse_demo_seed(seed)).to_json();
    }
    catch (const std::invalid_argument& error) { return demo_error(error.what()); }
    catch (const std::runtime_error& error) { return demo_error(error.what()); }
}

// The same name and call seed in several countries: only the country varies.
std::string compare_countries(const std::string& given, const std::string& family, const std::string& nickname,
                              int birth_year, bool safe)
{
    try
    {
        auto& gen = dasmig::emg::instance();
        const auto anchor = gen.get_email({});
        dasmig::core::json_writer writer;
        writer.begin_array();
        for (const char* country : std::array{"US", "DE", "BR", "JP", "CN", "RU", "FR", "EG"})
        {
            writer.begin_object().field("country", country).key("email");
            gen.get_email(make_request(country, given, family, nickname, birth_year, safe), anchor.seed())
                .to_json(writer);
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
    for (const auto& code : dasmig::emg::instance().countries()) writer.value(code);
    writer.end_array();
    return writer.str();
}

} // namespace

EMSCRIPTEN_BINDINGS(emailgen_demo)
{
    emscripten::function("init", &init);
    emscripten::function("supportedCountries", &supported_countries);
    emscripten::function("generateEmail", &generate_email);
    emscripten::function("generateEmailSeeded", &generate_email_seeded);
    emscripten::function("compareCountries", &compare_countries);
}
