// User Generator — user-generator composes seven libraries, and device-generator
// for the device of the last sign-in; this module turns the page's inputs into a
// user_request and returns the result as JSON.
//
// Seven of the libraries have their lite data compiled in (DASMIG_*_EMBED).
// address-generator's street data is a pack the page writes into /pack: the
// shared tables at start, and a country's street file before that country is
// asked for (see addressgen_demo.cpp), so the page names the country first.

#include "cpp23_polyfill.hpp"
#include <dasmig/usergen.hpp>
#include <emscripten/bind.h>
#include "today.hpp"
#include "demo_support.hpp"

#include <cstdint>
#include <format>
#include <stdexcept>
#include <string>

namespace
{

constexpr const char* pack_dir = "/pack";

dasmig::adg& addresses()
{
    static dasmig::adg instance;
    return instance;
}

dasmig::usg& generator()
{
    static dasmig::usg instance{dasmig::usg::sources{.address = &addresses()}};
    return instance;
}

bool init()
{
    addresses().load(pack_dir);
    return addresses().has_data() && generator().load(dasmig::dataset::lite);
}

// country: alpha-2 code (the page draws one when the user chooses "Any");
// sex: "male", "female" or empty (drawn); status: a status key or empty (drawn);
// device: also the device of the last sign-in.
dasmig::user_request make_request(const std::string& country, const std::string& sex, bool safe,
                                  const std::string& status, bool device)
{
    if (country.empty()) throw std::invalid_argument("Choose a country.");
    dasmig::user_request request;
    request.country = country;
    if (sex == "male") request.person_sex = dasmig::sex::male;
    else if (sex == "female") request.person_sex = dasmig::sex::female;
    else if (!sex.empty()) throw std::invalid_argument("Sex must be male, female or empty.");
    request.reference_date = today();
    request.safe = safe;
    if (!status.empty()) request.status = status;
    request.device = device;
    return request;
}

// The result as the demo's table shows it.
std::string describe(const dasmig::user& user)
{
    const auto& account = user.account;
    dasmig::core::json_writer writer;
    writer.begin_object()
        .field("Name", user.name)
        .field("Sex", dasmig::core::sex_name(user.person_sex()))
        .field("Age", static_cast<std::uint64_t>(user.age()))
        .field("Country", user.country_name)
        .field("Username", account.username)
        .field("Password", account.password)
        .field("Email", user.mail.address())
        .field("Phone", user.mobile.international())
        .field("Address", user.location.formatted(", "))
        .field("Status", account.status_label)
        .field("Created", account.created)
        .field("Last login", account.last_login.empty() ? "never" : account.last_login)
        .field("Platform", account.platform_label)
        .field("Locale", account.locale)
        .field("Time zone", account.timezone)
        .field("Currency", account.currency.empty() ? "-" : account.currency);
    if (user.client)
    {
        const auto& d = *user.client;
        writer.field("Device", d.to_string())
            .field("User-Agent", d.user_agent.empty() ? "-" : d.user_agent)
            .field("Accept-Language", d.accept_language.empty() ? "-" : d.accept_language);
    }
    writer.field("Seed (hex)", std::format("{:016x}", user.signature().seed)).end_object();
    return writer.str();
}

std::string generate_user(const std::string& country, const std::string& sex, bool safe, const std::string& status,
                          bool device)
{
    try
    {
        return describe(generator().get_user(make_request(country, sex, safe, status, device)));
    }
    catch (const std::invalid_argument& error) { return demo_error(error.what()); }
    catch (const std::runtime_error& error) { return demo_error(error.what()); }
}

std::string generate_user_seeded(const std::string& country, const std::string& sex, bool safe,
                                 const std::string& status, bool device, const std::string& seed)
{
    try
    {
        return generator().get_user(make_request(country, sex, safe, status, device), parse_demo_seed(seed)).to_json();
    }
    catch (const std::invalid_argument& error) { return demo_error(error.what()); }
    catch (const std::runtime_error& error) { return demo_error(error.what()); }
}

// The same call seed with and without the safe option.
std::string compare_safe(const std::string& country, const std::string& sex, const std::string& status, bool device)
{
    try
    {
        auto& gen = generator();
        const auto anchor = gen.get_user(make_request(country, sex, false, status, device));
        dasmig::core::json_writer writer;
        writer.begin_array();
        for (const bool safe : {false, true})
        {
            writer.begin_object().field("safe", safe).key("user");
            gen.get_user(make_request(country, sex, safe, status, device), anchor.seed()).to_json(writer);
            writer.end_object();
        }
        writer.end_array();
        return writer.str();
    }
    catch (const std::invalid_argument& error) { return demo_error(error.what()); }
    catch (const std::runtime_error& error) { return demo_error(error.what()); }
}

// The country codes every part has data for, as a JSON array.
std::string supported_countries()
{
    dasmig::core::json_writer writer;
    writer.begin_array();
    for (const auto& code : generator().supported_countries()) writer.value(code);
    writer.end_array();
    return writer.str();
}

// Whether the country has a street file to fetch before generating.
bool has_street_file(const std::string& country) { return addresses().has_streets(country); }

} // namespace

EMSCRIPTEN_BINDINGS(usergen_demo)
{
    emscripten::function("init", &init);
    emscripten::function("supportedCountries", &supported_countries);
    emscripten::function("hasStreetFile", &has_street_file);
    emscripten::function("generateUser", &generate_user);
    emscripten::function("generateUserSeeded", &generate_user_seeded);
    emscripten::function("compareSafe", &compare_safe);
}
