// Persona Generator — composes all 7 dasmig libraries through entity-generator
// adapter components. Each library result is a component; downstream components
// read upstream values via the entity-generator's generation_context.

#include "cpp23_polyfill.hpp"
#include <dasmig/entitygen.hpp>
#include <dasmig/countrygen.hpp>
#include <dasmig/citygen.hpp>
#include <dasmig/birthgen.hpp>
#include <dasmig/biodatagen.hpp>
#include <dasmig/namegen.hpp>
#include <emscripten/bind.h>

#include <algorithm>
#include <string>
#include <vector>

namespace
{

// ── UTF-8 ↔ wstring helpers (needed for name/nickname libraries) ──

std::string to_utf8(const std::wstring& ws)
{
    std::string out;
    out.reserve(ws.size() * 3);
    for (wchar_t wc : ws)
    {
        if (wc <= 0x7F)
            out.push_back(static_cast<char>(wc));
        else if (wc <= 0x7FF)
        {
            out.push_back(static_cast<char>(0xC0 | ((wc >> 6) & 0x1F)));
            out.push_back(static_cast<char>(0x80 | (wc & 0x3F)));
        }
        else if (wc <= 0xFFFF)
        {
            out.push_back(static_cast<char>(0xE0 | ((wc >> 12) & 0x0F)));
            out.push_back(static_cast<char>(0x80 | ((wc >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (wc & 0x3F)));
        }
        else
        {
            out.push_back(static_cast<char>(0xF0 | ((wc >> 18) & 0x07)));
            out.push_back(static_cast<char>(0x80 | ((wc >> 12) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | ((wc >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (wc & 0x3F)));
        }
    }
    return out;
}

std::wstring from_utf8(const std::string& s)
{
    std::wstring out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size();)
    {
        wchar_t wc;
        unsigned char c = s[i];
        if (c <= 0x7F)      { wc = c; i += 1; }
        else if ((c & 0xE0) == 0xC0)
        {
            wc = (c & 0x1F) << 6;
            if (i + 1 < s.size()) wc |= (s[i + 1] & 0x3F);
            i += 2;
        }
        else if ((c & 0xF0) == 0xE0)
        {
            wc = (c & 0x0F) << 12;
            if (i + 1 < s.size()) wc |= (s[i + 1] & 0x3F) << 6;
            if (i + 2 < s.size()) wc |= (s[i + 2] & 0x3F);
            i += 3;
        }
        else
        {
            wc = (c & 0x07) << 18;
            if (i + 1 < s.size()) wc |= (s[i + 1] & 0x3F) << 12;
            if (i + 2 < s.size()) wc |= (s[i + 2] & 0x3F) << 6;
            if (i + 3 < s.size()) wc |= (s[i + 3] & 0x3F);
            i += 4;
        }
        out.push_back(wc);
    }
    return out;
}

std::wstring to_wstr(const std::string& s) { return from_utf8(s); }

// ── Countries supported by all libraries ──

const std::vector<std::string> supported_countries = {
    "AE","AL","AR","AT","AZ","BD","BE","BG","BH","BO",
    "BR","CA","CH","CL","CM","CN","CO","CR","CY","CZ",
    "DE","DK","DZ","EC","EE","EG","ES","ET","FI","FR",
    "GB","GE","GH","GR","HN","HR","HU","ID","IE","IL",
    "IN","IQ","IR","IS","IT","JM","JO","JP","KR","KZ",
    "LB","LT","LU","LY","MA","MD","MT","MX","MY","NG",
    "NL","NO","PE","PH","PL","PT","RS","RU","SA","SD",
    "SE","SG","SI","SV","SY","TN","TR","US","YE","ZA"
};

// ── Helper: format double to 1 decimal ──

std::wstring fmt1(double v)
{
    int x = static_cast<int>(v * 10);
    if (x < 0) x = -x;
    return std::to_wstring(x / 10) + L"." + std::to_wstring(x % 10);
}

// Module-level state for country selection from the UI.
std::string g_selected_country;

// ═══════════════════════════════════════════════════════════════════
// Adapter Components — each wraps a library call
// ═══════════════════════════════════════════════════════════════════

// ── 1. Country (root component — picks a supported country) ──

class country_component : public dasmig::component
{
  public:
    [[nodiscard]] std::wstring key() const override { return L"Country"; }

    [[nodiscard]] std::any generate(
        const dasmig::generation_context& /*ctx*/) const override
    {
        auto& gen = dasmig::cntg::instance();

        // If a specific country was requested, look it up directly.
        if (!g_selected_country.empty())
            return gen.find_country(g_selected_country);

        // Otherwise pick a random supported country.
        for (;;)
        {
            auto c = gen.get_country();
            if (std::ranges::find(supported_countries, c.cca2)
                != supported_countries.end())
                return c;
        }
    }

    [[nodiscard]] std::wstring to_string(const std::any& val) const override
    {
        auto c = std::any_cast<dasmig::country>(val);
        return to_wstr(c.name_common);
    }
};

// ── Country field extractors ──

class cca2_component : public dasmig::component
{
  public:
    [[nodiscard]] std::wstring key() const override { return L"CCA2"; }
    [[nodiscard]] std::any generate(
        const dasmig::generation_context& ctx) const override
    {
        return ctx.get<dasmig::country>(L"Country").cca2;
    }
    [[nodiscard]] std::wstring to_string(const std::any& val) const override
    {
        return to_wstr(std::any_cast<std::string>(val));
    }
};

class flag_component : public dasmig::component
{
  public:
    [[nodiscard]] std::wstring key() const override { return L"Flag"; }
    [[nodiscard]] std::any generate(
        const dasmig::generation_context& ctx) const override
    {
        return ctx.get<dasmig::country>(L"Country").flag_emoji;
    }
    [[nodiscard]] std::wstring to_string(const std::any& val) const override
    {
        return to_wstr(std::any_cast<std::string>(val));
    }
};

class capital_component : public dasmig::component
{
  public:
    [[nodiscard]] std::wstring key() const override { return L"Capital"; }
    [[nodiscard]] std::any generate(
        const dasmig::generation_context& ctx) const override
    {
        return ctx.get<dasmig::country>(L"Country").capital;
    }
    [[nodiscard]] std::wstring to_string(const std::any& val) const override
    {
        return to_wstr(std::any_cast<std::string>(val));
    }
};

class region_component : public dasmig::component
{
  public:
    [[nodiscard]] std::wstring key() const override { return L"Region"; }
    [[nodiscard]] std::any generate(
        const dasmig::generation_context& ctx) const override
    {
        auto c = ctx.get<dasmig::country>(L"Country");
        return c.region + " / " + c.subregion;
    }
    [[nodiscard]] std::wstring to_string(const std::any& val) const override
    {
        return to_wstr(std::any_cast<std::string>(val));
    }
};

class languages_component : public dasmig::component
{
  public:
    [[nodiscard]] std::wstring key() const override { return L"Languages"; }
    [[nodiscard]] std::any generate(
        const dasmig::generation_context& ctx) const override
    {
        return ctx.get<dasmig::country>(L"Country").languages;
    }
    [[nodiscard]] std::wstring to_string(const std::any& val) const override
    {
        return to_wstr(std::any_cast<std::string>(val));
    }
};

class currency_component : public dasmig::component
{
  public:
    [[nodiscard]] std::wstring key() const override { return L"Currency"; }
    [[nodiscard]] std::any generate(
        const dasmig::generation_context& ctx) const override
    {
        auto c = ctx.get<dasmig::country>(L"Country");
        return c.currency_name + " (" + c.currency_code + ") " + c.currency_symbol;
    }
    [[nodiscard]] std::wstring to_string(const std::any& val) const override
    {
        return to_wstr(std::any_cast<std::string>(val));
    }
};

// ── 2. City (reads country from context) ──

class city_component : public dasmig::component
{
  public:
    [[nodiscard]] std::wstring key() const override { return L"City"; }
    [[nodiscard]] std::any generate(
        const dasmig::generation_context& ctx) const override
    {
        auto cca2 = ctx.get<dasmig::country>(L"Country").cca2;
        return dasmig::cg::instance().get_city(cca2);
    }
    [[nodiscard]] std::wstring to_string(const std::any& val) const override
    {
        return to_wstr(std::any_cast<dasmig::city>(val).name);
    }
};

class timezone_component : public dasmig::component
{
  public:
    [[nodiscard]] std::wstring key() const override { return L"Timezone"; }
    [[nodiscard]] std::any generate(
        const dasmig::generation_context& ctx) const override
    {
        return ctx.get<dasmig::city>(L"City").timezone;
    }
    [[nodiscard]] std::wstring to_string(const std::any& val) const override
    {
        return to_wstr(std::any_cast<std::string>(val));
    }
};

class city_population_component : public dasmig::component
{
  public:
    [[nodiscard]] std::wstring key() const override { return L"City Population"; }
    [[nodiscard]] std::any generate(
        const dasmig::generation_context& ctx) const override
    {
        return ctx.get<dasmig::city>(L"City").population;
    }
    [[nodiscard]] std::wstring to_string(const std::any& val) const override
    {
        return std::to_wstring(std::any_cast<std::uint64_t>(val));
    }
};

class city_admin1_component : public dasmig::component
{
  public:
    [[nodiscard]] std::wstring key() const override { return L"State/Province"; }
    [[nodiscard]] std::any generate(
        const dasmig::generation_context& ctx) const override
    {
        auto c = ctx.get<dasmig::city>(L"City");
        return dasmig::cg::instance().resolve_admin1(
            c.country_code, c.admin1_code);
    }
    [[nodiscard]] std::wstring to_string(const std::any& val) const override
    {
        return to_wstr(std::any_cast<std::string>(val));
    }
};

class city_coords_component : public dasmig::component
{
  public:
    [[nodiscard]] std::wstring key() const override { return L"Coordinates"; }
    [[nodiscard]] std::any generate(
        const dasmig::generation_context& ctx) const override
    {
        auto c = ctx.get<dasmig::city>(L"City");
        return std::make_pair(c.latitude, c.longitude);
    }
    [[nodiscard]] std::wstring to_string(const std::any& val) const override
    {
        auto [lat, lon] = std::any_cast<std::pair<double, double>>(val);
        return fmt1(lat) + L"°, " + fmt1(lon) + L"°";
    }
};

class city_lat_component : public dasmig::component
{
  public:
    [[nodiscard]] std::wstring key() const override { return L"Latitude"; }
    [[nodiscard]] std::any generate(
        const dasmig::generation_context& ctx) const override
    {
        return ctx.get<dasmig::city>(L"City").latitude;
    }
    [[nodiscard]] std::wstring to_string(const std::any& val) const override
    {
        return fmt1(std::any_cast<double>(val)) + L"°";
    }
};

class city_lon_component : public dasmig::component
{
  public:
    [[nodiscard]] std::wstring key() const override { return L"Longitude"; }
    [[nodiscard]] std::any generate(
        const dasmig::generation_context& ctx) const override
    {
        return ctx.get<dasmig::city>(L"City").longitude;
    }
    [[nodiscard]] std::wstring to_string(const std::any& val) const override
    {
        return fmt1(std::any_cast<double>(val)) + L"°";
    }
};

class city_elevation_component : public dasmig::component
{
  public:
    [[nodiscard]] std::wstring key() const override { return L"Elevation"; }
    [[nodiscard]] std::any generate(
        const dasmig::generation_context& ctx) const override
    {
        return static_cast<int>(ctx.get<dasmig::city>(L"City").elevation);
    }
    [[nodiscard]] std::wstring to_string(const std::any& val) const override
    {
        auto v = std::any_cast<int>(val);
        if (v <= -9999) return L"N/A";
        return std::to_wstring(v) + L" m";
    }
};

// ── 3. Birth (reads country from context) ──

class birth_component : public dasmig::component
{
  public:
    [[nodiscard]] std::wstring key() const override { return L"Birthday"; }
    [[nodiscard]] std::any generate(
        const dasmig::generation_context& ctx) const override
    {
        auto cca2 = ctx.get<dasmig::country>(L"Country").cca2;
        return dasmig::bthg::instance().get_birth(cca2);
    }
    [[nodiscard]] std::wstring to_string(const std::any& val) const override
    {
        auto b = std::any_cast<dasmig::birth>(val);
        // Format as YYYY-MM-DD
        auto pad = [](int v) { return (v < 10 ? L"0" : L"") + std::to_wstring(v); };
        return std::to_wstring(b.year) + L"-" + pad(b.month) + L"-" + pad(b.day);
    }
};

class age_component : public dasmig::component
{
  public:
    [[nodiscard]] std::wstring key() const override { return L"Age"; }
    [[nodiscard]] std::any generate(
        const dasmig::generation_context& ctx) const override
    {
        return static_cast<int>(ctx.get<dasmig::birth>(L"Birthday").age);
    }
    [[nodiscard]] std::wstring to_string(const std::any& val) const override
    {
        return std::to_wstring(std::any_cast<int>(val));
    }
};

class sex_component : public dasmig::component
{
  public:
    [[nodiscard]] std::wstring key() const override { return L"Sex"; }
    [[nodiscard]] std::any generate(
        const dasmig::generation_context& ctx) const override
    {
        return ctx.get<dasmig::birth>(L"Birthday").bio_sex;
    }
    [[nodiscard]] std::wstring to_string(const std::any& val) const override
    {
        auto s = std::any_cast<dasmig::sex>(val);
        return (s == dasmig::sex::female) ? L"Female" : L"Male";
    }
};

// ── 4. Biodata (reads country + sex from context) ──

class biodata_component : public dasmig::component
{
  public:
    [[nodiscard]] std::wstring key() const override { return L"Height"; }
    [[nodiscard]] std::any generate(
        const dasmig::generation_context& ctx) const override
    {
        auto cca2 = ctx.get<dasmig::country>(L"Country").cca2;
        auto bio_sex = ctx.get<dasmig::sex>(L"Sex");
        return dasmig::bdg::instance().get_biodata(cca2, bio_sex);
    }
    [[nodiscard]] std::wstring to_string(const std::any& val) const override
    {
        return fmt1(std::any_cast<dasmig::biodata>(val).height_cm) + L" cm";
    }
};

class weight_component : public dasmig::component
{
  public:
    [[nodiscard]] std::wstring key() const override { return L"Weight"; }
    [[nodiscard]] std::any generate(
        const dasmig::generation_context& ctx) const override
    {
        return ctx.get<dasmig::biodata>(L"Height").weight_kg;
    }
    [[nodiscard]] std::wstring to_string(const std::any& val) const override
    {
        return fmt1(std::any_cast<double>(val)) + L" kg";
    }
};

class bmi_component : public dasmig::component
{
  public:
    [[nodiscard]] std::wstring key() const override { return L"BMI"; }
    [[nodiscard]] std::any generate(
        const dasmig::generation_context& ctx) const override
    {
        return ctx.get<dasmig::biodata>(L"Height").bmi;
    }
    [[nodiscard]] std::wstring to_string(const std::any& val) const override
    {
        return fmt1(std::any_cast<double>(val));
    }
};

class eye_color_component : public dasmig::component
{
  public:
    [[nodiscard]] std::wstring key() const override { return L"Eye Color"; }
    [[nodiscard]] std::any generate(
        const dasmig::generation_context& ctx) const override
    {
        return ctx.get<dasmig::biodata>(L"Height").eyes;
    }
    [[nodiscard]] std::wstring to_string(const std::any& val) const override
    {
        return to_wstr(std::string(
            dasmig::biodata::eye_color_str(std::any_cast<dasmig::eye_color>(val))));
    }
};

class hair_color_component : public dasmig::component
{
  public:
    [[nodiscard]] std::wstring key() const override { return L"Hair Color"; }
    [[nodiscard]] std::any generate(
        const dasmig::generation_context& ctx) const override
    {
        return ctx.get<dasmig::biodata>(L"Height").hair;
    }
    [[nodiscard]] std::wstring to_string(const std::any& val) const override
    {
        return to_wstr(std::string(
            dasmig::biodata::hair_color_str(std::any_cast<dasmig::hair_color>(val))));
    }
};

class skin_type_component : public dasmig::component
{
  public:
    [[nodiscard]] std::wstring key() const override { return L"Skin Type"; }
    [[nodiscard]] std::any generate(
        const dasmig::generation_context& ctx) const override
    {
        return ctx.get<dasmig::biodata>(L"Height").skin;
    }
    [[nodiscard]] std::wstring to_string(const std::any& val) const override
    {
        return to_wstr(std::string(
            dasmig::biodata::skin_type_str(std::any_cast<dasmig::skin_type>(val))));
    }
};

class blood_type_component : public dasmig::component
{
  public:
    [[nodiscard]] std::wstring key() const override { return L"Blood Type"; }
    [[nodiscard]] std::any generate(
        const dasmig::generation_context& ctx) const override
    {
        return ctx.get<dasmig::biodata>(L"Height").blood;
    }
    [[nodiscard]] std::wstring to_string(const std::any& val) const override
    {
        return to_wstr(std::string(
            dasmig::biodata::blood_type_str(std::any_cast<dasmig::blood_type>(val))));
    }
};

// ── 5. Name (reads country → culture, sex → gender from context) ──

class name_component : public dasmig::component
{
  public:
    [[nodiscard]] std::wstring key() const override { return L"Name"; }
    [[nodiscard]] std::any generate(
        const dasmig::generation_context& ctx) const override
    {
        auto cca2 = ctx.get<dasmig::country>(L"Country").cca2;
        auto bio_sex = ctx.get<dasmig::sex>(L"Sex");

        // Map CCA2 to name-generator culture (lowercase)
        std::wstring code;
        for (char c : cca2) code += static_cast<wchar_t>(std::tolower(c));
        auto culture = dasmig::ng::to_culture(code);

        auto gender = (bio_sex == dasmig::sex::female)
            ? dasmig::gender::f : dasmig::gender::m;

        auto n = dasmig::ng::instance().get_name(gender, culture);
        n.append_surname();
        return static_cast<std::wstring>(n);
    }
    [[nodiscard]] std::wstring to_string(const std::any& val) const override
    {
        return std::any_cast<std::wstring>(val);
    }
};

// ═══════════════════════════════════════════════════════════════════
// Init & Generate
// ═══════════════════════════════════════════════════════════════════

bool init()
{
    // Load all data-driven libraries
    bool ok = true;

    auto& cntg = dasmig::cntg::instance();
    if (!cntg.has_data()) cntg.load("/resources/country/lite/countries.tsv");
    ok &= cntg.has_data();

    auto& cg = dasmig::cg::instance();
    if (!cg.has_data()) cg.load("/resources/city/lite/cities.tsv");
    if (!cg.has_admin1()) cg.load_admin1("/resources/city/admin1CodesASCII.txt");
    ok &= cg.has_data();

    auto& bthg = dasmig::bthg::instance();
    if (!bthg.has_data()) bthg.load("/resources/birth/lite");
    ok &= bthg.has_data();

    auto& bdg = dasmig::bdg::instance();
    if (!bdg.has_data()) bdg.load("/resources/biodata/lite");
    ok &= bdg.has_data();

    auto& ng = dasmig::ng::instance();
    if (!ng.has_resources()) ng.load("/resources/name/lite");
    ok &= ng.has_resources();

    // Register adapter components in dependency order
    auto& gen = dasmig::eg::instance();
    gen.clear();

    // Location group
    gen.add(std::make_unique<country_component>());
    gen.add(std::make_unique<cca2_component>());
    gen.add(std::make_unique<flag_component>());
    gen.add(std::make_unique<capital_component>());
    gen.add(std::make_unique<region_component>());
    gen.add(std::make_unique<languages_component>());
    gen.add(std::make_unique<currency_component>());
    gen.add(std::make_unique<city_component>());
    gen.add(std::make_unique<timezone_component>());
    gen.add(std::make_unique<city_population_component>());
    gen.add(std::make_unique<city_admin1_component>());
    gen.add(std::make_unique<city_coords_component>());
    gen.add(std::make_unique<city_lat_component>());
    gen.add(std::make_unique<city_lon_component>());
    gen.add(std::make_unique<city_elevation_component>());

    // Demographics group
    gen.add(std::make_unique<birth_component>());
    gen.add(std::make_unique<age_component>());
    gen.add(std::make_unique<sex_component>());
    // Physical group
    gen.add(std::make_unique<biodata_component>());
    gen.add(std::make_unique<weight_component>());
    gen.add(std::make_unique<bmi_component>());
    gen.add(std::make_unique<eye_color_component>());
    gen.add(std::make_unique<hair_color_component>());
    gen.add(std::make_unique<skin_type_component>());
    gen.add(std::make_unique<blood_type_component>());

    // Identity group (depends on country + sex)
    gen.add(std::make_unique<name_component>());

    return ok;
}

std::string generate_persona(const std::string& country_cca2)
{
    g_selected_country = country_cca2;

    auto& gen = dasmig::eg::instance();
    auto entity = gen.generate();
    auto m = entity.to_map();

    auto escape = [](const std::string& s) {
        std::string out;
        out.reserve(s.size());
        for (char c : s) {
            switch (c) {
                case '"':  out += "\\\""; break;
                case '\\': out += "\\\\"; break;
                case '\n': out += "\\n";  break;
                default:   out += c;
            }
        }
        return out;
    };

    std::string json = "{";
    bool first = true;
    for (auto& [key, val] : m) {
        if (!first) json += ",";
        json += "\"" + escape(to_utf8(key)) + "\":\"" + escape(to_utf8(val)) + "\"";
        first = false;
    }
    json += "}";
    return json;
}

} // namespace

EMSCRIPTEN_BINDINGS(personagen_demo)
{
    emscripten::function("init", &init);
    emscripten::function("generatePersona", &generate_persona);
}
