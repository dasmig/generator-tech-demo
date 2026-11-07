#pragma once

// A person for the identity and finance demos: a birth and a name, which are what give
// a document its machine-readable zone and a card its holder. Adults, so that the
// documents and cards asked for can be held.

#include <dasmig/birthgen.hpp>
#include <dasmig/namegen.hpp>
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

struct demo_person
{
    std::string country;
    std::chrono::year_month_day birth_date;
    dasmig::sex bio_sex;
    std::string full_name;
    std::optional<std::string> given;
    std::optional<std::string> surname;        // the first family name, or the patronymic where there is none
    std::optional<std::string> second_surname; // the second of a double surname
};

inline dasmig::culture demo_culture(std::string country)
{
    std::ranges::transform(country, country.begin(), [](unsigned char c) { return static_cast<char>(c >= 'A' && c <= 'Z' ? c + 32 : c); });
    return dasmig::ng::to_culture(dasmig::core::from_utf8(country));
}

inline std::optional<std::string> demo_text(const std::string& text)
{
    if (text.empty()) return std::nullopt;
    return text;
}

inline std::string demo_name_part(const dasmig::name& n, dasmig::name_role role)
{
    std::string out;
    for (std::size_t i = 0; i < n.parts().size(); ++i)
    {
        if (n.roles()[i] != role) continue;
        if (!out.empty()) out.push_back(' ');
        out += dasmig::core::to_utf8(n.parts()[i]);
    }
    return out;
}

// The birth and the name of a person of `country`, drawn from their own streams.
inline demo_person draw_demo_person(const std::string& country, std::chrono::year_month_day reference_date, std::uint64_t seed)
{
    const auto culture = demo_culture(country);
    if (culture == dasmig::culture::any) throw std::invalid_argument("No names for country " + country + ".");
    dasmig::birth_request ask;
    ask.country_code = country;
    ask.ages = dasmig::age_range{.min = 18, .max = 80};
    ask.reference_date = reference_date;
    const auto birth = dasmig::bthg::instance().get_birth(ask, dasmig::core::rng{seed}.split("birth").seed());
    const auto n = dasmig::ng::instance().get_full_name(
        {.gender = birth.bio_sex == dasmig::sex::female ? dasmig::gender::f : dasmig::gender::m, .culture = culture},
        dasmig::core::rng{seed}.split("name").seed());
    return demo_person{.country = country,
                       .birth_date = std::chrono::year{birth.year} / std::chrono::month{birth.month} / std::chrono::day{birth.day},
                       .bio_sex = birth.bio_sex,
                       .full_name = dasmig::core::to_utf8(static_cast<std::wstring>(n)),
                       .given = demo_text(dasmig::core::to_utf8(n.given())),
                       .surname = demo_text(n.family().empty() ? dasmig::core::to_utf8(n.patronymic())
                                                               : demo_name_part(n, dasmig::name_role::family)),
                       .second_surname = demo_text(demo_name_part(n, dasmig::name_role::family2))};
}

// Those of `countries` a person can be drawn for: with names and births.
inline std::vector<std::string> demo_person_countries(const std::vector<std::string>& countries)
{
    std::vector<std::string> out;
    for (const auto& code : countries)
    {
        if (demo_culture(code) == dasmig::culture::any) continue;
        try
        {
            (void)draw_demo_person(code, std::chrono::year{2023} / std::chrono::month{7} / std::chrono::day{1}, 1);
            out.push_back(code);
        }
        catch (const std::invalid_argument&) // a country a library has no data for
        {
        }
    }
    return out;
}
