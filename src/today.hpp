// Today's local date, for birth-generator's reference date.
//
// The libraries never read the clock (a seed must give the same result
// whenever it is replayed), so a caller that wants ages "as of now" passes
// the date itself. The demos do: ages and birthdays are relative to the day
// the page is opened.
#pragma once

#include <chrono>
#include <ctime>

inline std::chrono::year_month_day today()
{
    const std::time_t now = std::time(nullptr);
    std::tm local{};
    localtime_r(&now, &local);
    return std::chrono::year{local.tm_year + 1900} /
           std::chrono::month{static_cast<unsigned>(local.tm_mon + 1)} /
           std::chrono::day{static_cast<unsigned>(local.tm_mday)};
}
