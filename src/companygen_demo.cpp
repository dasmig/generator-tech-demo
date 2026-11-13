#include <dasmig/companygen.hpp>
#include <emscripten/bind.h>
#include <array>
#include <charconv>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include "demo_support.hpp"
#include "today.hpp"

namespace
{

bool init()
{
    // The singleton loads the compiled-in lite tier (DASMIG_COMPANY_EMBED).
    return dasmig::cpg::instance().has_data();
}

int whole(const std::string& text, int lo, int hi, const char* message)
{
    int value{};
    const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
    if (result.ec != std::errc{} || result.ptr != text.data() + text.size() || value < lo || value > hi)
        throw std::invalid_argument(message);
    return value;
}

// industry: "" (drawn), an ISIC section letter or a division; size: "" or a band; employees: "" or a head count.
// counts: "enterprises" (the register's mix) or "employment" (a worker's employer).
std::string generate(const std::string& country, const std::string& industry, const std::string& size,
                     const std::string& employees, const std::string& legal_form, const std::string& counts,
                     bool incorporated_only, bool safe, const std::string& date, std::uint64_t seed)
{
    static constexpr std::array<std::string_view, 4> sizes{"micro", "small", "medium", "large"};
    if (counts != "enterprises" && counts != "employment")
        throw std::invalid_argument("Counts must be enterprises or employment.");
    if (!size.empty() && !employees.empty())
        throw std::invalid_argument("Choose a size or a head count, not both: a head count sets the size.");

    dasmig::company_request request;
    if (!country.empty()) request.country = country;
    if (!industry.empty()) request.isic = industry;
    if (!size.empty())
    {
        bool found = false;
        for (std::size_t i = 0; i < sizes.size(); ++i)
            if (sizes[i] == size)
            {
                request.size = static_cast<dasmig::size_band>(i);
                found = true;
            }
        if (!found) throw std::invalid_argument("Unknown size: " + size + ".");
    }
    if (!employees.empty())
        request.employees = static_cast<std::uint32_t>(whole(employees, 1, 100000000, "Head count must be 1 or more."));
    if (!legal_form.empty()) request.legal_form = legal_form;
    request.weight = counts == "employment" ? dasmig::company_weight::employment : dasmig::company_weight::enterprises;
    request.incorporated_only = incorporated_only;
    request.safe = safe;
    request.reference_date = date.empty() ? today() : parse_reference_date(date);

    const auto& gen = dasmig::cpg::instance();
    const auto company = gen.get_company(request, seed);
    const auto evidence = gen.evidence(company.country_code);

    dasmig::core::json_writer w;
    w.begin_object()
        .key("evidence")
        .begin_object()
        .field("industry", evidence.industry)
        .field("size", evidence.size)
        .field("legal_form", evidence.legal_form)
        .field("age", evidence.age)
        .field("name", evidence.name)
        .field("numbers", evidence.numbers)
        .field("domain", evidence.domain)
        .end_object()
        .key("company");
    company.to_json(w);
    w.end_object();
    return w.str();
}

std::string generate_company(const std::string& country, const std::string& industry, const std::string& size,
                             const std::string& employees, const std::string& legal_form, const std::string& counts,
                             bool incorporated_only, bool safe, const std::string& date)
{
    try
    {
        return generate(country, industry, size, employees, legal_form, counts, incorporated_only, safe, date,
                        dasmig::core::rng::entropy_seed());
    }
    catch (const std::logic_error& error) { return demo_error(error.what()); }
    catch (const std::runtime_error& error) { return demo_error(error.what()); }
}

std::string generate_company_seeded(const std::string& country, const std::string& industry, const std::string& size,
                                    const std::string& employees, const std::string& legal_form,
                                    const std::string& counts, bool incorporated_only, bool safe,
                                    const std::string& date, const std::string& seed)
{
    try
    {
        return generate(country, industry, size, employees, legal_form, counts, incorporated_only, safe, date,
                        parse_demo_seed(seed));
    }
    catch (const std::logic_error& error) { return demo_error(error.what()); }
    catch (const std::runtime_error& error) { return demo_error(error.what()); }
}

std::string supported_countries()
{
    dasmig::core::json_writer writer;
    writer.begin_array();
    for (const auto& code : dasmig::cpg::instance().countries()) writer.value(code);
    writer.end_array();
    return writer.str();
}

// The legal forms of a country: abbreviation, name and group, for the picker.
std::string legal_forms(const std::string& country)
{
    try
    {
        dasmig::core::json_writer writer;
        writer.begin_array();
        for (const auto& form : dasmig::cpg::instance().legal_forms(country))
            writer.begin_object()
                .field("code", form.code)
                .field("abbreviation", form.abbreviation)
                .field("name", form.name)
                .field("group", form.group)
                .end_object();
        writer.end_array();
        return writer.str();
    }
    catch (const std::logic_error&) { return "[]"; }
}

} // namespace

EMSCRIPTEN_BINDINGS(companygen_demo)
{
    emscripten::function("init", &init);
    emscripten::function("supportedCountries", &supported_countries);
    emscripten::function("legalForms", &legal_forms);
    emscripten::function("generateCompany", &generate_company);
    emscripten::function("generateCompanySeeded", &generate_company_seeded);
}
