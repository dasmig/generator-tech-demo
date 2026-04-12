#include "cpp23_polyfill.hpp"
#include <dasmig/countrygen.hpp>
#include <emscripten/bind.h>
#include <string>

namespace
{

bool init()
{
    auto& gen = dasmig::cntg::instance();
    if (!gen.has_data())
    {
        gen.load("/resources/lite/countries.tsv");
    }
    return gen.has_data();
}

std::string generate_country(const std::string& region)
{
    auto& gen = dasmig::cntg::instance();
    auto c = region.empty() ? gen.get_country() : gen.get_country(region);

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

} // namespace

EMSCRIPTEN_BINDINGS(countrygen_demo)
{
    emscripten::function("init", &init);
    emscripten::function("generateCountry", &generate_country);
}
