#include <dasmig/namegen.hpp>
#include <emscripten/bind.h>
#include <string>

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

    auto n = gen.get_name(g, c);
    if (with_surname)
        n.append_surname();

    return to_utf8(static_cast<std::wstring>(n));
}

} // namespace

EMSCRIPTEN_BINDINGS(namegen_demo)
{
    emscripten::function("init", &init);
    emscripten::function("generateName", &generate_name);
}
