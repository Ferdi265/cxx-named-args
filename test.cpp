#include <iostream>
#include <string>
#include <functional>
#include "named_args.h"

using namespace std::literals;

// define argument types
struct name_t : public named_args::req_arg {};
struct age_t : public named_args::opt_arg  {};
struct bufsiz_t : public named_args::def_arg<size_t, 4096> {};
struct nice_t : public named_args::opt_arg {};

// create named argument markers
constexpr named_args::marker<name_t> name;
constexpr named_args::marker<age_t> age;
constexpr named_args::marker<bufsiz_t> bufsiz;
constexpr named_args::marker<nice_t> nice;

// implementation
void test_impl(std::string name, std::optional<int> age, size_t bufsiz, std::optional<std::reference_wrapper<int>> nice) {
    std::cout << "test:\n";

    std::cout << "- name is " << name << "\n";

    if (age) {
        std::cout << "- age is " << *age << "\n";
    } else {
        std::cout << "- no age given\n";
    }

    std::cout << "- bufsiz is " << bufsiz << "\n";

    if (nice) {
        nice->get() = 42;
    }
}

constexpr named_args::function<test_impl, name_t, age_t, bufsiz_t, nice_t> test{};

// tests
void foo(char * s, int a, int b, int& n) {
    test(name = s, age = a, bufsiz = b, nice = n);
}

int main() {
    int n;

    test(name = "foo", age = 42, bufsiz = 8192);
    test(bufsiz = 8192, name = "foo", age = 42);
    test(name = "bar", age = 1337);
    test(name = "baz"s, nice = n);

    std::cout << "nice! " << n << "\n";
}
