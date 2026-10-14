#include "cpp23_polyfill.hpp"
#include <dasmig/entitygen.hpp>
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

void setup_rpg_components()
{
    auto& gen = dasmig::eg::instance();
    gen.clear();

    gen.add(std::make_unique<dasmig::choice_component<std::wstring>>(
        L"race", std::vector<std::wstring>{L"Human", L"Elf", L"Dwarf", L"Orc",
                                           L"Halfling", L"Gnome", L"Dragonborn",
                                           L"Tiefling"}));

    gen.add(std::make_unique<dasmig::choice_component<std::wstring>>(
        L"class", std::vector<std::wstring>{L"Warrior", L"Mage", L"Rogue",
                                            L"Cleric", L"Ranger", L"Paladin",
                                            L"Bard", L"Druid", L"Monk",
                                            L"Sorcerer", L"Warlock"}));

    gen.add(std::make_unique<dasmig::range_component<int>>(L"level", 1, 20));

    gen.add(std::make_unique<dasmig::range_component<int>>(L"strength", 3, 18));
    gen.add(std::make_unique<dasmig::range_component<int>>(L"dexterity", 3, 18));
    gen.add(std::make_unique<dasmig::range_component<int>>(L"constitution", 3, 18));
    gen.add(std::make_unique<dasmig::range_component<int>>(L"intelligence", 3, 18));
    gen.add(std::make_unique<dasmig::range_component<int>>(L"wisdom", 3, 18));
    gen.add(std::make_unique<dasmig::range_component<int>>(L"charisma", 3, 18));

    using wco = dasmig::weighted_choice_component<std::wstring>;
    gen.add(std::make_unique<wco>(
        L"alignment",
        std::vector<wco::option>{
            {L"Lawful Good", 1.0},
            {L"Neutral Good", 1.5},
            {L"Chaotic Good", 1.0},
            {L"Lawful Neutral", 1.0},
            {L"True Neutral", 1.5},
            {L"Chaotic Neutral", 1.5},
            {L"Lawful Evil", 0.5},
            {L"Neutral Evil", 0.5},
            {L"Chaotic Evil", 0.5}}));
}

std::string generate_entity()
{
    auto& gen = dasmig::eg::instance();
    auto entity = gen.generate({});
    auto m = entity.to_map();

    std::string result;
    bool first = true;
    for (auto& [key, val] : m)
    {
        if (!first) result += "  ";
        result += to_utf8(key) + ": " + to_utf8(val);
        first = false;
    }
    return result;
}

std::string generate_entity_json()
{
    auto& gen = dasmig::eg::instance();
    auto entity = gen.generate({});
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
    for (auto& [key, val] : m)
    {
        if (!first) json += ",";
        json += "\"" + escape(to_utf8(key)) + "\":\"" + escape(to_utf8(val)) + "\"";
        first = false;
    }
    json += "}";
    return json;
}

std::string generate_seeded(const std::string& seed, bool identity_only)
{
    try
    {
        dasmig::entity_request request;
        if (identity_only) request.component_keys = {L"race", L"class", L"alignment"};
        return dasmig::eg::instance().generate(request, parse_demo_seed(seed)).to_json();
    }
    catch (const std::invalid_argument& error) { return demo_error(error.what()); }
    catch (const std::runtime_error& error) { return demo_error(error.what()); }
}

} // namespace

EMSCRIPTEN_BINDINGS(entitygen_demo)
{
    emscripten::function("setup", &setup_rpg_components);
    emscripten::function("generateEntity", &generate_entity);
    emscripten::function("generateEntityJson", &generate_entity_json);
    emscripten::function("generateEntitySeeded", &generate_seeded);
}
