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

        // get the value of the arg with kind K from args A or rest R
        template <typename K, typename A, typename R,
            bool = tuple_traits::contains_v<arg_kinds_t<A>, K>
        >
        struct select_single;

        template <typename K, typename A, typename R>
        using select_single_t = type_t<select_single<K, A, R>>;

        template <typename K, typename A, typename R>
        struct select_single<K, A, R, true> {
            using arg_kinds = arg_kinds_t<A>;
            constexpr static size_t arg_index = tuple_traits::index_v<arg_kinds, K>;

            using type = tuple_traits::nth_t<A, arg_index>;

            using rest_values = tuple_traits::value_types_t<R>;
            constexpr static type select(A& a, [[maybe_unused]] rest_values& r) {
                return std::move(std::get<arg_index>(a));
            }
        };

        template <typename K, typename A, typename R>
        struct select_single<K, A, R, false> {
            constexpr static size_t arg_index = tuple_traits::index_v<R, K>;

            using type = arg<value_t<K>, K>;

            using rest_values = tuple_traits::value_types_t<R>;
            constexpr static type select([[maybe_unused]] A& a, rest_values& r) {
                return {std::get<arg_index>(r)};
            }
        };

        // get the type the named argument function implementation returns
        template <auto impl, typename K, typename A>
        struct impl_return;

        template <auto impl, typename K, typename A>
        using impl_return_t = type_t<impl_return<impl, K, A>>;

        template <auto impl, typename A, typename... Ks>
        struct impl_return<impl, tuple<Ks...>, A> {
            using R = missing_non_req_args_t<tuple<Ks...>, A>;

            using type = decltype(impl(std::declval<value_t<select_single_t<Ks, A, R>>&&>()...));
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

        // check args and then call impl_return
        template <auto impl, typename K, typename A, bool = check_args<K, A>::valid>
        struct impl_return_check;

        template <auto impl, typename K, typename A>
        using impl_return_check_t = type_t<impl_return_check<impl, K, A>>;

        template <auto impl, typename K, typename A>
        struct impl_return_check<impl, K, A, true> {
            using type = impl_return_t<impl, K, A>;
        };

        template <auto impl, typename K, typename A>
        struct impl_return_check<impl, K, A, false> {};
    }

    // named argument function type
    template <auto impl, typename... Ks>
    struct function {
    private:
        using kinds_t = std::tuple<Ks...>;

    public:
        template <typename... Args>
        [[gnu::always_inline]]
        constexpr detail::impl_return_check_t<impl, kinds_t, std::tuple<Args...>> operator()(Args&&... a) const {
            using args_t = std::tuple<Args...>;
            using rest_kinds_t = detail::missing_non_req_args_t<kinds_t, args_t>;
            using rest_values_t = tuple_traits::value_types_t<rest_kinds_t>;

            args_t args = std::forward_as_tuple(std::forward<Args>(a)...);
            rest_values_t rest = tuple_traits::values_v<rest_kinds_t>;

            return impl(std::forward<detail::value_t<detail::select_single_t<Ks, args_t, rest_kinds_t>>>(detail::select_single<Ks, args_t, rest_kinds_t>::select(args, rest).value)...);
        }
    };
}
