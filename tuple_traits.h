#pragma once

#include <cstddef>
#include <type_traits>
#include <tuple>

namespace tuple_traits {
    using std::tuple;

    // template alias to reduce the amount of "typename T::type" bloat
    template <typename T>
    using type_t = typename T::type;

    // template alias to reduce the amount of "decltype(...)" bloat
    template <typename T>
    using value_t = decltype(T::value);

    // append a type U to a tuple T
    template <typename T, typename U>
    struct append;

    template <typename T, typename U>
    using append_t = type_t<append<T, U>>;

    template <typename U, typename... Ts>
    struct append<tuple<Ts...>, U> {
        using type = tuple<Ts..., U>;
    };

    // prepend a type U to a tuple T
    template <typename T, typename U>
    struct prepend;

    template <typename T, typename U>
    using prepend_t = type_t<prepend<T, U>>;

    template <typename U, typename... Ts>
    struct prepend<tuple<Ts...>, U> {
        using type = tuple<U, Ts...>;
    };

    // check if a type U is contained in a tuple T
    template <typename T, typename U>
    struct contains;

    template <typename T, typename U>
    constexpr bool contains_v = contains<T, U>::value;

    template <typename U>
    struct contains<tuple<>, U> {
        constexpr static bool value = false;
    };

    template <typename U, typename... Ts>
    struct contains<tuple<U, Ts...>, U> {
        constexpr static bool value = true;
    };

    template <typename U, typename T, typename... Ts>
    struct contains<tuple<T, Ts...>, U> {
        constexpr static bool value = contains_v<tuple<Ts...>, U>;
    };

    // count the number of occurrences of a type U in a tuple T
    template <typename T, typename U>
    struct count;

    template <typename T, typename U>
    constexpr size_t count_v = count<T, U>::value;

    template <typename U>
    struct count<tuple<>, U> {
        constexpr static size_t value = 0;
    };

    template <typename U, typename... Ts>
    struct count<tuple<U, Ts...>, U> {
        constexpr static size_t value = 1 + count_v<tuple<Ts...>, U>;
    };

    template <typename U, typename T, typename... Ts>
    struct count<tuple<T, Ts...>, U> {
        constexpr static size_t value = count_v<tuple<Ts...>, U>;
    };

    // find the index of a type U in a tuple T
    template <typename T, typename U>
    struct index;

    template <typename T, typename U>
    constexpr size_t index_v = index<T, U>::value;

    template <typename U, typename... Ts>
    struct index<tuple<U, Ts...>, U> {
        constexpr static size_t value = 0;
    };

    template <typename U, typename T, typename... Ts>
    struct index<tuple<T, Ts...>, U> {
        constexpr static size_t value = 1 + index_v<tuple<Ts...>, U>;
    };

    // map a type mapper template M<U> over a tuple T
    template <typename T, template <typename U> class M>
    struct map_type;

    template <typename T, template <typename U> class M>
    using map_type_t = type_t<map_type<T, M>>;

    template <template <typename U> class M, typename... Ts>
    struct map_type<tuple<Ts...>, M> {
        using type = tuple<type_t<M<Ts>>...>;
    };

    // map a value mapper template M<U> over a tuple T
    template <typename T, template <typename U> class M>
    struct map_value;

    template <typename T, template <typename U> class M>
    constexpr value_t<map_value<T, M>> map_value_v = map_value<T, M>::value;

    template <template <typename U> class M, typename... Ts>
    struct map_value<tuple<Ts...>, M> {
        constexpr static tuple<value_t<M<Ts>>...> value = { M<Ts>::value... };
    };

    // template wrapper structs for use as a mapper
    template <typename T>
    struct ident {
        using type = T;
    };

    template <typename T>
    struct type_of {
        using type = type_t<T>;
    };

    template <typename T>
    struct value_of {
        constexpr static value_t<T> value = T::value;
    };

    // get the types of a tuple T of type wrappers
    template <typename T>
    struct types {
        using type = map_type_t<T, type_of>;
    };

    template <typename T>
    using types_t = type_t<types<T>>;

    // get the values of a tuple T of value wrappers
    template <typename T>
    struct values {
        constexpr static value_t<map_value<T, value_of>> value = map_value_v<T, value_of>;
    };

    template <typename T>
    constexpr value_t<values<T>> values_v = values<T>::value;
}
