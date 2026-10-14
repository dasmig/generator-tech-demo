#include <dasmig/occupationgen.hpp>
#include <emscripten/bind.h>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>
#include "demo_support.hpp"

namespace
{

bool init()
{
    // The singleton loads the compiled-in lite tier (DASMIG_*_EMBED).
    return dasmig::ocg::instance().has_data();
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

std::string join(const std::vector<std::string>& items)
{
    std::string out;
    for (const auto& i : items)
    {
        if (!out.empty()) out += ", ";
        out += i;
    }
    return out.empty() ? "-" : out;
}

// "" is unknown (drawn); otherwise one of the education_level names.
std::optional<dasmig::education_level> parse_education(const std::string& name)
{
    for (const auto level : {dasmig::education_level::less_than_basic, dasmig::education_level::basic,
                             dasmig::education_level::intermediate, dasmig::education_level::advanced})
    {
        if (dasmig::to_string(level) == name) return level;
    }
    return std::nullopt;
}

// country: alpha-2 code or empty (random); sex: "male", "female" or empty;
// age: years; education: a level name or empty (drawn).
std::string generate_occupation(const std::string& country, const std::string& sex_str, int age,
                                const std::string& education)
{
    try
    {
        auto& gen = dasmig::ocg::instance();

        dasmig::occupation_request req;
        req.country = country;
        req.age = static_cast<std::uint8_t>(age < 0 ? 0 : (age > 120 ? 120 : age));
        if (sex_str == "male") req.person_sex = dasmig::sex::male;
        if (sex_str == "female") req.person_sex = dasmig::sex::female;
        req.education = parse_education(education);

        const auto v = gen.get_occupation(req);

        auto q = [](const std::string& s) { return "\"" + escape(s) + "\""; };
        std::string json = "{";
        json += "\"Country\":" + q(v.country_code);
        json += ",\"Sex\":" + q(v.person_sex == dasmig::sex::male ? "male" : "female");
        json += ",\"Age\":" + q(std::to_string(v.age));
        json += ",\"Status\":" + q(std::string{dasmig::to_string(v.status)});
        json += ",\"Education\":" + q(v.education ? std::string{dasmig::to_string(*v.education)} : "-");
        json += ",\"Job\":" + q(v.job ? v.job->label : "-");
        json += ",\"ISCO-08 group\":" + q(v.job ? v.job->isco + " " + v.job->isco_label : "-");
        json += ",\"Skills\":" + q(v.job ? join(v.job->skills) : "-");
        json += ",\"Gear\":" + q(v.job ? join(v.job->gear) : "-");
        json += "}";
        return json;
    }
    catch (const std::invalid_argument& error) { return demo_error(error.what()); }
    catch (const std::runtime_error& error) { return demo_error(error.what()); }
}

// The four education levels for the same country, sex and call seed, all
// employed at age 35, so only the education input varies.
std::string compare_education(const std::string& country, const std::string& sex_str)
{
    try
    {
        auto& gen = dasmig::ocg::instance();
        dasmig::occupation_request request;
        request.country = country;
        request.age = 35;
        request.status = dasmig::employment_status::employed;
        if (sex_str == "male") request.person_sex = dasmig::sex::male;
        if (sex_str == "female") request.person_sex = dasmig::sex::female;
        const auto anchor = gen.get_occupation(request);
        request.country = anchor.country_code;
        request.person_sex = anchor.person_sex;
        dasmig::core::json_writer writer;
        writer.begin_array();
        for (const auto level : {dasmig::education_level::less_than_basic, dasmig::education_level::basic,
                                 dasmig::education_level::intermediate, dasmig::education_level::advanced})
        {
            request.education = level;
            writer.begin_object().field("education", dasmig::to_string(level)).key("occupation");
            gen.get_occupation(request, anchor.seed()).to_json(writer);
            writer.end_object();
        }
        writer.end_array();
        return writer.str();
    }
    catch (const std::invalid_argument& error) { return demo_error(error.what()); }
    catch (const std::runtime_error& error) { return demo_error(error.what()); }
}

std::string generate_seeded(const std::string& country, const std::string& sex, int age,
                            const std::string& education, const std::string& status,
                            const std::string& seed)
{
    try
    {
        if (age < 0 || age > 120) return demo_error("Choose an age between 0 and 120.");
        dasmig::occupation_request request{.country = country, .age = static_cast<std::uint8_t>(age)};
        if (!sex.empty()) request.person_sex = sex == "female" ? dasmig::sex::female : dasmig::sex::male;
        request.education = parse_education(education);
        if (!education.empty() && !request.education) return demo_error("Choose a valid education level.");
        if (!status.empty())
        {
            for (const auto value : {dasmig::employment_status::employed, dasmig::employment_status::unemployed,
                                    dasmig::employment_status::student, dasmig::employment_status::retired,
                                    dasmig::employment_status::inactive})
                if (dasmig::to_string(value) == status) request.status = value;
            if (!request.status) return demo_error("Choose a valid employment status.");
        }
        return dasmig::ocg::instance().get_occupation(request, parse_demo_seed(seed)).to_json();
    }
    catch (const std::invalid_argument& error) { return demo_error(error.what()); }
    catch (const std::runtime_error& error) { return demo_error(error.what()); }
}

} // namespace

EMSCRIPTEN_BINDINGS(occupationgen_demo)
{
    emscripten::function("init", &init);
    emscripten::function("generateOccupation", &generate_occupation);
    emscripten::function("compareEducation", &compare_education);
    emscripten::function("generateOccupationSeeded", &generate_seeded);
}
