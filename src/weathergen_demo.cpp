#include "cpp23_polyfill.hpp"
#include <dasmig/weathergen.hpp>
#include <emscripten/bind.h>
#include <string>
#include "demo_support.hpp"

namespace
{

bool init()
{
    // The singleton loads the compiled-in lite tier (DASMIG_WEATHER_EMBED).
    return dasmig::wtg::instance().has_data();
}

dasmig::weather_request place(double latitude, double longitude, const std::string& units)
{
    if (units != "metric" && units != "imperial") throw std::invalid_argument("Units must be metric or imperial.");
    dasmig::weather_request request;
    request.latitude = latitude;
    request.longitude = longitude;
    request.units = units == "imperial" ? dasmig::unit_system::imperial : dasmig::unit_system::metric;
    return request;
}

// A year of days at a point, with the cell's normals: {"normals":{...},"days":[...]}.
std::string year_seeded(double latitude, double longitude, int year, const std::string& units,
                        const std::string& seed)
{
    try
    {
        if (year < 1900 || year > 2100) throw std::invalid_argument("Choose a year between 1900 and 2100.");
        auto request = place(latitude, longitude, units);
        request.date = std::chrono::year{year} / 1 / 1;
        const int days = std::chrono::year{year}.is_leap() ? 366 : 365;
        const auto& gen = dasmig::wtg::instance();
        const auto series = gen.get_weather_series(request, days, parse_demo_seed(seed));
        dasmig::core::json_writer w;
        w.begin_object().key("normals");
        gen.climate_at(latitude, longitude).to_json(w);
        w.key("days").begin_array();
        for (const auto& day : series) day.to_json(w);
        w.end_array().end_object();
        return w.str();
    }
    catch (const std::invalid_argument& error) { return demo_error(error.what()); }
    catch (const std::runtime_error& error) { return demo_error(error.what()); }
}

// One day with its 24 hours; the same day as in the year with that seed.
std::string day_seeded(double latitude, double longitude, const std::string& date, const std::string& units,
                       const std::string& seed)
{
    try
    {
        auto request = place(latitude, longitude, units);
        request.date = parse_reference_date(date);
        request.hourly = true;
        return dasmig::wtg::instance().get_weather(request, parse_demo_seed(seed)).to_json();
    }
    catch (const std::invalid_argument& error) { return demo_error(error.what()); }
    catch (const std::runtime_error& error) { return demo_error(error.what()); }
}

} // namespace

EMSCRIPTEN_BINDINGS(weathergen_demo)
{
    emscripten::function("init", &init);
    emscripten::function("yearSeeded", &year_seeded);
    emscripten::function("daySeeded", &day_seeded);
}
