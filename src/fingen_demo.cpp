#include <dasmig/fingen.hpp>
#include <emscripten/bind.h>
#include <cstdint>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <string>
#include <vector>
#include "demo_person.hpp"
#include "demo_support.hpp"
#include "today.hpp"

namespace
{

bool init()
{
    // The singletons load the compiled-in lite tiers (DASMIG_*_EMBED).
    return dasmig::fng::instance().has_data() && dasmig::bthg::instance().has_data();
}

const std::vector<std::string>& countries()
{
    static const auto list = demo_person_countries(dasmig::fng::instance().countries());
    return list;
}

// accounts: "current", "savings", "both" or "none". card: "debit", "credit", "prepaid", "any" or "none".
// source: "test_number", "wrong_check" or "network_range". network: empty to draw it. Date: YYYY-MM-DD or empty (today).
dasmig::finance_request make_request(const std::string& accounts, const std::string& card, const std::string& source,
                                     const std::string& network, const std::string& date, bool safe)
{
    dasmig::finance_request request;
    request.accounts.clear();
    if (accounts == "current" || accounts == "both") request.accounts.push_back(dasmig::account_kind::current);
    if (accounts == "savings" || accounts == "both") request.accounts.push_back(dasmig::account_kind::savings);
    if (accounts != "none" && request.accounts.empty()) throw std::invalid_argument("Accounts must be current, savings, both or none.");
    if (card == "debit") request.cards.emplace_back(dasmig::card_kind::debit);
    else if (card == "credit") request.cards.emplace_back(dasmig::card_kind::credit);
    else if (card == "prepaid") request.cards.emplace_back(dasmig::card_kind::prepaid);
    else if (card == "any") request.cards.emplace_back(std::nullopt);
    else if (card != "none") throw std::invalid_argument("Card must be debit, credit, prepaid, any or none.");
    if (source == "test_number") request.card_numbers = dasmig::card_source::test_number;
    else if (source == "wrong_check") request.card_numbers = dasmig::card_source::wrong_check;
    else if (source == "network_range") request.card_numbers = dasmig::card_source::network_range;
    else throw std::invalid_argument("Card numbers must be test_number, wrong_check or network_range.");
    if (!network.empty()) request.network = network;
    request.reference_date = date.empty() ? today() : parse_reference_date(date);
    request.safe = safe;
    return request;
}

// A country by population among those with names, the same way each time for a seed.
std::string drawn_country(const dasmig::finance_request& base, std::uint64_t seed)
{
    const auto& allowed = countries();
    const auto draws = dasmig::core::rng{seed}.split("country").seed();
    dasmig::finance_request draw;
    draw.accounts.clear();
    draw.reference_date = base.reference_date;
    for (std::uint64_t attempt = 0; attempt < 256; ++attempt)
    {
        auto code = dasmig::fng::instance().get_finance(draw, dasmig::core::hash_combine(draws, attempt)).country_code;
        if (std::ranges::find(allowed, code) != allowed.end()) return code;
    }
    return allowed.front();
}

std::string generate(const std::string& country, const std::string& accounts, const std::string& card, const std::string& source,
                     const std::string& network, const std::string& date, bool safe, std::uint64_t seed)
{
    auto request = make_request(accounts, card, source, network, date, safe);
    const auto code = country.empty() ? drawn_country(request, seed) : country;
    const auto person = draw_demo_person(code, request.reference_date, seed);
    request.country = person.country;
    request.birth_date = person.birth_date;
    request.given_names = person.given;
    request.surname = person.surname;
    const auto item = dasmig::fng::instance().get_finance(request, seed);

    dasmig::core::json_writer w;
    w.begin_object().field("name", person.full_name).key("finance");
    item.to_json(w);
    w.end_object();
    return w.str();
}

std::string generate_finance(const std::string& country, const std::string& accounts, const std::string& card, const std::string& source,
                             const std::string& network, const std::string& date, bool safe)
{
    try
    {
        return generate(country, accounts, card, source, network, date, safe, dasmig::core::rng::entropy_seed());
    }
    catch (const std::logic_error& error) { return demo_error(error.what()); }
    catch (const std::runtime_error& error) { return demo_error(error.what()); }
}

std::string generate_finance_seeded(const std::string& country, const std::string& accounts, const std::string& card, const std::string& source,
                                    const std::string& network, const std::string& date, bool safe, const std::string& seed)
{
    try
    {
        return generate(country, accounts, card, source, network, date, safe, parse_demo_seed(seed));
    }
    catch (const std::logic_error& error) { return demo_error(error.what()); }
    catch (const std::runtime_error& error) { return demo_error(error.what()); }
}

std::string json_list(const std::vector<std::string>& items)
{
    dasmig::core::json_writer writer;
    writer.begin_array();
    for (const auto& item : items) writer.value(item);
    writer.end_array();
    return writer.str();
}

std::string supported_countries() { return json_list(countries()); }

// The networks of a country, for any kind of card, as a JSON array.
std::string networks(const std::string& country)
{
    try
    {
        return json_list(dasmig::fng::instance().networks(country));
    }
    catch (const std::logic_error& error) { return demo_error(error.what()); }
}

} // namespace

EMSCRIPTEN_BINDINGS(fingen_demo)
{
    emscripten::function("init", &init);
    emscripten::function("supportedCountries", &supported_countries);
    emscripten::function("networks", &networks);
    emscripten::function("generateFinance", &generate_finance);
    emscripten::function("generateFinanceSeeded", &generate_finance_seeded);
}
