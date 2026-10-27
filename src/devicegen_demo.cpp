#include <dasmig/devicegen.hpp>
#include <emscripten/bind.h>
#include <array>
#include <chrono>
#include <cstdint>
#include <format>
#include <stdexcept>
#include <string>
#include "demo_support.hpp"
#include "today.hpp"

namespace
{

bool init()
{
    // The singleton loads the compiled-in lite tier (DASMIG_DEVICE_EMBED).
    return dasmig::dvg::instance().has_data();
}

// Empty strings mean "drawn"; ip is "v4", "v6" or empty (any); date is
// YYYY-MM-DD or empty (today, seen as the data's newest release when later).
dasmig::device_request make_request(const std::string& country, const std::string& platform,
                                    const std::string& browser, const std::string& ip, const std::string& locale,
                                    const std::string& date, bool safe)
{
    dasmig::device_request request;
    if (!country.empty()) request.country = country;
    if (!platform.empty()) request.platform = platform;
    if (!browser.empty()) request.browser = browser;
    if (ip == "v4") request.ip = dasmig::ip_version::v4;
    else if (ip == "v6") request.ip = dasmig::ip_version::v6;
    else if (!ip.empty()) throw std::invalid_argument("IP must be v4, v6 or empty.");
    request.locale = locale;
    request.reference_date = date.empty() ? today() : parse_reference_date(date);
    request.safe = safe;
    return request;
}

std::string screen_text(const dasmig::device_screen& s)
{
    if (s.width <= 0) return "-";
    return std::format("{} × {} CSS px at {}x", s.width, s.height, s.pixel_ratio);
}

std::string or_dash(const std::string& text) { return text.empty() ? "-" : text; }

// "Samsung Galaxy A54", but "Xiaomi Pad 5" rather than "Xiaomi Xiaomi Pad 5".
std::string maker_and_model(const dasmig::device& d)
{
    if (d.model.empty()) return or_dash(d.maker);
    if (d.maker.empty() || d.model.starts_with(d.maker + " ")) return d.model;
    return d.maker + " " + d.model;
}

// The result as the demo's table shows it.
std::string describe(const dasmig::device& d)
{
    dasmig::core::json_writer writer;
    writer.begin_object()
        .field("Summary", d.to_string())
        .field("Country", d.country_code)
        .field("Platform", d.platform_label)
        .field("Maker and model", maker_and_model(d))
        .field("OS", or_dash(d.os + (d.os_version.empty() ? "" : " " + d.os_version)))
        .field("Browser", d.browser.empty() ? "-" : dasmig::device::browser_name(d.browser) + " " + d.browser_version)
        .field("Engine", or_dash(d.engine))
        .field("User-Agent", or_dash(d.user_agent))
        .field("Sec-CH-UA", or_dash(d.sec_ch_ua))
        .field("Sec-CH-UA-Mobile", or_dash(d.sec_ch_ua_mobile))
        .field("Sec-CH-UA-Platform", or_dash(d.sec_ch_ua_platform))
        .field("Accept-Language", or_dash(d.accept_language))
        .field("Screen", screen_text(d.screen))
        .field("IP address", d.ip)
        .field("Documentation address", d.reserved)
        .field("Seed (hex)", std::format("{:016x}", d.signature().seed))
        .end_object();
    return writer.str();
}

std::string generate_device(const std::string& country, const std::string& platform, const std::string& browser,
                            const std::string& ip, const std::string& locale, const std::string& date, bool safe)
{
    try
    {
        return describe(
            dasmig::dvg::instance().get_device(make_request(country, platform, browser, ip, locale, date, safe)));
    }
    catch (const std::invalid_argument& error) { return demo_error(error.what()); }
    catch (const std::runtime_error& error) { return demo_error(error.what()); }
}

std::string generate_device_seeded(const std::string& country, const std::string& platform,
                                   const std::string& browser, const std::string& ip, const std::string& locale,
                                   const std::string& date, bool safe, const std::string& seed)
{
    try
    {
        return dasmig::dvg::instance()
            .get_device(make_request(country, platform, browser, ip, locale, date, safe), parse_demo_seed(seed))
            .to_json();
    }
    catch (const std::invalid_argument& error) { return demo_error(error.what()); }
    catch (const std::runtime_error& error) { return demo_error(error.what()); }
}

// One call seed seen at five dates: the model, OS and browser versions and the
// user agent move with the date; the country, platform and address stay.
std::string compare_dates(const std::string& country, const std::string& platform, const std::string& browser)
{
    try
    {
        auto& gen = dasmig::dvg::instance();
        auto request = make_request(country, platform, browser, "", "", "", false);
        const auto anchor = gen.get_device(request);
        request.country = anchor.country_code;
        request.platform = anchor.platform;
        const auto now = today();
        const std::array dates{std::chrono::year{2021} / 6 / 1, std::chrono::year{2022} / 6 / 1,
                               std::chrono::year{2023} / 6 / 1, std::chrono::year{2024} / 6 / 1, now};
        dasmig::core::json_writer writer;
        writer.begin_array();
        for (const auto& date : dates)
        {
            request.reference_date = date;
            writer.begin_object()
                .field("date", std::format("{:04}-{:02}-{:02}", static_cast<int>(date.year()),
                                           static_cast<unsigned>(date.month()), static_cast<unsigned>(date.day())))
                .key("device");
            gen.get_device(request, anchor.seed()).to_json(writer);
            writer.end_object();
        }
        writer.end_array();
        return writer.str();
    }
    catch (const std::invalid_argument& error) { return demo_error(error.what()); }
    catch (const std::runtime_error& error) { return demo_error(error.what()); }
}

// The country codes a request can name, as a JSON array.
std::string supported_countries()
{
    dasmig::core::json_writer writer;
    writer.begin_array();
    for (const auto& code : dasmig::dvg::instance().countries()) writer.value(code);
    writer.end_array();
    return writer.str();
}

// The platform keys a request can name, as a JSON array.
std::string supported_platforms()
{
    dasmig::core::json_writer writer;
    writer.begin_array();
    for (const auto& key : dasmig::dvg::instance().platforms()) writer.value(key);
    writer.end_array();
    return writer.str();
}

} // namespace

EMSCRIPTEN_BINDINGS(devicegen_demo)
{
    emscripten::function("init", &init);
    emscripten::function("supportedCountries", &supported_countries);
    emscripten::function("supportedPlatforms", &supported_platforms);
    emscripten::function("generateDevice", &generate_device);
    emscripten::function("generateDeviceSeeded", &generate_device_seeded);
    emscripten::function("compareDates", &compare_dates);
}
