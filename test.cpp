#include <iostream>
#include <string>
#include "named_args.h"

using namespace std::literals;

// define arguments
struct name_t : named_args::req_arg_t<std::string> {};
struct age_t : named_args::opt_arg_t<int> {};
struct bufsiz_t : named_args::def_arg_t<size_t, 4096> {};

// create named argument markers
constexpr named_args::arg_t<name_t> name;
constexpr named_args::arg_t<age_t> age;
constexpr named_args::arg_t<bufsiz_t> bufsiz;

// named argument wrapper
void test_impl(std::string name, std::optional<int> age, size_t bufsiz);
template <typename... Args>
void test(Args... a) {
    named_args::storage<name_t, age_t, bufsiz_t> args(a...);

    using named_args::get_arg;
    test_impl(get_arg<name_t>(args), get_arg<age_t>(args), get_arg<bufsiz_t>(args));
}

// implementation
void test_impl(std::string name, std::optional<int> age, size_t bufsiz) {
    std::cout << "test:\n";

    std::cout << "- name is " << name << "\n";

    if (age) {
        std::cout << "- age is " << *age << "\n";
    } else {
        std::cout << "- no age given\n";
    }

    std::cout << "- bufsiz is " << bufsiz << "\n";
}

// test
int main() {
    test(name = "foo", age = 42, bufsiz = 8192);
    test(bufsiz = 8192, name = "foo", age = 42);
    test(name = "bar", age = 1337);
    test(name = "baz"s);
}
