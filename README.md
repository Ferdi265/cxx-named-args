# `named-args`

A proof of concept implementation of named function arguments for C++17.

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

Most of the code in this library could also be used with lower C++ standard
versions:

- for C++14 support, `opt_arg` must be removed (or changed to use a
  C++14-compatible implementation of `std::optional`) and `auto` non-type
  template parameters need an additional template parameter for their type
  - the branch `cxx14` contains a version of this library with this change
- for C++11 support, remove `constexpr` from the `function` type's call operator
  and inline all variable templates
  - the branch `cxx11` contains a version of this library with this change

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

The `Arg` argument types are structures that inherit from `named_args::req_arg`,
`named_args::opt_arg`, or `named_args::def_arg<Type, Value>` and represent
required, optional, and defaulted arguments respectively.

This would be a possible implementation of the above function:

```cpp
#include <iostream>
#include <string>
#include <optional>
#include "named_args.h"

struct name_t : named_args::req_arg {};
struct age_t : named_args::opt_arg {};

constexpr named_args::marker<name_t> name;
constexpr named_args::marker<age_t> age;

void test_impl(std::string name, std::optional<int> age) {
    if (age) {
        std::cout << name << " is " << *age << " years old.\n";
    } else {
        std::cout << "Hello " << name << "!\n";
    }
}

constexpr named_args::function<test_impl, name_t, age_t> test{};
```

The file `test.cpp` contains a second slightly larger example that demonstrates
defaulted arguments.
