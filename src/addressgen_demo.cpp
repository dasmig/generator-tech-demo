#include <dasmig/addressgen.hpp>
#include <emscripten/bind.h>
#include <cstdint>
#include <format>
#include <stdexcept>
#include <string>
#include "demo_support.hpp"

// The data is not compiled in. The page writes the data pack into /pack (the
// core files at start, a country's street file the first time it is asked
// for) and init() loads it; a country's street file is read on the first call
// that needs it.

namespace
{

constexpr const char* pack_dir = "/pack";

dasmig::adg& generator()
{
    static dasmig::adg instance;
    return instance;
}

bool init()
{
    generator().load(pack_dir);
    return generator().has_data();
}

// country: alpha-2 code; script: "local" or "latin".
dasmig::address_request make_request(const std::string& country, const std::string& script)
{
    if (country.empty()) throw std::invalid_argument("Choose a country.");
    dasmig::address_request request;
    request.country = country;
    if (script == "latin") request.script = dasmig::address_script::latin;
    else if (script != "local" && !script.empty()) throw std::invalid_argument("Script must be local or latin.");
    return request;
}

// The result as the demo's table shows it.
std::string describe(const dasmig::address& address)
{
    dasmig::core::json_writer writer;
    writer.begin_object()
        .field("Country", address.country_code)
        .field("Address", address.formatted(", "))
        .field("Street", address.street)
        .field("Number", static_cast<std::uint64_t>(address.number))
        .field("Unit", address.unit.empty() ? "-" : address.unit)
        .field("City", address.city)
        .field("Region", address.region.empty() ? "-" : address.region)
        .field("Postcode", address.postcode.empty() ? "-" : address.postcode)
        .field("Real street", address.real_street)
        .field("Seed (hex)", std::format("{:016x}", address.signature().seed))
        .end_object();
    return writer.str();
}

std::string generate_address(const std::string& country, const std::string& script)
{
    try
    {
        return describe(generator().get_address(make_request(country, script)));
    }
    catch (const std::invalid_argument& error) { return demo_error(error.what()); }
    catch (const std::runtime_error& error) { return demo_error(error.what()); }
}

std::string generate_address_seeded(const std::string& country, const std::string& script, const std::string& seed)
{
    try
    {
        return generator().get_address(make_request(country, script), parse_demo_seed(seed)).to_json();
    }
    catch (const std::invalid_argument& error) { return demo_error(error.what()); }
    catch (const std::runtime_error& error) { return demo_error(error.what()); }
}

// The same call seed in the country's own layout and in the Latin one.
std::string compare_scripts(const std::string& country)
{
    try
    {
        auto& gen = generator();
        const auto anchor = gen.get_address(make_request(country, "local"));
        dasmig::core::json_writer writer;
        writer.begin_array();
        for (const char* script : {"local", "latin"})
        {
            writer.begin_object().field("script", script).key("address");
            gen.get_address(make_request(country, script), anchor.seed()).to_json(writer);
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
    for (const auto& code : generator().countries()) writer.value(code);
    writer.end_array();
    return writer.str();
}

// Whether the country has a street file to fetch before generating.
bool has_street_file(const std::string& country) { return generator().has_streets(country); }

bool streets_loaded(const std::string& country) { return generator().streets_loaded(country); }

} // namespace

EMSCRIPTEN_BINDINGS(addressgen_demo)
{
    emscripten::function("init", &init);
    emscripten::function("supportedCountries", &supported_countries);
    emscripten::function("hasStreetFile", &has_street_file);
    emscripten::function("streetsLoaded", &streets_loaded);
    emscripten::function("generateAddress", &generate_address);
    emscripten::function("generateAddressSeeded", &generate_address_seeded);
    emscripten::function("compareScripts", &compare_scripts);
}
