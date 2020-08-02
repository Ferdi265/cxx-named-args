#pragma once

#include <type_traits>
#include <utility>
#include <tuple>
#include <optional>

namespace named_args {
    // named argument types
    template <typename T>
    struct req_arg_t {
        constexpr static bool required = true;
        using type = T;
        type value;
    };

    template <typename T>
    struct opt_arg_t {
        constexpr static bool required = false;
        using type = std::optional<T>;
        type value;
    };

    template <typename T, auto _default>
    struct def_arg_t {
        constexpr static bool required = false;
        using type = T;
        type value = _default;
    };

    // named argument marker type
    template <typename N>
    struct arg_t {
        using type = typename N::type;

        template <typename T>
        N operator=(T&& t) const {
            return {{type(std::forward<T>(t))}};
        }
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

        // check for missing required arguments
        template <typename N, typename M>
        struct missing_req_args;

        template <typename N, typename M>
        using missing_req_args_t = typename missing_req_args<N, M>::type;

        template <typename M>
        struct missing_req_args<std::tuple<>, M> {
            using type = std::tuple<>;
            constexpr static bool empty = true;
        };

        template <typename M, typename N, typename... Ns>
        struct missing_req_args<std::tuple<N, Ns...>, M> {
            using type = std::conditional_t<
                !N::required || tuple_contains_v<M, N>,
                missing_req_args_t<std::tuple<Ns...>, M>,
                tuple_prepend_t<missing_req_args_t<std::tuple<Ns...>, M>, N>
            >;
            constexpr static bool empty = std::is_same_v<type, std::tuple<>>;
        };

        // check for missing non-required arguments
        template <typename N, typename M>
        struct missing_non_req_args;

        template <typename N, typename M>
        using missing_non_req_args_t = typename missing_non_req_args<N, M>::type;

        template <typename M>
        struct missing_non_req_args<std::tuple<>, M> {
            using type = std::tuple<>;
            constexpr static bool empty = true;
        };

        template <typename M, typename N, typename... Ns>
        struct missing_non_req_args<std::tuple<N, Ns...>, M> {
            using type = std::conditional_t<
                N::required || tuple_contains_v<M, N>,
                missing_non_req_args_t<std::tuple<Ns...>, M>,
                tuple_prepend_t<missing_non_req_args_t<std::tuple<Ns...>, M>, N>
            >;
            constexpr static bool empty = std::is_same_v<type, std::tuple<>>;
        };

        // check for duplicate arguments
        template <typename N, typename M>
        struct duplicate_args;

        template <typename N, typename M>
        using duplicate_args_t = typename duplicate_args<N, M>::type;

        template <typename M>
        struct duplicate_args<std::tuple<>, M> {
            using type = std::tuple<>;
            constexpr static bool empty = true;
        };

        template <typename M, typename N, typename... Ns>
        struct duplicate_args<std::tuple<N, Ns...>, M> {
            using type = std::conditional_t<
                tuple_count_v<M, N> <= 1,
                duplicate_args_t<std::tuple<Ns...>, M>,
                tuple_prepend_t<duplicate_args_t<std::tuple<Ns...>, M>, N>
            >;
            constexpr static bool empty = std::is_same_v<type, std::tuple<>>;
        };

        // check for invalid arguments
        template <typename N, typename M>
        struct invalid_args;

        template <typename N, typename M>
        using invalid_args_t = typename invalid_args<N, M>::type;

        template <typename N>
        struct invalid_args<N, std::tuple<>> {
            using type = std::tuple<>;
            constexpr static bool empty = true;
        };

        template <typename N, typename M, typename... Ms>
        struct invalid_args<N, std::tuple<M, Ms...>> {
            using type = std::conditional_t<
                tuple_contains_v<N, M>,
                invalid_args_t<N, std::tuple<Ms...>>,
                tuple_prepend_t<invalid_args_t<N, std::tuple<Ms...>>, M>
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
        template <typename N, typename M>
        struct check_args {
            using missing_req_args = typename detail::missing_req_args_t<N, M>;
            using duplicate_args = typename detail::duplicate_args_t<N, M>;
            using invalid_args = typename detail::invalid_args_t<N, M>;
            constexpr static bool valid =
                detail::missing_req_args<N, M>::empty &&
                detail::duplicate_args<N, M>::empty &&
                detail::invalid_args<N, M>::empty;
            constexpr static named_args::error<missing_req_args, duplicate_args, invalid_args, valid> error{};
        };
    }

    template <auto impl, typename... Ns>
    struct function {
    private:
        using R = decltype(impl(std::declval<typename Ns::type>()...));

        template <typename N, typename Rest, typename... Args>
        static constexpr N&& construct_or_default(std::tuple<Args&&...>& args, Rest& rest) {
            if constexpr (detail::tuple_contains_v<std::tuple<Args...>, N>) {
                return std::move(std::get<N&&>(args));
            } else {
                return std::move(std::get<N>(rest));
            }
        }

    public:
        template <typename... Args>
        constexpr R operator()(Args&&... a) const {
            [[maybe_unused]] detail::check_args<std::tuple<Ns...>, std::tuple<std::remove_reference_t<Args>...>> check;

            std::tuple<Args&&...> args = std::forward_as_tuple(std::forward<Args>(a)...);
            detail::missing_non_req_args_t<std::tuple<Ns...>, std::tuple<std::remove_reference_t<Args>...>> rest;
            return impl(construct_or_default<Ns>(args, rest).value...);
        }
    };
}
