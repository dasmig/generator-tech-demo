#include <dasmig/vehiclegen.hpp>
#include <emscripten/bind.h>
#include <array>
#include <cstdint>
#include <format>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>
#include "demo_support.hpp"
#include "today.hpp"

namespace
{

bool init()
{
    // The singleton loads the compiled-in lite tier (DASMIG_VEHICLE_EMBED).
    return dasmig::vhg::instance().has_data();
}

template <typename Enum, std::size_t N>
std::optional<Enum> enum_of(const std::string& text, const char* what)
{
    if (text.empty()) return std::nullopt;
    for (std::size_t i = 0; i < N; ++i)
        if (dasmig::to_string(static_cast<Enum>(i)) == text) return static_cast<Enum>(i);
    throw std::invalid_argument(std::string{what} + " " + text + " is not known.");
}

// Empty strings mean "drawn"; kind is "car" or "motorcycle"; units is "country", "metric" or "imperial";
// date is YYYY-MM-DD or empty (today).
dasmig::vehicle_request make_request(const std::string& country, const std::string& region, const std::string& make,
                                     const std::string& kind, const std::string& body, const std::string& fuel, const std::string& date,
                                     const std::string& units, bool safe)
{
    dasmig::vehicle_request request;
    if (!country.empty()) request.country = country;
    if (!region.empty()) request.region = region;
    if (!make.empty()) request.make = make;
    request.kind = enum_of<dasmig::vehicle_kind, 2>(kind, "Kind");
    request.body = enum_of<dasmig::vehicle_body, 16>(body, "Body");
    request.fuel = enum_of<dasmig::vehicle_fuel, 8>(fuel, "Fuel");
    request.reference_date = date.empty() ? today() : parse_reference_date(date);
    if (units == "metric") request.units = dasmig::distance_units::metric;
    else if (units == "imperial") request.units = dasmig::distance_units::imperial;
    else if (!units.empty() && units != "country") throw std::invalid_argument("Units must be country, metric or imperial.");
    request.safe = safe;
    return request;
}

std::string iso_date(const std::chrono::year_month_day& d)
{
    return std::format("{:04}-{:02}-{:02}", static_cast<int>(d.year()), static_cast<unsigned>(d.month()),
                       static_cast<unsigned>(d.day()));
}

std::string or_dash(const std::string& text) { return text.empty() ? "-" : text; }

// 54300 -> "54,300", as the library's summary writes the odometer.
std::string grouped(unsigned n)
{
    const std::string digits = std::to_string(n);
    std::string out;
    for (std::size_t i = 0; i < digits.size(); ++i)
    {
        if (i > 0 && (digits.size() - i) % 3 == 0) out += ',';
        out += digits[i];
    }
    return out;
}

// The result as the demo's table shows it.
std::string describe(const dasmig::vehicle& v)
{
    const std::string engine = v.engine_cc == 0 ? std::format("electric, {} kW", v.power_kw)
                                                 : std::format("{} cc, {} kW", v.engine_cc, v.power_kw);
    dasmig::core::json_writer writer;
    writer.begin_object()
        .field("Summary", v.to_string())
        .field("Country", v.country_code)
        .field("Kind", std::string{dasmig::to_string(v.kind)})
        .field("Vehicle", std::format("{} {} {}{}", v.model_year, v.make, v.model, v.trim.empty() ? "" : " " + v.trim))
        .field("Body", std::string{dasmig::to_string(v.body)})
        .field("Fuel", std::string{dasmig::to_string(v.fuel)})
        .field("Engine", engine)
        .field("Gearbox and drive", v.transmission + ", " + v.drive)
        .field(v.doors == 0 ? "Seats" : "Doors and seats",
               v.doors == 0 ? std::format("{} (rider and passenger)", v.seats)
                            : std::format("{} doors, {} seats", v.doors, v.seats))
        .field("Colour", or_dash(v.colour) + (v.colour_family.empty() ? "" : " (" + v.colour_family + ")"))
        .field("First registration", iso_date(v.first_registration))
        .field("Odometer", grouped(v.odometer) + " " + v.odometer_unit)
        .field("Plate", or_dash(v.plate))
        .field("Plate region", or_dash(v.plate_region))
        .field("Plate format", or_dash(v.plate_format))
        .field("VIN", or_dash(v.vin))
        .field("WMI", or_dash(v.wmi))
        .field("Unissued plate and VIN", v.reserved)
        .field("Seed (hex)", std::format("{:016x}", v.signature().seed))
        .end_object();
    return writer.str();
}

std::string generate_vehicle(const std::string& country, const std::string& region, const std::string& make,
                             const std::string& kind, const std::string& body, const std::string& fuel, const std::string& date,
                             const std::string& units, bool safe)
{
    try
    {
        return describe(dasmig::vhg::instance().get_vehicle(
            make_request(country, region, make, kind, body, fuel, date, units, safe)));
    }
    catch (const std::logic_error& error) { return demo_error(error.what()); }
    catch (const std::runtime_error& error) { return demo_error(error.what()); }
}

std::string generate_vehicle_seeded(const std::string& country, const std::string& region, const std::string& make,
                                    const std::string& kind, const std::string& body, const std::string& fuel, const std::string& date,
                                    const std::string& units, bool safe, const std::string& seed)
{
    try
    {
        return dasmig::vhg::instance()
            .get_vehicle(make_request(country, region, make, kind, body, fuel, date, units, safe), parse_demo_seed(seed))
            .to_json();
    }
    catch (const std::logic_error& error) { return demo_error(error.what()); }
    catch (const std::runtime_error& error) { return demo_error(error.what()); }
}

// One call seed in five countries: each part draws from its own stream, so
// only what the country changes moves (its fleet, its plates, its factories).
std::string compare_countries(const std::string& kind, const std::string& body, const std::string& fuel, const std::string& date, bool safe)
{
    try
    {
        auto& gen = dasmig::vhg::instance();
        auto request = make_request("", "", "", kind, body, fuel, date, "country", safe);
        const auto seed = gen.get_vehicle(request).seed();
        constexpr std::array countries{"DE", "GB", "ES", "US", "JP"};
        dasmig::core::json_writer writer;
        writer.begin_array();
        for (const auto* country : countries)
        {
            request.country = country;
            gen.get_vehicle(request, seed).to_json(writer);
        }
        writer.end_array();
        return writer.str();
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

// The country codes a request can name, as a JSON array.
std::string supported_countries() { return json_list(dasmig::vhg::instance().countries()); }

// The makes sold in a country (every make for an empty country) of a kind (both for an empty kind), as a JSON array.
std::string makes(const std::string& country, const std::string& kind)
{
    try
    {
        return json_list(dasmig::vhg::instance().makes(country, enum_of<dasmig::vehicle_kind, 2>(kind, "Kind")));
    }
    catch (const std::logic_error& error) { return demo_error(error.what()); }
}

// The plate regions of a country (US states, German districts, ...), as a JSON array.
std::string plate_regions(const std::string& country)
{
    try
    {
        return json_list(dasmig::vhg::instance().plate_regions(country));
    }
    catch (const std::logic_error& error) { return demo_error(error.what()); }
}

} // namespace

EMSCRIPTEN_BINDINGS(vehiclegen_demo)
{
    emscripten::function("init", &init);
    emscripten::function("supportedCountries", &supported_countries);
    emscripten::function("makes", &makes);
    emscripten::function("plateRegions", &plate_regions);
    emscripten::function("generateVehicle", &generate_vehicle);
    emscripten::function("generateVehicleSeeded", &generate_vehicle_seeded);
    emscripten::function("compareCountries", &compare_countries);
}
