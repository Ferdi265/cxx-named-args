# `named-args`

A proof of concept implementation of named function arguments for C++11.

## Features

- Simple specification of named arguments without needing to write too much
  boilerplate argument handling code.
- Compile time error messages for duplicate arguments, missing required
  arguments, or invalid arguments. (not very specialized error messages at the
  moment)
- Uses forwarding to allow passing any type convertible to the argument type,
  while preserving value category.
- Single header implementation

## Limitations

- Positional arguments are not (yet) supported. All arguments must be named.
- Failed conversions (e.g. wrong argument type) will result in overload
  resolution error messages from inside the implementation.

## Required Standard Version

The original version of this library was written for C++17.

## How to use

A function with named arguments can be called by calling the named argument
function object and "assigning" values to the marker objects for each argument.

For example:

```cpp
test(name = "peter", age = 23);
```

Here, `name` and `age` are marker objects of type `named_args::marker<Arg>`,
which returned a named argument when assigned to. `test` is a named function
object of type `named_args::function<Implementation, Args...>`.

The `Arg` argument types are structures that inherit from `named_args::req_arg`
or `named_args::def_arg<Type, Value>` and represent required and defaulted
arguments respectively.

This would be a possible implementation of the above function:

```cpp
#include <iostream>
#include <string>
#include "named_args.h"

struct name_t : named_args::req_arg {};
struct age_t : named_args::def_arg<int, -1> {};

constexpr named_args::marker<name_t> name;
constexpr named_args::marker<age_t> age;

void test_impl(std::string name, int age) {
    if (age != -1) {
        std::cout << name << " is " << age << " years old.\n";
    } else {
        std::cout << "Hello " << name << "!\n";
    }
}

constexpr named_args::function<decltype(&test_impl), &test_impl, name_t, age_t> test{};
```

The file `test.cpp` contains a second slightly larger example.
