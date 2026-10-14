#include <dasmig/biodatagen.hpp>
#include <emscripten/bind.h>
#include <string>
#include "demo_support.hpp"

namespace
{

bool init()
{
    // The singleton loads the compiled-in lite tier (DASMIG_*_EMBED).
    return dasmig::bdg::instance().has_data();
}

std::string generate_biodata(const std::string& country, const std::string& sex_str)
{
    auto& gen = dasmig::bdg::instance();

    dasmig::biodata_request request{.country = country};
    if (!sex_str.empty())
        request.bio_sex = sex_str == "female" ? dasmig::sex::female : dasmig::sex::male;
    const auto b = gen.get_biodata(request);

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

    // Format doubles to 1 decimal
    auto fmt1 = [](double v) {
        int x = static_cast<int>(v * 10);
        return std::to_string(x / 10) + "." + std::to_string(x % 10);
    };

    std::string json = "{";
    json += "\"Country\":" + q(b.country_code);
    json += ",\"Sex\":" + q(std::string(dasmig::biodata::sex_str(b.bio_sex)));
    json += ",\"Height\":" + q(fmt1(b.height_cm) + " cm");
    json += ",\"Weight\":" + q(fmt1(b.weight_kg) + " kg");
    json += ",\"BMI\":" + q(fmt1(b.bmi));
    json += ",\"Eye Color\":" + q(std::string(dasmig::biodata::eye_color_str(b.eyes)));
    json += ",\"Hair Color\":" + q(std::string(dasmig::biodata::hair_color_str(b.hair)));
    json += ",\"Skin Type\":" + q(std::string(dasmig::biodata::skin_type_str(b.skin)));
    json += ",\"Blood Type\":" + q(std::string(dasmig::biodata::blood_type_str(b.blood)));
    json += ",\"Handedness\":" + q(std::string(dasmig::biodata::handedness_str(b.hand)));
    json += "}";
    return json;
}

std::string compare_biodata(const std::string& country)
{
    try
    {
        auto& gen = dasmig::bdg::instance();
        const auto anchor = gen.get_biodata({.country = country});
        dasmig::core::json_writer writer;
        writer.begin_array();
        gen.get_biodata({.country = anchor.country_code, .bio_sex = dasmig::sex::male},
                       anchor.seed()).to_json(writer);
        gen.get_biodata({.country = anchor.country_code, .bio_sex = dasmig::sex::female},
                       anchor.seed()).to_json(writer);
        writer.end_array();
        return writer.str();
    }
    catch (const std::invalid_argument& error) { return demo_error(error.what()); }
    catch (const std::runtime_error& error) { return demo_error(error.what()); }
}

std::string generate_seeded(const std::string& country, const std::string& sex, int age,
                            const std::string& seed)
{
    try
    {
        dasmig::biodata_request request{.country = country};
        if (!sex.empty()) request.bio_sex = sex == "female" ? dasmig::sex::female : dasmig::sex::male;
        if (age >= 0) request.age = static_cast<unsigned>(age);
        return dasmig::bdg::instance().get_biodata(request, parse_demo_seed(seed)).to_json();
    }
    catch (const std::invalid_argument& error) { return demo_error(error.what()); }
    catch (const std::runtime_error& error) { return demo_error(error.what()); }
}

} // namespace

EMSCRIPTEN_BINDINGS(biodatagen_demo)
{
    emscripten::function("init", &init);
    emscripten::function("generateBiodata", &generate_biodata);
    emscripten::function("compareBiodata", &compare_biodata);
    emscripten::function("generateBiodataSeeded", &generate_seeded);
}
