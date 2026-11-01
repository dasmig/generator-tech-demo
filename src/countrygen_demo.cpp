#include "cpp23_polyfill.hpp"
#include <dasmig/countrygen.hpp>
#include <emscripten/bind.h>
#include <array>
#include <chrono>
#include <cstdio>
#include <stdexcept>
#include <string>
#include "demo_support.hpp"

namespace
{

bool init()
{
    // The singleton loads the compiled-in lite tier (DASMIG_*_EMBED).
    return dasmig::cntg::instance().has_data();
}

std::string country_json(const dasmig::country& c)
{
    auto escape = [](const std::string& s) {
        std::string out;
        out.reserve(s.size());
        for (char ch : s)
        {
            switch (ch)
            {
                case '"':  out += "\\\""; break;
                case '\\': out += "\\\\"; break;
                case '\n': out += "\\n";  break;
                default:   out += ch;
            }
        }
        return out;
    };

    auto q = [&](const std::string& v) { return "\"" + escape(v) + "\""; };

    std::string json = "{";
    json += "\"Flag\":" + q(c.flag_emoji);
    json += ",\"Name\":" + q(c.name_common);
    json += ",\"Official Name\":" + q(c.name_official);
    json += ",\"CCA2\":" + q(c.cca2);
    json += ",\"CCA3\":" + q(c.cca3);
    json += ",\"Capital\":" + q(c.capital);
    json += ",\"Region\":" + q(c.region);
    json += ",\"Subregion\":" + q(c.subregion);
    json += ",\"Continent\":" + q(c.continent);
    json += ",\"Population\":" + std::to_string(c.population);
    json += ",\"Area (km²)\":" + std::to_string(c.area);
    json += ",\"Languages\":" + q(c.languages);
    json += ",\"Currency\":" + q(c.currency_name + " (" + c.currency_code + ") " + c.currency_symbol);
    json += ",\"Timezones\":" + q(c.timezones);
    json += ",\"Driving Side\":" + q(c.driving_side);
    json += ",\"TLD\":" + q(c.tld);
    json += ",\"Calling Code\":" + q(c.idd_root + c.idd_suffix);
    json += ",\"UN Member\":" + std::string(c.un_member ? "true" : "false");
    json += ",\"Independent\":" + std::string(c.independent ? "true" : "false");
    json += ",\"Landlocked\":" + std::string(c.landlocked ? "true" : "false");
    json += "}";
    return json;
}

std::string generate_country(const std::string& region)
{
    auto& gen = dasmig::cntg::instance();
    dasmig::country_request request;
    if (!region.empty()) request.region = region;
    return country_json(gen.get_country(request));
}

std::string sample_country(const std::string& region, bool weighted, std::uint32_t seed)
{
    auto& gen = dasmig::cntg::instance();
    try
    {
        dasmig::country_request request;
        if (!region.empty()) request.region = region;
        request.weighted = weighted;
        return country_json(gen.get_country(request, seed));
    }
    catch (const std::invalid_argument& error) { return demo_error(error.what()); }
    catch (const std::runtime_error& error) { return demo_error(error.what()); }
}

std::string sample_seeded(const std::string& region, bool weighted, const std::string& seed)
{
    try
    {
        dasmig::country_request request;
        if (!region.empty()) request.region = region;
        request.weighted = weighted;
        return dasmig::cntg::instance().get_country(request, parse_demo_seed(seed)).to_json();
    }
    catch (const std::invalid_argument& error) { return demo_error(error.what()); }
    catch (const std::runtime_error& error) { return demo_error(error.what()); }
}

unsigned catalogue_count() { return static_cast<unsigned>(dasmig::cntg::instance().country_count()); }

// Every country code the library has (the lite tier), for the country picker.
std::string supported_countries()
{
    const auto& gen = dasmig::cntg::instance();
    dasmig::core::json_writer w;
    w.begin_array();
    for (char a = 'A'; a <= 'Z'; ++a)
    {
        for (char b = 'A'; b <= 'Z'; ++b)
        {
            const std::string code{a, b};
            try
            {
                (void)gen.find_country(code);
                w.value(code);
            }
            catch (const std::invalid_argument&)
            {
            }
        }
    }
    w.end_array();
    return w.str();
}

std::string weekday_name(std::chrono::weekday d)
{
    static constexpr std::array<const char*, 7> names{"Sunday",   "Monday", "Tuesday", "Wednesday",
                                                       "Thursday", "Friday", "Saturday"};
    return names.at(d.c_encoding());
}

std::string offset_text(std::chrono::seconds offset)
{
    const auto total = offset.count();
    const auto minutes = (total < 0 ? -total : total) / 60;
    char text[8];
    std::snprintf(text, sizeof text, "%c%02d:%02d", total < 0 ? '-' : '+', static_cast<int>(minutes / 60),
                  static_cast<int>(minutes % 60));
    return text;
}

// A country's locales (each formatting the same date, time, number and price),
// its time zones (offset and local time now) and its holidays in a year. The
// page passes today's date and the current instant: the library never reads
// the clock.
std::string country_locale(const std::string& country, int year, bool optional, const std::string& today_text,
                           double now_seconds)
{
    using namespace std::chrono;
    const auto& gen = dasmig::cntg::instance();
    try
    {
        const auto today = parse_reference_date(today_text);
        const sys_seconds now{seconds{static_cast<long long>(now_seconds)}};
        const hh_mm_ss<seconds> afternoon{hours{15} + minutes{4}};
        const auto c = gen.find_country(country);
        dasmig::core::json_writer w;
        w.begin_object().field("country", c.cca2).field("name", c.name_common).field("flag", c.flag_emoji);

        w.key("locales").begin_array();
        for (const auto& loc : gen.locales(country))
        {
            const auto price = dasmig::core::money::from_minor(123450, loc.currency_code, loc.currency_digits);
            w.begin_object()
                .field("tag", loc.tag)
                .field("language", loc.language_name)
                .field("share", loc.share)
                .field("basis", loc.basis)
                .field("date", loc.format_date(today, dasmig::date_style::long_))
                .field("date_short", loc.format_date(today))
                .field("time", loc.format_time(afternoon))
                .field("number", loc.format_number(1234567.891))
                .field("money", loc.format_money(price))
                .field("first_day", weekday_name(loc.first_day))
                .field("measurement", loc.measurement == dasmig::measurement::metric ? "metric"
                                      : loc.measurement == dasmig::measurement::us   ? "US"
                                                                                     : "UK")
                .end_object();
        }
        w.end_array();

        w.key("zones").begin_array();
        for (const auto& tz : gen.time_zones(country))
        {
            const auto zone = gen.find_time_zone(tz.name, today);
            const auto local = zone.to_local(now);
            const auto day = floor<days>(local);
            const hh_mm_ss<seconds> clock{floor<seconds>(local - day)};
            char local_text[24];
            const year_month_day ymd{day};
            std::snprintf(local_text, sizeof local_text, "%04d-%02u-%02u %02d:%02d", static_cast<int>(ymd.year()),
                          static_cast<unsigned>(ymd.month()), static_cast<unsigned>(ymd.day()),
                          static_cast<int>(clock.hours().count()), static_cast<int>(clock.minutes().count()));
            w.begin_object()
                .field("name", zone.name)
                .field("share", zone.share)
                .field("utc_offset", offset_text(zone.utc_offset))
                .field("abbreviation", zone.abbreviation)
                .field("dst", zone.dst)
                .field("observes_dst", zone.observes_dst)
                .field("local_time", std::string{local_text})
                .end_object();
        }
        w.end_array();

        const auto span = gen.holiday_years(country);
        w.key("years").begin_array().value(span.from).value(span.to).end_array();
        if (year < span.from || year > span.to)
        {
            w.field("holidays_error", "Holidays for " + c.name_common + " are available from " +
                                          std::to_string(span.from) + " to " + std::to_string(span.to) + ".");
        }
        else
        {
            w.key("holidays").begin_array();
            for (const auto& h : gen.holidays({.country = country, .year = year, .optional = optional}))
            {
                const auto iso = [](year_month_day d)
                {
                    char text[11];
                    std::snprintf(text, sizeof text, "%04d-%02u-%02u", static_cast<int>(d.year()),
                                  static_cast<unsigned>(d.month()), static_cast<unsigned>(d.day()));
                    return std::string{text};
                };
                w.begin_object()
                    .field("date", iso(h.date))
                    .field("observed", h.observed ? iso(*h.observed) : std::string{})
                    .field("name", h.name)
                    .field("name_en", h.name_en)
                    .field("kind", h.kind == dasmig::holiday_kind::public_  ? "public"
                                   : h.kind == dasmig::holiday_kind::bank   ? "bank"
                                   : h.kind == dasmig::holiday_kind::optional ? "optional"
                                                                             : "observance")
                    .field("day", h.day)
                    .end_object();
            }
            w.end_array();
        }
        w.end_object();
        return w.str();
    }
    catch (const std::invalid_argument& error) { return demo_error(error.what()); }
    catch (const std::out_of_range& error) { return demo_error(error.what()); }
    catch (const std::runtime_error& error) { return demo_error(error.what()); }
}

} // namespace

EMSCRIPTEN_BINDINGS(countrygen_demo)
{
    emscripten::function("init", &init);
    emscripten::function("generateCountry", &generate_country);
    emscripten::function("sampleCountry", &sample_country);
    emscripten::function("sampleCountrySeeded", &sample_seeded);
    emscripten::function("catalogueCount", &catalogue_count);
    emscripten::function("supportedCountries", &supported_countries);
    emscripten::function("countryLocale", &country_locale);
}
