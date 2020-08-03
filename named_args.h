#pragma once

#include <cstddef>
#include <type_traits>
#include <tuple>
#include <utility>
#include <optional>
#include "tuple_traits.h"

namespace named_args {
    // named argument value type
    template <typename T, typename K>
    struct arg {
        using type = T;
        using kind = K;
        type value;
    };

    // named argument marker type
    template <typename K>
    struct marker {
        template <typename T>
        arg<decltype(std::forward<T>(std::declval<T&&>())), K> operator=(T&& t) const {
            return {std::forward<T>(t)};
        }
    };

    // named argument kinds
    struct req_arg {
        constexpr static bool required = true;
    };

    struct opt_arg {
        constexpr static bool required = false;
        using type = std::nullopt_t;
        constexpr static type value = std::nullopt;
    };

    template <typename T, T _default>
    struct def_arg {
        constexpr static bool required = false;
        using type = T;
        constexpr static type value = _default;
    };

    namespace detail {
        using std::tuple;
        using tuple_traits::type_t;
        using tuple_traits::value_t;

        // template meta-programs specific to named_args

        // get the kind of an argument
        template <typename A>
        struct arg_kind {
            using type = void;
        };

        template <typename T, typename K>
        struct arg_kind<arg<T, K>> {
            using type = K;
        };

        template <typename A>
        using arg_kind_t = type_t<arg_kind<A>>;

        // transform a tuple of arguments into a tuple of kinds
        template <typename A>
        struct arg_kinds {
            using type = tuple_traits::map_type_t<A, arg_kind>;
        };

        template <typename A>
        using arg_kinds_t = type_t<arg_kinds<A>>;

        // check for missing required arguments
        template <typename K, typename A>
        struct missing_req_args;

        template <typename K, typename A>
        using missing_req_args_t = type_t<missing_req_args<K, A>>;

        template <typename A>
        struct missing_req_args<tuple<>, A> {
            using type = tuple<>;
            constexpr static bool empty = true;
        };

        template <typename A, typename K, typename... Ks>
        struct missing_req_args<tuple<K, Ks...>, A> {
            using type = std::conditional_t<
                !K::required || tuple_traits::contains_v<arg_kinds_t<A>, K>,
                missing_req_args_t<tuple<Ks...>, A>,
                tuple_traits::prepend_t<missing_req_args_t<tuple<Ks...>, A>, K>
            >;
            constexpr static bool empty = std::is_same_v<type, tuple<>>;
        };

        // check for missing non-required arguments
        template <typename K, typename A>
        struct missing_non_req_args;

        template <typename K, typename A>
        using missing_non_req_args_t = typename missing_non_req_args<K, A>::type;

        template <typename A>
        struct missing_non_req_args<tuple<>, A> {
            using type = tuple<>;
            constexpr static bool empty = true;
        };

        template <typename A, typename K, typename... Ks>
        struct missing_non_req_args<tuple<K, Ks...>, A> {
            using type = std::conditional_t<
                K::required || tuple_traits::contains_v<arg_kinds_t<A>, K>,
                missing_non_req_args_t<tuple<Ks...>, A>,
                tuple_traits::prepend_t<missing_non_req_args_t<tuple<Ks...>, A>, K>
            >;
            constexpr static bool empty = std::is_same_v<type, tuple<>>;
        };

        // check for duplicate arguments
        template <typename K, typename A>
        struct duplicate_args;

        template <typename K, typename A>
        using duplicate_args_t = typename duplicate_args<K, A>::type;

        template <typename A>
        struct duplicate_args<tuple<>, A> {
            using type = tuple<>;
            constexpr static bool empty = true;
        };

        template <typename A, typename K, typename... Ks>
        struct duplicate_args<tuple<K, Ks...>, A> {
            using type = std::conditional_t<
                tuple_traits::count_v<arg_kinds_t<A>, K> <= 1,
                duplicate_args_t<tuple<Ks...>, A>,
                tuple_traits::prepend_t<duplicate_args_t<tuple<Ks...>, A>, K>
            >;
            constexpr static bool empty = std::is_same_v<type, tuple<>>;
        };

        // check for invalid arguments
        template <typename K, typename A>
        struct invalid_args;

        template <typename K, typename A>
        using invalid_args_t = typename invalid_args<K, A>::type;

        template <typename K>
        struct invalid_args<K, tuple<>> {
            using type = tuple<>;
            constexpr static bool empty = true;
        };

        template <typename K, typename A, typename... As>
        struct invalid_args<K, tuple<A, As...>> {
            using type = std::conditional_t<
                tuple_traits::contains_v<K, arg_kind_t<A>>,
                invalid_args_t<K, tuple<As...>>,
                tuple_traits::prepend_t<invalid_args_t<K, tuple<As...>>, A>
            >;
            constexpr static bool empty = std::is_same_v<type, tuple<>>;
        };

        // TODO: select rework
    }

    // error reporting type
    template <typename missing_req_args, typename duplicate_args, typename invalid_args, bool valid = false>
    struct error {
        static_assert(valid, "named_args::error<missing_req_args, duplicate_args, invalid_args>: missing, duplicate, or invalid arguments");
    };

    namespace detail {
        // instantiation of error checks
        template <typename K, typename A>
        struct check_args {
            using missing_req_args = detail::missing_req_args_t<K, A>;
            using duplicate_args = detail::duplicate_args_t<K, A>;
            using invalid_args = detail::invalid_args_t<K, A>;
            constexpr static bool valid =
                detail::missing_req_args<K, A>::empty &&
                detail::duplicate_args<K, A>::empty &&
                detail::invalid_args<K, A>::empty;
            constexpr static named_args::error<missing_req_args, duplicate_args, invalid_args, valid> error{};
        };
    }

    template <auto impl, typename... Ks>
    struct function {
    private:
        template <typename K, typename Rest, typename... Args>
        static constexpr decltype(auto) select(std::tuple<Args&&...>& args, Rest& rest) {
            using arg_kinds = detail::arg_kinds_t<std::tuple<Args...>>;
            using rest_kinds = detail::arg_kinds_t<std::remove_const_t<Rest>>;

            if constexpr (tuple_traits::contains_v<arg_kinds, K>) {
                return std::get<tuple_traits::index_v<arg_kinds, K>>(args);
            } else {
                return std::get<tuple_traits::index_v<rest_kinds, K>>(rest);
            }
        }

    public:
        template <typename... Args>
        constexpr decltype(auto) operator()(Args&&... a) const {
            [[maybe_unused]] detail::check_args<std::tuple<Ks...>, std::tuple<std::remove_reference_t<Args>...>> check;
            using rest_args = detail::missing_non_req_args_t<std::tuple<Ks...>, std::tuple<std::remove_reference_t<Args>...>>;

            std::tuple<Args&&...> args = std::forward_as_tuple(std::forward<Args>(a)...);
            detail::kind_types_t<rest_args> rest = detail::kind_values_v<rest_args>;
            return impl(std::forward<decltype(select<Ks>(args, rest).value)>(select<Ks>(args, rest).value)...);
        }
    };
}
