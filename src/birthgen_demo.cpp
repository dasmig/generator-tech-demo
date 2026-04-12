#include <dasmig/birthgen.hpp>
#include <emscripten/bind.h>
#include <string>

namespace
{

const char* weekday_name(std::uint8_t wd)
{
    static const char* names[] = {
        "Sunday", "Monday", "Tuesday", "Wednesday",
        "Thursday", "Friday", "Saturday"
    };
    return (wd < 7) ? names[wd] : "Unknown";
}

bool init()
{
    auto& gen = dasmig::bthg::instance();
    if (!gen.has_data())
    {
        gen.load("/resources/lite");
    }
    return gen.has_data();
}

std::string generate_birth(const std::string& country, const std::string& sex_str)
{
    auto& gen = dasmig::bthg::instance();

    dasmig::birth b;
    bool has_country = !country.empty();
    bool has_sex     = !sex_str.empty();
    auto bio_sex     = (sex_str == "female") ? dasmig::sex::female : dasmig::sex::male;

    if (has_country && has_sex)
        b = gen.get_birth(country, bio_sex);
    else if (has_country)
        b = gen.get_birth(country);
    else if (has_sex)
        b = gen.get_birth(bio_sex);
    else
        b = gen.get_birth();

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

    std::string date = std::to_string(b.year) + "-"
        + (b.month < 10 ? "0" : "") + std::to_string(b.month) + "-"
        + (b.day < 10 ? "0" : "") + std::to_string(b.day);

    std::string json = "{";
    json += "\"Country\":" + q(b.country_code);
    json += ",\"Date\":" + q(date);
    json += ",\"Weekday\":" + q(weekday_name(b.weekday));
    json += ",\"Age\":" + std::to_string(b.age);
    json += ",\"Sex\":" + q(b.bio_sex == dasmig::sex::female ? "Female" : "Male");
    json += ",\"Cohort\":" + q(b.cohort);

    // Format le_remaining to 1 decimal
    int le_int    = static_cast<int>(b.le_remaining * 10);
    std::string le = std::to_string(le_int / 10) + "." + std::to_string(le_int % 10);
    json += ",\"Life Expectancy Remaining\":" + q(le + " years");
    json += "}";
    return json;
}

} // namespace

EMSCRIPTEN_BINDINGS(birthgen_demo)
{
    emscripten::function("init", &init);
    emscripten::function("generateBirth", &generate_birth);
}
