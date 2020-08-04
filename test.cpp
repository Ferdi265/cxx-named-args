#include <iostream>
#include <string>
#include <functional>
#include "named_args.h"

// define argument types
struct name_t : named_args::req_arg {};
struct age_t : named_args::def_arg<int, -1>  {};
struct bufsiz_t : named_args::def_arg<size_t, 4096> {};
struct nice_t : named_args::def_arg<int*> {};

// create named argument markers
constexpr named_args::marker<name_t> name;
constexpr named_args::marker<age_t> age;
constexpr named_args::marker<bufsiz_t> bufsiz;
constexpr named_args::marker<nice_t> nice;

// implementation
void test_impl(std::string name, int age, size_t bufsiz, int* nice) {
    std::cout << "test:\n";

    std::cout << "- name is " << name << "\n";

    if (age != -1) {
        std::cout << "- age is " << age << "\n";
    } else {
        std::cout << "- no age given\n";
    }

    std::cout << "- bufsiz is " << bufsiz << "\n";

    if (nice) {
        *nice = 42;
    }
}

constexpr named_args::function<decltype(&test_impl), &test_impl, name_t, age_t, bufsiz_t, nice_t> test{};

// tests
void foo(char * s, int a, int b, int& n) {
    test(name = s, age = a, bufsiz = b, nice = &n);
}

int main() {
    int n;

    test(name = "foo", age = 42, bufsiz = 8192);
    test(bufsiz = 8192, name = "foo", age = 42);
    test(name = "bar", age = 1337);
    test(name = std::string("baz"), nice = &n);

    std::cout << "nice! " << n << "\n";
}
