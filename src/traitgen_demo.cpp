#include <dasmig/traitgen.hpp>
#include <emscripten/bind.h>
#include <cstdint>
#include <string>
#include "demo_support.hpp"

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

std::string explained(const dasmig::personality& personality)
{
    dasmig::core::json_writer writer;
    writer.begin_object().key("axes").begin_array();
    for (const auto& axis : personality.axes)
        writer.begin_object().field("label", axis.label).field("base", axis.base)
            .field("effect", axis.value - axis.base).field("value", axis.value).end_object();
    writer.end_array().key("traits").begin_array();
    for (const auto& trait : personality.traits)
    {
        // An include-only draw with the same seed retains the base and isolates
        // this trait's actual library loadings, without copying the catalogue.
        dasmig::trait_request isolated;
        isolated.count = 1;
        isolated.include = {trait.key};
        const auto single = dasmig::trtg::instance().get_personality(isolated, personality.seed());
        writer.begin_object().field("label", trait.label).key("effects").begin_array();
        for (const auto& axis : single.axes) writer.value(axis.value - axis.base);
        writer.end_array().end_object();
    }
    writer.end_array().end_object();
    auto json = personality.to_json();
    json.pop_back();
    return json + ",\"explanation\":" + writer.str() + "}";
}

std::string generate_traits(int count, const std::string& seed)
{
    auto& gen = dasmig::trtg::instance();
    if (!gen.has_data()) return error("Trait data is not loaded. Reload the page to try again.");
    if (count < 1 || static_cast<std::size_t>(count) > gen.trait_count())
        return error("Trait count must be between 1 and the catalogue size.");

    dasmig::trait_request request;
    request.count = static_cast<std::size_t>(count);
    if (seed.empty()) return explained(gen.get_personality(request));

    try { return explained(gen.get_personality(request, parse_demo_seed(seed))); }
    catch (const std::invalid_argument& exception) { return error(exception.what()); }
}

} // namespace

EMSCRIPTEN_BINDINGS(traitgen_demo)
{
    emscripten::function("init", &init);
    emscripten::function("generateTraits", &generate_traits);
}
