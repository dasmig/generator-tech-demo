#include "cpp23_polyfill.hpp"
#include <dasmig/nicknamegen.hpp>
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

bool init()
{
    // The singleton loads the compiled-in lite tier (DASMIG_*_EMBED).
    return dasmig::nng::instance().has_wordlists();
}

std::string generate_nickname(const std::string& input_name)
{
    auto& gen = dasmig::nng::instance();
    auto nick = gen.get_nickname(from_utf8(input_name));
    return to_utf8(static_cast<std::wstring>(nick));
}

std::string generate_random_nickname()
{
    auto& gen = dasmig::nng::instance();
    auto nick = gen.get_nickname();
    return to_utf8(static_cast<std::wstring>(nick));
}

} // namespace

EMSCRIPTEN_BINDINGS(nickgen_demo)
{
    emscripten::function("init", &init);
    emscripten::function("generateNickname", &generate_nickname);
    emscripten::function("generateRandomNickname", &generate_random_nickname);
}
