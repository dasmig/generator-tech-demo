// Polyfills for C++23 features missing from Emscripten's libc++.
// Include BEFORE any library header that uses these features.
#pragma once

#include <algorithm>
#include <optional>
#include <ranges>
#include <tuple>
#include <cstddef>

// ── std::ranges::fold_left_first ────────────────────────────────
#if !defined(__cpp_lib_ranges_fold)
namespace std { namespace ranges {
template<typename R, typename F>
auto fold_left_first(R&& r, F&& f)
    -> std::optional<range_value_t<std::remove_cvref_t<R>>>
{
    auto it  = std::ranges::begin(r);
    auto end = std::ranges::end(r);
    using V  = range_value_t<std::remove_cvref_t<R>>;
    if (it == end) return std::optional<V>{};
    V acc = *it; ++it;
    for (; it != end; ++it) acc = f(std::move(acc), *it);
    return std::optional<V>{std::move(acc)};
}
}} // namespace std::ranges
#endif

// ── std::views::enumerate ───────────────────────────────────────
#if !defined(__cpp_lib_ranges_enumerate)
namespace std::ranges {

template<input_range V>
    requires view<V>
class enumerate_view : public view_interface<enumerate_view<V>> {
    V base_ = V();
public:
    class iterator {
    public:
        iterator_t<V> current_ = {};
        std::size_t   pos_     = 0;

        using difference_type = std::ptrdiff_t;
        using value_type      = std::tuple<std::size_t, range_value_t<V>>;

        iterator() = default;
        constexpr iterator(iterator_t<V> c, std::size_t p)
            : current_(std::move(c)), pos_(p) {}

        constexpr auto operator*() const {
            return std::tuple<std::size_t, range_reference_t<V>>{pos_, *current_};
        }
        constexpr iterator& operator++() { ++pos_; ++current_; return *this; }
        constexpr void      operator++(int) { ++*this; }

        friend constexpr bool operator==(const iterator& a,
                                         const iterator& b) {
            return a.current_ == b.current_;
        }
    };

    class sentinel {
    public:
        sentinel_t<V> end_ = {};
        sentinel() = default;
        constexpr explicit sentinel(sentinel_t<V> e) : end_(std::move(e)) {}
        friend constexpr bool operator==(const iterator& it,
                                         const sentinel& s) {
            return it.current_ == s.end_;
        }
    };

    enumerate_view() = default;
    constexpr explicit enumerate_view(V base) : base_(std::move(base)) {}

    constexpr auto begin() {
        return iterator(std::ranges::begin(base_), 0);
    }
    constexpr auto end() {
        if constexpr (common_range<V>)
            return iterator(std::ranges::end(base_), 0);
        else
            return sentinel(std::ranges::end(base_));
    }
};

template<viewable_range R>
enumerate_view(R&&) -> enumerate_view<views::all_t<R>>;

} // namespace std::ranges

namespace std::ranges::views {
struct _enumerate_adaptor {
    template<std::ranges::viewable_range R>
    constexpr auto operator()(R&& r) const {
        return std::ranges::enumerate_view(std::views::all(std::forward<R>(r)));
    }
    template<std::ranges::viewable_range R>
    friend constexpr auto operator|(R&& r, _enumerate_adaptor) {
        return std::ranges::enumerate_view(std::views::all(std::forward<R>(r)));
    }
};
inline constexpr _enumerate_adaptor enumerate{};
} // namespace std::ranges::views
#endif
