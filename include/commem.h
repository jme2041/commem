// commem.h ///////////////////////////////////////////////////////////////////
//
// commem: COM Memory Management
//
// commem is released under the MIT license.
//
// Copyright 2024 Jeffrey M. Engelmann
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//

#pragma once
#ifndef COMMEM_H
#define COMMEM_H

#include <Windows.h>
#include <memory>
#include <exception>

namespace commem {

    // Deleter that calls CoTaskMemFree
    // This deleter can be used with std::unique_ptr and std::shared_ptr
    // Examples:
    // std::unique_ptr<std::remove_pointer_t<void*>, ComHeapDeleter<void*>> x(CoTaskMemAlloc(10));
    // std::shared_ptr<std::remove_pointer_t<void*>> y(CoTaskMemAlloc(10), ComHeapDeleter<void*>());

    template<typename T>
    struct ComHeapDeleter {
        static_assert(std::is_pointer_v<T>);
        typedef T pointer;
        void operator()(pointer const p) noexcept
        {
            CoTaskMemFree(p);
        }
    };

    // Alias templates for unique and shared pointers

    template <typename T>
    using unique_heap = std::unique_ptr<std::remove_pointer_t<T>, ComHeapDeleter<T>>;

    template <typename T>
    using shared_heap = std::shared_ptr<std::remove_pointer_t<T>>;

    // Deleter that calls SysFreeString
    // This deleter can be used with std::unique_ptr and std::shared_ptr
    // Examples:
    // std::unique_ptr<std::remove_pointer_t<BSTR>, BStringDeleter> x(SysAllocString(L"ABCD"));
    // std::shared_ptr<std::remove_pointer_t<BSTR>> y(SysAllocString(L"ABCD"), BStringDeleter());

    struct BStringDeleter {
        typedef BSTR pointer;
        void operator()(pointer const p) noexcept
        {
            SysFreeString(p);
        }
    };

    // Type aliases for unique and shared pointers

    using unique_bstr = std::unique_ptr<std::remove_pointer_t<BSTR>, BStringDeleter>;

    using shared_bstr = std::shared_ptr<std::remove_pointer_t<BSTR>>;

    // Deleter that calls SafeArrayDestroy
    // This deleter terminates the program if SafeArrayDestroy fails
    // This deleter can be used with std::unique_ptr and std::shared_ptr
    // Examples:
    // std::unique_ptr<std::remove_pointer_t<LPSAFEARRAY>, SafeArrayDeleter> x(SafeArrayCreateVector(VT_I4, 0, 10));
    // std::shared_ptr<std::remove_pointer_t<LPSAFEARRAY>> y(SafeArrayCreateVector(VT_I4, 0, 10), SafeArrayDeleter());

    struct SafeArrayDeleter {
        typedef LPSAFEARRAY pointer;
        void operator()(pointer const p) noexcept
        {
            if (p && FAILED(SafeArrayDestroy(p))) std::terminate();
        }
    };

    // Type aliases for unique and shared pointers

    using unique_safearray = std::unique_ptr<std::remove_pointer_t<LPSAFEARRAY>, SafeArrayDeleter>;

    using shared_safearray = std::shared_ptr<std::remove_pointer_t<LPSAFEARRAY>>;
}

#endif  // COMMEM_H

///////////////////////////////////////////////////////////////////////////////
