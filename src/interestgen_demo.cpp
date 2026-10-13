#include <dasmig/interestgen.hpp>
#include <emscripten/bind.h>
#include <cstdint>
#include <string>
#include <vector>

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

} // namespace

EMSCRIPTEN_BINDINGS(interestgen_demo)
{
    emscripten::function("init", &init);
    emscripten::function("generateInterest", &generate_interest);
}
