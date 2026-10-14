#include <dasmig/interestgen.hpp>
#include <emscripten/bind.h>
#include <cstdint>
#include <string>
#include <vector>
#include "demo_support.hpp"

namespace
{

bool init()
{
    // The singleton loads the compiled-in lite tier (DASMIG_*_EMBED).
    return dasmig::intg::instance().has_data();
}

std::string escape(const std::string& s)
{
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
}

std::string join(const std::vector<dasmig::interest_item>& items)
{
    std::string out;
    for (const auto& i : items)
    {
        if (!out.empty()) out += ", ";
        out += i.label;
    }
    return out.empty() ? "-" : out;
}

// country: alpha-2 code or empty (random); sex: "male", "female" or empty;
// age: years, or a negative number for unknown.
std::string generate_interest(const std::string& country, const std::string& sex_str, int age)
{
    auto& gen = dasmig::intg::instance();

    dasmig::interest_request req;
    req.country = country;
    if (sex_str == "male") req.person_sex = dasmig::sex::male;
    if (sex_str == "female") req.person_sex = dasmig::sex::female;
    if (age >= 0) req.age = static_cast<std::uint8_t>(age > 120 ? 120 : age);

    const auto v = gen.get_interest(req);

    auto q = [](const std::string& s) { return "\"" + escape(s) + "\""; };
    std::string json = "{";
    json += "\"Country\":" + q(v.country_code);
    json += ",\"Religion\":" + q(v.religion);
    json += ",\"Hobbies\":" + q(join(v.hobbies));
    json += ",\"Films\":" + q(join(v.films));
    json += ",\"Games\":" + q(join(v.games));
    json += ",\"Books\":" + q(join(v.books));
    json += ",\"Music\":" + q(join(v.music));
    json += "}";
    return json;
}

std::string compare_ages(const std::string& country, const std::string& sex_str)
{
    try
    {
        auto& gen = dasmig::intg::instance();
        dasmig::interest_request request;
        request.country = country;
        request.age = 8;
        if (sex_str == "male") request.person_sex = dasmig::sex::male;
        if (sex_str == "female") request.person_sex = dasmig::sex::female;
        const auto anchor = gen.get_interest(request);
        request.country = anchor.country_code;
        dasmig::core::json_writer writer;
        writer.begin_array();
        for (const auto age : {8, 30, 72})
        {
            request.age = static_cast<std::uint8_t>(age);
            writer.begin_object().field("age", age).key("interest");
            gen.get_interest(request, anchor.seed()).to_json(writer);
            writer.end_object();
        }
        writer.end_array();
        return writer.str();
    }
    catch (const std::invalid_argument& error) { return demo_error(error.what()); }
    catch (const std::runtime_error& error) { return demo_error(error.what()); }
}

std::string generate_seeded(const std::string& country, const std::string& sex, int age,
                            int hobbies, int media, const std::string& seed)
{
    try
    {
        if (age > 120 || hobbies < 0 || media < 0)
            return demo_error("Choose an age up to 120 and nonnegative interest counts.");
        dasmig::interest_request request{.country = country};
        if (age >= 0) request.age = static_cast<std::uint8_t>(age);
        if (!sex.empty()) request.person_sex = sex == "female" ? dasmig::sex::female : dasmig::sex::male;
        request.hobbies = static_cast<std::size_t>(hobbies);
        request.media = static_cast<std::size_t>(media);
        return dasmig::intg::instance().get_interest(request, parse_demo_seed(seed)).to_json();
    }
    catch (const std::invalid_argument& error) { return demo_error(error.what()); }
    catch (const std::runtime_error& error) { return demo_error(error.what()); }
}

} // namespace

EMSCRIPTEN_BINDINGS(interestgen_demo)
{
    emscripten::function("init", &init);
    emscripten::function("generateInterest", &generate_interest);
    emscripten::function("compareAges", &compare_ages);
    emscripten::function("generateInterestSeeded", &generate_seeded);
}
