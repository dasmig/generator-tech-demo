#include <dasmig/occupationgen.hpp>
#include <dasmig/propertygen.hpp>
#include <emscripten/bind.h>
#include <array>
#include <charconv>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include "demo_support.hpp"
#include "today.hpp"

namespace
{

bool init()
{
    // The singletons load the compiled-in lite tiers (DASMIG_*_EMBED).
    return dasmig::pty::instance().has_data() && dasmig::ocg::instance().has_data();
}

// An enum from its library name, by position in the enum (the same order as the CLI's options).
template <typename E, std::size_t N>
std::optional<E> named(const std::string& text, const std::array<std::string_view, N>& names, const char* what)
{
    if (text.empty()) return std::nullopt;
    for (std::size_t i = 0; i < N; ++i)
        if (names[i] == text) return static_cast<E>(i);
    throw std::invalid_argument(std::string{"Unknown "} + what + ": " + text + ".");
}

int whole(const std::string& text, int lo, int hi, const char* message)
{
    int value{};
    const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
    if (result.ec != std::errc{} || result.ptr != text.data() + text.size() || value < lo || value > hi)
        throw std::invalid_argument(message);
    return value;
}

double amount_of(const std::string& text)
{
    double value{};
    const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
    if (result.ec != std::errc{} || result.ptr != text.data() + text.size() || !(value >= 0.0) || value > 1e12)
        throw std::invalid_argument("Income must be an amount from 0 to 10^12.");
    return value;
}

// income: "earner" (one employed earner of the reference person's age, from occupation-generator),
// "amount" (the amount given, in the country's currency) or "none" (no cost).
std::string generate(const std::string& country, const std::string& area, const std::string& household_type,
                     const std::string& household_size, const std::string& type, const std::string& tenure,
                     const std::string& age, const std::string& income, const std::string& amount,
                     const std::string& date, std::uint64_t seed)
{
    static constexpr std::array<std::string_view, 3> areas{"city", "town", "rural"};
    static constexpr std::array<std::string_view, 6> types{"detached",   "semi_detached", "flat_small",
                                                           "flat_large", "informal",      "other"};
    static constexpr std::array<std::string_view, 4> tenures{"owner_outright", "owner_mortgage", "tenant_market",
                                                             "tenant_reduced"};
    if (income != "earner" && income != "amount" && income != "none")
        throw std::invalid_argument("Income must be earner, amount or none.");

    dasmig::property_request request;
    request.area = named<dasmig::settlement>(area, areas, "area");
    request.type = named<dasmig::dwelling_type>(type, types, "dwelling type");
    request.holding = named<dasmig::tenure>(tenure, tenures, "tenure");
    if (!household_type.empty()) request.household_type = household_type;
    if (!household_size.empty())
        request.household_size = static_cast<std::uint8_t>(whole(household_size, 1, 30, "Household size must be 1 to 30."));
    request.reference_date = date.empty() ? today() : parse_reference_date(date);

    // The country and the reference person's age come first: the earner is drawn for both.
    dasmig::core::rng draws{seed};
    if (country.empty())
    {
        dasmig::property_request draw;
        draw.reference_date = request.reference_date;
        request.country = dasmig::pty::instance().get_property(draw, draws.split("country").seed()).country_code;
    }
    else request.country = country;
    auto ages = draws.split("age");
    const auto anchor = age.empty() ? dasmig::core::uniform_int(ages, 18, 80)
                                    : whole(age, 16, 110, "The reference person's age must be 16 to 110.");
    request.anchor_age = static_cast<std::uint8_t>(anchor);

    std::optional<dasmig::occupation> earner;
    if (income == "earner")
    {
        dasmig::occupation_request r;
        r.country = *request.country;
        r.age = request.anchor_age.value();
        r.status = dasmig::employment_status::employed;
        earner = dasmig::ocg::instance().get_occupation(r, draws.split("earner").seed());
        request.household_income = earner->income;
    }
    else if (income == "amount" && !amount.empty())
    {
        const auto currency = dasmig::ocg::instance().income_currency(*request.country);
        if (!currency)
            throw std::invalid_argument("No income currency is known for " + *request.country + "; choose no income.");
        request.household_income = dasmig::core::money::from_major(amount_of(amount), currency->first, currency->second);
    }

    const auto home = dasmig::pty::instance().get_property(request, seed);
    const auto evidence = dasmig::pty::instance().evidence(home.country_code);

    dasmig::core::json_writer w;
    w.begin_object().field("anchor_age", static_cast<unsigned>(anchor)).key("earner");
    if (earner)
    {
        w.begin_object().field("job", earner->job ? earner->job->label : std::string{}).key("income");
        if (earner->income) earner->income->write_json(w);
        else w.value(nullptr);
        w.end_object();
    }
    else w.value(nullptr);
    w.key("household_income");
    if (request.household_income) request.household_income->write_json(w);
    else w.value(nullptr);
    w.key("evidence")
        .begin_object()
        .field("area", evidence.area)
        .field("household", evidence.household)
        .field("type", evidence.type)
        .field("tenure", evidence.tenure)
        .field("rooms", evidence.rooms)
        .field("floor_area", evidence.floor_area)
        .field("period", evidence.period)
        .field("cost", evidence.cost)
        .end_object()
        .key("home");
    home.to_json(w);
    w.end_object();
    return w.str();
}

std::string generate_property(const std::string& country, const std::string& area, const std::string& household_type,
                              const std::string& household_size, const std::string& type, const std::string& tenure,
                              const std::string& age, const std::string& income, const std::string& amount,
                              const std::string& date)
{
    try
    {
        return generate(country, area, household_type, household_size, type, tenure, age, income, amount, date,
                        dasmig::core::rng::entropy_seed());
    }
    catch (const std::logic_error& error) { return demo_error(error.what()); }
    catch (const std::runtime_error& error) { return demo_error(error.what()); }
}

std::string generate_property_seeded(const std::string& country, const std::string& area,
                                     const std::string& household_type, const std::string& household_size,
                                     const std::string& type, const std::string& tenure, const std::string& age,
                                     const std::string& income, const std::string& amount, const std::string& date,
                                     const std::string& seed)
{
    try
    {
        return generate(country, area, household_type, household_size, type, tenure, age, income, amount, date,
                        parse_demo_seed(seed));
    }
    catch (const std::logic_error& error) { return demo_error(error.what()); }
    catch (const std::runtime_error& error) { return demo_error(error.what()); }
}

std::string supported_countries()
{
    dasmig::core::json_writer writer;
    writer.begin_array();
    for (const auto& code : dasmig::pty::instance().countries()) writer.value(code);
    writer.end_array();
    return writer.str();
}

// The currency of a country's incomes, or an empty string when none is known.
std::string income_currency(const std::string& country)
{
    const auto currency = dasmig::ocg::instance().income_currency(country);
    return currency ? currency->first : std::string{};
}

} // namespace

EMSCRIPTEN_BINDINGS(propertygen_demo)
{
    emscripten::function("init", &init);
    emscripten::function("supportedCountries", &supported_countries);
    emscripten::function("incomeCurrency", &income_currency);
    emscripten::function("generateProperty", &generate_property);
    emscripten::function("generatePropertySeeded", &generate_property_seeded);
}
