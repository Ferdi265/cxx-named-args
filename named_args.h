#pragma once

#include <type_traits>
#include <utility>
#include <tuple>
#include <optional>

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
        // append a type to a tuple
        template <typename T, typename U>
        struct tuple_append;

        template <typename T, typename U>
        using tuple_append_t = typename tuple_append<T, U>::type;

        template <typename U, typename... Ts>
        struct tuple_append<std::tuple<Ts...>, U> {
            using type = std::tuple<Ts..., U>;
        };

        // prepend a type to a tuple
        template <typename T, typename U>
        struct tuple_prepend;

        template <typename T, typename U>
        using tuple_prepend_t = typename tuple_prepend<T, U>::type;

        template <typename U, typename... Ts>
        struct tuple_prepend<std::tuple<Ts...>, U> {
            using type = std::tuple<U, Ts...>;
        };

        // check if a type is contained in a tuple
        template <typename T, typename U>
        struct tuple_contains;

        template <typename T, typename U>
        constexpr bool tuple_contains_v = tuple_contains<T, U>::value;

        template <typename U>
        struct tuple_contains<std::tuple<>, U> {
            constexpr static bool value = false;
        };

        template <typename U, typename... Ts>
        struct tuple_contains<std::tuple<U, Ts...>, U> {
            constexpr static bool value = true;
        };

        template <typename U, typename T, typename... Ts>
        struct tuple_contains<std::tuple<T, Ts...>, U> {
            constexpr static bool value = tuple_contains_v<std::tuple<Ts...>, U>;
        };

        // count the number of occurrences of a type in a tuple
        template <typename T, typename U>
        struct tuple_count;

        template <typename T, typename U>
        constexpr size_t tuple_count_v = tuple_count<T, U>::value;

        template <typename U>
        struct tuple_count<std::tuple<>, U> {
            constexpr static size_t value = 0;
        };

        template <typename U, typename... Ts>
        struct tuple_count<std::tuple<U, Ts...>, U> {
            constexpr static size_t value = 1 + tuple_count_v<std::tuple<Ts...>, U>;
        };

        template <typename U, typename T, typename... Ts>
        struct tuple_count<std::tuple<T, Ts...>, U> {
            constexpr static size_t value = tuple_count_v<std::tuple<Ts...>, U>;
        };

        // find the index of a type in a tuple
        template <typename T, typename U>
        struct tuple_index;

        template <typename T, typename U>
        constexpr size_t tuple_index_v = tuple_index<T, U>::value;

        template <typename U, typename... Ts>
        struct tuple_index<std::tuple<U, Ts...>, U> {
            constexpr static size_t value = 0;
        };

        template <typename U, typename T, typename... Ts>
        struct tuple_index<std::tuple<T, Ts...>, U> {
            constexpr static size_t value = 1 + tuple_index_v<std::tuple<Ts...>, U>;
        };

        // transform a tuple of kinds into their default value types
        template <typename K>
        struct kind_types;

        template <typename K>
        using kind_types_t = typename kind_types<K>::type;

        template <typename... Ks>
        struct kind_types<std::tuple<Ks...>> {
            using type = std::tuple<arg<typename Ks::type, Ks>...>;
        };

        // transform a tuple of kinds into their default values
        template <typename K>
        struct kind_values;

        template <typename K>
        constexpr kind_types_t<K> kind_values_v = kind_values<K>::value;

        template <typename... Ks>
        struct kind_values<std::tuple<Ks...>> {
            static constexpr kind_types_t<std::tuple<Ks...>> value = {{Ks::value}...};
        };

        // get the kind of an argument
        template <typename A>
        struct arg_kind {
            using type = void;
        };

        template <typename A>
        using arg_kind_t = typename arg_kind<A>::type;

        template <typename T, typename K>
        struct arg_kind<arg<T, K>> {
            using type = K;
        };

        // transform a tuple of arguments into a tuple of kinds
        template <typename A>
        struct arg_kinds;

        template <typename A>
        using arg_kinds_t = typename arg_kinds<A>::type;

        template <typename... As>
        struct arg_kinds<std::tuple<As...>> {
            using type = std::tuple<arg_kind_t<As>...>;
        };

        // check for missing required arguments
        template <typename K, typename A>
        struct missing_req_args;

        template <typename K, typename A>
        using missing_req_args_t = typename missing_req_args<K, A>::type;

        template <typename A>
        struct missing_req_args<std::tuple<>, A> {
            using type = std::tuple<>;
            constexpr static bool empty = true;
        };

        template <typename A, typename K, typename... Ks>
        struct missing_req_args<std::tuple<K, Ks...>, A> {
            using type = std::conditional_t<
                !K::required || tuple_contains_v<arg_kinds_t<A>, K>,
                missing_req_args_t<std::tuple<Ks...>, A>,
                tuple_prepend_t<missing_req_args_t<std::tuple<Ks...>, A>, K>
            >;
            constexpr static bool empty = std::is_same_v<type, std::tuple<>>;
        };

        // check for missing non-required arguments
        template <typename K, typename A>
        struct missing_non_req_args;

        template <typename K, typename A>
        using missing_non_req_args_t = typename missing_non_req_args<K, A>::type;

        template <typename A>
        struct missing_non_req_args<std::tuple<>, A> {
            using type = std::tuple<>;
            constexpr static bool empty = true;
        };

        template <typename A, typename K, typename... Ks>
        struct missing_non_req_args<std::tuple<K, Ks...>, A> {
            using type = std::conditional_t<
                K::required || tuple_contains_v<arg_kinds_t<A>, K>,
                missing_non_req_args_t<std::tuple<Ks...>, A>,
                tuple_prepend_t<missing_non_req_args_t<std::tuple<Ks...>, A>, K>
            >;
            constexpr static bool empty = std::is_same_v<type, std::tuple<>>;
        };

        // check for duplicate arguments
        template <typename K, typename A>
        struct duplicate_args;

        template <typename K, typename A>
        using duplicate_args_t = typename duplicate_args<K, A>::type;

        template <typename A>
        struct duplicate_args<std::tuple<>, A> {
            using type = std::tuple<>;
            constexpr static bool empty = true;
        };

        template <typename A, typename K, typename... Ks>
        struct duplicate_args<std::tuple<K, Ks...>, A> {
            using type = std::conditional_t<
                tuple_count_v<arg_kinds_t<A>, K> <= 1,
                duplicate_args_t<std::tuple<Ks...>, A>,
                tuple_prepend_t<duplicate_args_t<std::tuple<Ks...>, A>, K>
            >;
            constexpr static bool empty = std::is_same_v<type, std::tuple<>>;
        };

        // check for invalid arguments
        template <typename K, typename A>
        struct invalid_args;

        template <typename K, typename A>
        using invalid_args_t = typename invalid_args<K, A>::type;

        template <typename K>
        struct invalid_args<K, std::tuple<>> {
            using type = std::tuple<>;
            constexpr static bool empty = true;
        };

        template <typename K, typename A, typename... As>
        struct invalid_args<K, std::tuple<A, As...>> {
            using type = std::conditional_t<
                tuple_contains_v<K, arg_kind_t<A>>,
                invalid_args_t<K, std::tuple<As...>>,
                tuple_prepend_t<invalid_args_t<K, std::tuple<As...>>, A>
            >;
            constexpr static bool empty = std::is_same_v<type, std::tuple<>>;
        };
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

            if constexpr (detail::tuple_contains_v<arg_kinds, K>) {
                return std::move(std::get<detail::tuple_index_v<arg_kinds, K>>(args));
            } else {
                return std::move(std::get<detail::tuple_index_v<rest_kinds, K>>(rest));
            }
        }

    public:
        template <typename... Args>
        constexpr decltype(auto) operator()(Args&&... a) const {
            [[maybe_unused]] detail::check_args<std::tuple<Ks...>, std::tuple<std::remove_reference_t<Args>...>> check;
            using rest_args = detail::missing_non_req_args_t<std::tuple<Ks...>, std::tuple<std::remove_reference_t<Args>...>>;

            std::tuple<Args&&...> args = std::forward_as_tuple(std::forward<Args>(a)...);
            return impl(select<Ks>(args, detail::kind_values_v<rest_args>).value...);
        }
    };
}
