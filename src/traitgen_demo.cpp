#include <dasmig/traitgen.hpp>
#include <emscripten/bind.h>
#include <charconv>
#include <cstdint>
#include <string>

namespace
{

bool init()
{
    return dasmig::trtg::instance().has_data();
}

std::string error(const std::string& message)
{
    dasmig::core::json_writer writer;
    writer.begin_object().field("error", message).end_object();
    return writer.str();
}

std::string generate_traits(int count, const std::string& seed)
{
    auto& gen = dasmig::trtg::instance();
    if (!gen.has_data()) return error("Trait data is not loaded. Reload the page to try again.");
    if (count < 1 || static_cast<std::size_t>(count) > gen.trait_count())
        return error("Trait count must be between 1 and the catalogue size.");

    dasmig::trait_request request;
    request.count = static_cast<std::size_t>(count);
    if (seed.empty()) return gen.get_personality(request).to_json();

    std::uint64_t call_seed{};
    const auto result = std::from_chars(seed.data(), seed.data() + seed.size(), call_seed, 16);
    if (seed.size() > 16 || result.ec != std::errc{} || result.ptr != seed.data() + seed.size())
        return error("Seed must contain 1 to 16 hexadecimal digits (0-9, a-f).");

    return gen.get_personality(request, call_seed).to_json();
}

} // namespace

EMSCRIPTEN_BINDINGS(traitgen_demo)
{
    emscripten::function("init", &init);
    emscripten::function("generateTraits", &generate_traits);
}
