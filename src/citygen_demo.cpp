#include "cpp23_polyfill.hpp"
#include <dasmig/citygen.hpp>
#include <emscripten/bind.h>
#include <string>

namespace
{

bool init()
{
    auto& gen = dasmig::cg::instance();
    if (!gen.has_data())
    {
        gen.load("/resources/lite/cities.tsv");
    }
    return gen.has_data();
}

std::string generate_city(const std::string& country)
{
    auto& gen = dasmig::cg::instance();
    auto c = country.empty() ? gen.get_city() : gen.get_city(country);

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
    json += ",\"ASCII Name\":" + q(c.asciiname);
    json += ",\"Country\":" + q(c.country_code);
    json += ",\"Population\":" + std::to_string(c.population);
    json += ",\"Latitude\":" + std::to_string(c.latitude);
    json += ",\"Longitude\":" + std::to_string(c.longitude);
    json += ",\"Timezone\":" + q(c.timezone);
    json += ",\"Elevation\":" + std::to_string(c.elevation);
    json += ",\"Feature Code\":" + q(c.feature_code);
    json += "}";
    return json;
}

} // namespace

EMSCRIPTEN_BINDINGS(citygen_demo)
{
    emscripten::function("init", &init);
    emscripten::function("generateCity", &generate_city);
}
