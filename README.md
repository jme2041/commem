# commem: COM Memory Management

`commem.h` provides deleter classes for Microsoft Component Object Model (COM)
data types that can be used with `std::unique_ptr` and `std::shared_ptr`.

A C++ compiler that supports C++17 or newer is required. Specifically, support
for `std::unique_ptr`, `std::shared_ptr`, `std::remove_pointer_t`,
`std::is_pointer_v`, type aliases, and alias templates is needed.

# Deleter Classes and Smart Pointer Type Aliases

`commem.h` defines the `commem` namespace. The deleter classes and smart
pointer type aliases are all implemented within this namespace.

Detailed examples of common use cases are provided in the unit tests (see the
`test` directory).

## ComHeapDeleter

`ComHeapDeleter` is a class template that frees memory allocated by the COM
heap allocator, `CoTaskMemAlloc()`. It calls `CoTaskMemFree()`. The alias
templates `unique_heap<T>` and `shared_heap<T>` are provided for unique and
shared pointers, respectively, where `T` is a pointer type.

Examples:

```cpp
using namespace commem;

// Unique pointer
unique_heap<void*> x(CoTaskMemAlloc(10));

// Shared pointer
shared_heap<void*> y(CoTaskMemAlloc(10), ComHeapDeleter<void*>());
```

## BStringDeleter

`BStringDeleter` frees `BSTR`s by calling `SysFreeString()`. Type aliases
`unique_bstr` and `shared_bstr` are provided for unique and shared pointers,
respectively.

Examples:

```cpp
using namespace commem;

// Unique pointer
unique_bstr x(SysAllocString(L"ABCD"));

// Shared pointer
shared_bstr y(SysAllocString(L"ABCD"), BStringDeleter());
```

## SafeArrayDeleter

`SafeArrayDeleter` frees `SAFEARRAY`s by calling `SafeArrayDestroy()`. Type
aliases `unique_safearray` and `shared_safearray` are provided for unique and
shared pointers, respectively.

If `SafeArrayDestroy()` fails, such as when the `SAFEARRAY` is locked,
`std::terminate()` is called. The rationale for program termination is to allow
for early detection of errors related to managing the `SAFEARRAY` lifecycle.

Examples:

```cpp
using namespace commem;

// Unique pointer
unique_safearray x(SafeArrayCreateVector(VT_I4, 0, 10));

// Shared pointer
shared_safearray y(SafeArrayCreateVector(VT_I4, 0, 10), SafeArrayDeleter());
```

# Using commem

Download `commem.h` and include it in source files that use `commem` classes.

This repository can also be included as a submodule in Git projects. It is not
necessary to initialize the GoogleTest submodule and run the `commem` tests or
to include `commem` using CMake's `add_subdirectory()`. Rather, add
`commem\include` to the path used to search for C++ header files.

# Testing commem

`commem` is developed and tested on Windows 11 (x86-64) using Visual Studio and
GoogleTest. The tests are compiled using the MSVC, MinGW-64, and Clang
compilers. To run the tests, CMake and a C++ compiler are required. The
compilers currently used for testing are the MSVC compiler bundled with Visual
Studio 2022 (`CL.exe` 19.40 or newer), `g++` (Rev 5, built by the MSYS2
project, version 13.2.0), and `clang-cl.exe` that is bundled with Visual Studio
(version 17.0.3) using the `x86_64-pc-windows-msvc` target.

Here is an example workflow for building and running the tests using a Visual
Studio developer command prompt and the MSVC compiler. For a release build,
replace `Debug` with `Release`.

```cmd
git clone --recurse-submodules git@github.com:jme2041/commem.git
cd commem
mkdir build
cd build
cmake -DCOMMEM_TESTS=ON ..
cmake --build . --config Debug
Debug\test_commem.exe
```

# Other Tools

The rationale for `commem` arose when switching from the Microsoft Active
Template Library (ATL) COM tools to the Windows Runtime C++ Library (WRL)
tools. I recommend using `Microsoft::WRL::ComPtr` for new projects that need a
COM interface smart pointer (include `wrl/client.h`; if only the client file is
included, no WRL libraries will be linked so `Microsoft::WRL::ComPtr` will
remain a lightweight and modern COM smart pointer).

WRL does not include smart pointer types for `BSTR` and `SAFEARRAY` data types,
which are commonly used in automation-compatible COM interfaces, or for
pointers to data allocated using `CoTaskMemAlloc()`. `commem` was designed to
address this gap with as little new code as possible; hence reliance on the
well-defined and widely-used `std::unique_ptr` and `std::shared_ptr` smart
pointers. The unit tests included with `commem` verify that the unique and
shared pointers to these three COM types work as expected. The tests also
include examples of how to use the smart pointers with Windows APIs for copying
or manipulating `BSTR`s and `SAFEARRAY`s.

`commem` does not provide a smart pointer to the `VARIANT` data type.
`VARIANT`s are typically allocated on the stack and patterns of use differ from
pointer types like `BSTR` and `SAFEARRAY`. It is, however, common to store
`BSTR` and/or `SAFEARRAY` data in a `VARIANT`. Examples of how to interact with
`VARIANT`s are included in the unit tests for `unique_bstr` (see class
`TestBStringUnique` ) and `unique_safearray` (see class `TestSafeArrayUnique`).

An alternative to `commem` is to use ATL. Certain limitations of ATL led to the
development of `commem`. First, `CComVariant`, the ATL wrapper for the
`VARIANT` data type, always copies a `SAFEARRAY` when a `SAFEARRAY` is assigned
to it. This results in significant overhead for large arrays that don't
otherwise need to be copied. Second, `CComSafeArray`, the ATL wrapper for
`SAFEARRAY`s, requires that the data type stored in the `SAFEARRAY` is known at
compile time.

# License

Copyright 2024 Jeffrey M. Engelmann

`commem` is released under the MIT License. For details, see
[LICENSE.txt](LICENSE.txt).
