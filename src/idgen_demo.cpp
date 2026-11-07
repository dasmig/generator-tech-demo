#include <dasmig/idgen.hpp>
#include <emscripten/bind.h>
#include <cstdint>
#include <fstream>
#include <map>
#include <optional>
#include <random>
#include <ranges>
#include <set>
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
    return dasmig::idg::instance().has_data() && dasmig::bthg::instance().has_data();
}

const std::vector<std::string>& countries()
{
    static const auto list = demo_person_countries(dasmig::idg::instance().countries());
    return list;
}

std::vector<std::string> split_tabs(const std::string& line)
{
    std::vector<std::string> cells;
    std::size_t from = 0;
    while (true)
    {
        const auto tab = line.find('\t', from);
        cells.push_back(line.substr(from, tab - from));
        if (tab == std::string::npos) return cells;
        from = tab + 1;
    }
}

// What each scheme encodes, from the survey of number schemes id-generator's data
// was curated from. The page preloads the file (see CMakeLists.txt).
const std::map<std::string, std::string>& scheme_encodes()
{
    static const auto table = []
    {
        std::map<std::string, std::string> out;
        std::ifstream file{"/survey_numbers.tsv"};
        std::string line;
        std::size_t scheme = 0;
        std::size_t encodes = 0;
        bool header = true;
        while (std::getline(file, line))
        {
            if (line.empty() || line.front() == '#') continue;
            if (line.back() == '\r') line.pop_back();
            const auto cells = split_tabs(line);
            if (header)
            {
                scheme = static_cast<std::size_t>(std::ranges::find(cells, "scheme") - cells.begin());
                encodes = static_cast<std::size_t>(std::ranges::find(cells, "encodes") - cells.begin());
                if (encodes >= cells.size() || scheme >= cells.size()) throw std::runtime_error("The number survey has no encodes column.");
                header = false;
            }
            else if (cells.size() > std::max(scheme, encodes))
            {
                out.emplace(cells[scheme], cells[encodes]);
            }
        }
        return out;
    }();
    return table;
}

// Empty documents are none; "passport", "id_card" or "both". Date is YYYY-MM-DD or empty (today).
dasmig::identity_request make_request(const std::string& documents, const std::string& date, bool safe)
{
    dasmig::identity_request request;
    if (documents == "passport" || documents == "both") request.documents.push_back(dasmig::id_document_kind::passport);
    if (documents == "id_card" || documents == "both") request.documents.push_back(dasmig::id_document_kind::id_card);
    if (!documents.empty() && request.documents.empty()) throw std::invalid_argument("Documents must be passport, id_card or both.");
    request.reference_date = date.empty() ? today() : parse_reference_date(date);
    request.safe = safe;
    return request;
}

// A country by population among those with names, the same way each time for a seed.
std::string drawn_country(const dasmig::identity_request& base, std::uint64_t seed)
{
    const auto& allowed = countries();
    const auto draws = dasmig::core::rng{seed}.split("country").seed();
    dasmig::identity_request draw;
    draw.reference_date = base.reference_date;
    for (std::uint64_t attempt = 0; attempt < 256; ++attempt)
    {
        auto code = dasmig::idg::instance().get_identity(draw, dasmig::core::hash_combine(draws, attempt)).country_code;
        if (std::ranges::find(allowed, code) != allowed.end()) return code;
    }
    return allowed.front();
}

std::string generate(const std::string& country, const std::string& documents, const std::string& date, bool safe, std::uint64_t seed)
{
    auto request = make_request(documents, date, safe);
    const auto code = country.empty() ? drawn_country(request, seed) : country;
    const auto person = draw_demo_person(code, request.reference_date, seed);
    request.country = person.country;
    request.birth_date = person.birth_date;
    request.sex = person.bio_sex;
    request.given_names = person.given;
    request.surname = person.surname;
    request.second_surname = person.second_surname;
    const auto item = dasmig::idg::instance().get_identity(request, seed);

    dasmig::core::json_writer w;
    w.begin_object().field("name", person.full_name).key("identity");
    item.to_json(w);
    w.key("encodes").begin_object();
    std::set<std::string> written;
    for (const auto& number : item.numbers)
    {
        const auto found = scheme_encodes().find(number.scheme);
        if (found != scheme_encodes().end() && written.insert(number.scheme).second) w.field(number.scheme, found->second);
    }
    w.end_object().end_object();
    return w.str();
}

std::string generate_identity(const std::string& country, const std::string& documents, const std::string& date, bool safe)
{
    try
    {
        return generate(country, documents, date, safe, dasmig::core::rng::entropy_seed());
    }
    catch (const std::logic_error& error) { return demo_error(error.what()); }
    catch (const std::runtime_error& error) { return demo_error(error.what()); }
}

std::string generate_identity_seeded(const std::string& country, const std::string& documents, const std::string& date, bool safe,
                                     const std::string& seed)
{
    try
    {
        return generate(country, documents, date, safe, parse_demo_seed(seed));
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

} // namespace

EMSCRIPTEN_BINDINGS(idgen_demo)
{
    emscripten::function("init", &init);
    emscripten::function("supportedCountries", &supported_countries);
    emscripten::function("generateIdentity", &generate_identity);
    emscripten::function("generateIdentitySeeded", &generate_identity_seeded);
}
