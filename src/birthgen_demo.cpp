#include <dasmig/birthgen.hpp>
#include "today.hpp"
#include <emscripten/bind.h>
#include <string>
#include "demo_support.hpp"

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
    // The singleton loads the compiled-in lite tier (DASMIG_*_EMBED).
    return dasmig::bthg::instance().has_data();
}

std::string birth_json(const std::string& country, const std::string& sex_str,
                       std::chrono::year_month_day reference)
{
    auto& gen = dasmig::bthg::instance();
    dasmig::birth_request request;
    request.reference_date = reference;
    if (!country.empty()) request.country_code = country;
    if (!sex_str.empty())
        request.bio_sex = sex_str == "female" ? dasmig::sex::female : dasmig::sex::male;
    const auto b = gen.get_birth(request);

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

std::string generate_seeded(const std::string& country, const std::string& sex,
                            const std::string& reference, int year, int min_age, int max_age,
                            const std::string& seed)
{
    try
    {
        dasmig::birth_request request;
        request.reference_date = parse_reference_date(reference);
        if (!country.empty()) request.country_code = country;
        if (!sex.empty()) request.bio_sex = sex == "female" ? dasmig::sex::female : dasmig::sex::male;
        if (year >= 0)
        {
            if (year > 65535) return demo_error("Birth year is out of range.");
            request.year = dasmig::year_t{static_cast<std::uint16_t>(year)};
        }
        if (min_age >= 0 || max_age >= 0)
        {
            if (min_age < 0 || max_age < min_age || max_age > 100)
                return demo_error("Choose an age range between 0 and 100.");
            request.ages = dasmig::age_range{static_cast<std::uint8_t>(min_age),
                                           static_cast<std::uint8_t>(max_age)};
        }
        return dasmig::bthg::instance().get_birth(request, parse_demo_seed(seed)).to_json();
    }
    catch (const std::invalid_argument& error) { return demo_error(error.what()); }
    catch (const std::runtime_error& error) { return demo_error(error.what()); }
}

std::string generate_birth(const std::string& country, const std::string& sex)
{
    return birth_json(country, sex, today());
}

std::string generate_birth_at(const std::string& country, const std::string& sex,
                              const std::string& reference)
{
    try
    {
        auto json = birth_json(country, sex, parse_reference_date(reference));
        json.pop_back();
        return json + ",\"Reference Date\":\"" + reference + "\"}";
    }
    catch (const std::invalid_argument& error) { return demo_error(error.what()); }
    catch (const std::runtime_error& error) { return demo_error(error.what()); }
}

} // namespace

EMSCRIPTEN_BINDINGS(birthgen_demo)
{
    emscripten::function("init", &init);
    emscripten::function("generateBirth", &generate_birth);
    emscripten::function("generateBirthAt", &generate_birth_at);
    emscripten::function("generateBirthSeeded", &generate_seeded);
}
