#include <iostream>
#include "named_args.h"

// define arguments
struct foo_t : named_args::req_arg_t<bool> {};
struct age_t : named_args::opt_arg_t<int> {};
struct bufsiz_t : named_args::def_arg_t<int, 4096> {};

// create named argument markers
constexpr named_args::arg_t<foo_t> foo;
constexpr named_args::arg_t<age_t> age;
constexpr named_args::arg_t<bufsiz_t> bufsiz;

// named argument wrapper
void test_impl(bool foo, std::optional<int> age, int bufsiz);
template <typename... Args>
void test(Args... a) {
    named_args::storage<foo_t, age_t, bufsiz_t> args(a...);

    using named_args::get_arg;
    test_impl(get_arg<foo_t>(args), get_arg<age_t>(args), get_arg<bufsiz_t>(args));
}

// implementation
void test_impl(bool foo, std::optional<int> age, int bufsiz) {
    std::cout << "test:\n";

    if (foo) {
        std::cout << "- foo enabled\n";
    } else {
        std::cout << "- foo disabled\n";
    }

    if (age) {
        std::cout << "- age is " << *age << "\n";
    } else {
        std::cout << "- no age given\n";
    }

    std::cout << "- bufsiz is " << bufsiz << "\n";
}

// test
int main() {
    test(foo = true, age = 42, bufsiz = 8192);
    test(foo = false, age = 1337);
    test(foo = true);
}
