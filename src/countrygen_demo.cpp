#include "cpp23_polyfill.hpp"
#include <dasmig/countrygen.hpp>
#include <emscripten/bind.h>
#include <string>
#include "demo_support.hpp"

namespace
{

bool init()
{
    // The singleton loads the compiled-in lite tier (DASMIG_*_EMBED).
    return dasmig::cntg::instance().has_data();
}

std::string country_json(const dasmig::country& c)
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
    json += "\"Flag\":" + q(c.flag_emoji);
    json += ",\"Name\":" + q(c.name_common);
    json += ",\"Official Name\":" + q(c.name_official);
    json += ",\"CCA2\":" + q(c.cca2);
    json += ",\"CCA3\":" + q(c.cca3);
    json += ",\"Capital\":" + q(c.capital);
    json += ",\"Region\":" + q(c.region);
    json += ",\"Subregion\":" + q(c.subregion);
    json += ",\"Continent\":" + q(c.continent);
    json += ",\"Population\":" + std::to_string(c.population);
    json += ",\"Area (km²)\":" + std::to_string(c.area);
    json += ",\"Languages\":" + q(c.languages);
    json += ",\"Currency\":" + q(c.currency_name + " (" + c.currency_code + ") " + c.currency_symbol);
    json += ",\"Timezones\":" + q(c.timezones);
    json += ",\"Driving Side\":" + q(c.driving_side);
    json += ",\"TLD\":" + q(c.tld);
    json += ",\"Calling Code\":" + q(c.idd_root + c.idd_suffix);
    json += ",\"UN Member\":" + std::string(c.un_member ? "true" : "false");
    json += ",\"Independent\":" + std::string(c.independent ? "true" : "false");
    json += ",\"Landlocked\":" + std::string(c.landlocked ? "true" : "false");
    json += "}";
    return json;
}

std::string generate_country(const std::string& region)
{
    auto& gen = dasmig::cntg::instance();
    gen.weighted(true);
    return country_json(region.empty() ? gen.get_country() : gen.get_country(region));
}

std::string sample_country(const std::string& region, bool weighted, std::uint32_t seed)
{
    auto& gen = dasmig::cntg::instance();
    gen.weighted(weighted);
    try
    {
        return country_json(region.empty() ? gen.get_country(seed) : gen.get_country(region, seed));
    }
    catch (const std::invalid_argument& error) { return demo_error(error.what()); }
    catch (const std::runtime_error& error) { return demo_error(error.what()); }
}

unsigned catalogue_count() { return static_cast<unsigned>(dasmig::cntg::instance().country_count()); }

} // namespace

EMSCRIPTEN_BINDINGS(countrygen_demo)
{
    emscripten::function("init", &init);
    emscripten::function("generateCountry", &generate_country);
    emscripten::function("sampleCountry", &sample_country);
    emscripten::function("catalogueCount", &catalogue_count);
}
