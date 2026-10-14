#pragma once

#include <dasmig/core.hpp>
#include <charconv>
#include <chrono>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>

inline std::string demo_error(const std::string& message)
{
    dasmig::core::json_writer writer;
    writer.begin_object().field("error", message).end_object();
    return writer.str();
}

inline std::uint64_t parse_demo_seed(std::string_view text)
{
    std::uint64_t seed{};
    const auto result = std::from_chars(text.data(), text.data() + text.size(), seed, 16);
    if (text.empty() || text.size() > 16 || result.ec != std::errc{} ||
        result.ptr != text.data() + text.size())
        throw std::invalid_argument("Seed must contain 1 to 16 hexadecimal digits (0-9, a-f).");
    return seed;
}

inline std::chrono::year_month_day parse_reference_date(std::string_view text)
{
    auto number = [&](std::size_t offset, std::size_t length) {
        int value{};
        const auto part = text.substr(offset, length);
        const auto result = std::from_chars(part.data(), part.data() + part.size(), value);
        if (result.ec != std::errc{} || result.ptr != part.data() + part.size())
            throw std::invalid_argument("Reference date must be a valid YYYY-MM-DD date.");
        return value;
    };
    if (text.size() != 10 || text[4] != '-' || text[7] != '-')
        throw std::invalid_argument("Reference date must be a valid YYYY-MM-DD date.");
    const int year = number(0, 4);
    const auto date = std::chrono::year{year} /
        std::chrono::month{static_cast<unsigned>(number(5, 2))} /
        std::chrono::day{static_cast<unsigned>(number(8, 2))};
    if (year < 1900 || year > 2100 || !date.ok())
        throw std::invalid_argument("Choose a valid reference date between 1900 and 2100.");
    return date;
}
