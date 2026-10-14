#include <dasmig/namegen.hpp>
#include <emscripten/bind.h>
#include <string>
#include "demo_support.hpp"

namespace
{

std::string to_utf8(const std::wstring& ws)
{
    std::string out;
    out.reserve(ws.size() * 3);
    for (wchar_t wc : ws)
    {
        if (wc <= 0x7F)
        {
            out.push_back(static_cast<char>(wc));
        }
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
        if (c <= 0x7F)
        {
            wc = c;
            i += 1;
        }
        else if ((c & 0xE0) == 0xC0)
        {
            wc = (c & 0x1F) << 6;
            if (i + 1 < s.size())
                wc |= (s[i + 1] & 0x3F);
            i += 2;
        }
        else if ((c & 0xF0) == 0xE0)
        {
            wc = (c & 0x0F) << 12;
            if (i + 1 < s.size())
                wc |= (s[i + 1] & 0x3F) << 6;
            if (i + 2 < s.size())
                wc |= (s[i + 2] & 0x3F);
            i += 3;
        }
        else
        {
            wc = (c & 0x07) << 18;
            if (i + 1 < s.size())
                wc |= (s[i + 1] & 0x3F) << 12;
            if (i + 2 < s.size())
                wc |= (s[i + 2] & 0x3F) << 6;
            if (i + 3 < s.size())
                wc |= (s[i + 3] & 0x3F);
            i += 4;
        }
        out.push_back(wc);
    }
    return out;
}

dasmig::culture parse_culture(const std::string& c)
{
    return dasmig::ng::to_culture(from_utf8(c));
}

dasmig::gender parse_gender(const std::string& g)
{
    return dasmig::ng::to_gender(from_utf8(g));
}

bool init()
{
    // The singleton loads the compiled-in lite tier (DASMIG_*_EMBED).
    return dasmig::ng::instance().has_resources();
}

std::string generate_name(const std::string& gender_str,
                          const std::string& culture_str,
                          bool with_surname)
{
    auto& gen = dasmig::ng::instance();
    auto g = gender_str.empty() ? dasmig::gender::any : parse_gender(gender_str);
    auto c = culture_str.empty() ? dasmig::culture::any : parse_culture(culture_str);

    auto n = gen.get_name({.gender = g, .culture = c});
    if (with_surname)
        n.append_surname();

    return to_utf8(static_cast<std::wstring>(n));
}

std::string generate_options(const std::string& gender_str, const std::string& culture_str,
                             const std::string& mode, int count)
{
    if (count < 1 || count > 10) return demo_error("Choose between 1 and 10 alternatives.");
    if (mode != "full" && mode != "first" && mode != "surname")
        return demo_error("Choose full name, first name or surname.");
    try
    {
        auto& gen = dasmig::ng::instance();
        const auto gender = gender_str.empty() ? dasmig::gender::any : parse_gender(gender_str);
        const auto culture = culture_str.empty() ? dasmig::culture::any : parse_culture(culture_str);
        const dasmig::name_request request{.gender = mode == "surname" ? dasmig::gender::any : gender,
                                           .culture = culture};
        dasmig::core::json_writer writer;
        writer.begin_array();
        for (int i = 0; i < count; ++i)
        {
            auto name = mode == "surname" ? gen.get_surname(request) : gen.get_name(request);
            if (mode == "full") name.append_surname();
            name.to_json(writer);
        }
        writer.end_array();
        return writer.str();
    }
    catch (const std::invalid_argument& error) { return demo_error(error.what()); }
    catch (const std::runtime_error& error) { return demo_error(error.what()); }
}

std::string generate_seeded(const std::string& gender_str, const std::string& culture_str,
                            const std::string& mode, const std::string& seed)
{
    try
    {
        if (mode != "full" && mode != "first" && mode != "surname")
            return demo_error("Choose full name, first name or surname.");
        const dasmig::name_request request{
            .gender = mode == "surname" || gender_str.empty() ? dasmig::gender::any : parse_gender(gender_str),
            .culture = culture_str.empty() ? dasmig::culture::any : parse_culture(culture_str)};
        auto& gen = dasmig::ng::instance();
        const auto call_seed = parse_demo_seed(seed);
        auto name = mode == "surname" ? gen.get_surname(request, call_seed) : gen.get_name(request, call_seed);
        if (mode == "full") name.append_surname();
        return name.to_json();
    }
    catch (const std::invalid_argument& error) { return demo_error(error.what()); }
    catch (const std::runtime_error& error) { return demo_error(error.what()); }
}

} // namespace

EMSCRIPTEN_BINDINGS(namegen_demo)
{
    emscripten::function("init", &init);
    emscripten::function("generateName", &generate_name);
    emscripten::function("generateNameOptions", &generate_options);
    emscripten::function("generateNameSeeded", &generate_seeded);
}
