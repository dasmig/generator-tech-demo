#include "cpp23_polyfill.hpp"
#include <dasmig/citygen.hpp>
#include <emscripten/bind.h>
#include <string>
#include "demo_support.hpp"

namespace
{

bool init()
{
    // The singleton loads the compiled-in lite tier (DASMIG_*_EMBED).
    return dasmig::cg::instance().has_data();
}

std::string city_json(const dasmig::city& c)
{
    auto escape = [](const std::string& s) {
        std::string out;
        out.reserve(s.size());
        for (char ch : s)
        {
            switch (ch)
            {
                case '"':  out += "\\\""; break;
                case '\\': out += "\\\\"; break;
                case '\n': out += "\\n";  break;
                default:   out += ch;
            }
        }
        return out;
    };

    auto q = [&](const std::string& v) { return "\"" + escape(v) + "\""; };

    std::string json = "{";
    json += "\"Name\":" + q(c.name);
    json += ",\"GeoNames ID\":" + std::to_string(c.geonameid);
    json += ",\"ASCII Name\":" + q(c.asciiname);
    json += ",\"Country\":" + q(c.country_code);
    json += ",\"Population\":" + std::to_string(c.population);
    json += ",\"Latitude\":" + std::to_string(c.latitude);
    json += ",\"Longitude\":" + std::to_string(c.longitude);
    json += ",\"Timezone\":" + q(c.timezone);
    json += ",\"Elevation\":" + (c.elevation == dasmig::city::unknown_elevation
                                     ? q("N/A")
                                     : q(std::to_string(c.elevation) + " m"));
    json += ",\"Feature Code\":" + q(c.feature_code);
    json += "}";
    return json;
}

std::string generate_city(const std::string& country)
{
    auto& gen = dasmig::cg::instance();
    dasmig::city_request request;
    if (!country.empty()) request.country = country;
    return city_json(gen.get_city(request));
}

std::string sample_city(const std::string& country, bool weighted, std::uint32_t seed)
{
    auto& gen = dasmig::cg::instance();
    try
    {
        dasmig::city_request request;
        if (!country.empty()) request.country = country;
        request.weighted = weighted;
        return city_json(gen.get_city(request, seed));
    }
    catch (const std::invalid_argument& error) { return demo_error(error.what()); }
    catch (const std::runtime_error& error) { return demo_error(error.what()); }
}

std::string sample_seeded(const std::string& country, bool weighted, const std::string& seed)
{
    try
    {
        dasmig::city_request request;
        if (!country.empty()) request.country = country;
        request.weighted = weighted;
        return dasmig::cg::instance().get_city(request, parse_demo_seed(seed)).to_json();
    }
    catch (const std::invalid_argument& error) { return demo_error(error.what()); }
    catch (const std::runtime_error& error) { return demo_error(error.what()); }
}

unsigned catalogue_count() { return static_cast<unsigned>(dasmig::cg::instance().city_count()); }

} // namespace

EMSCRIPTEN_BINDINGS(citygen_demo)
{
    emscripten::function("init", &init);
    emscripten::function("generateCity", &generate_city);
    emscripten::function("sampleCity", &sample_city);
    emscripten::function("sampleCitySeeded", &sample_seeded);
    emscripten::function("catalogueCount", &catalogue_count);
}
