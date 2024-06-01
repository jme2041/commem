// test_heap.cpp: Tests for commem::ComHeapDeleter ////////////////////////////
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

#include "commem.h"
#include "test_commem.h"

using namespace commem;

///////////////////////////////////////////////////////////////////////////////
//
// TestHeap: Base class for tests of ComHeapDeleter
//

class TestHeap : public TestCommem {
protected:
    typedef ComHeapDeleter<LPOLESTR> olestr_deleter;

    typedef unique_heap<LPOLESTR> unique_olestr;

    typedef shared_heap<LPOLESTR> shared_olestr;

    // Test ComHeapDeleter with an OLESTR allocated using CoTaskMemAlloc
    static LPOLESTR ReturnOLESTR(wchar_t const* const wsz) noexcept
    {
        assert(wsz);
        auto const cch = wcslen(wsz) + 1;
        auto p = static_cast<LPOLESTR>(CoTaskMemAlloc(cch * sizeof(wchar_t)));
        if (p && wcscpy_s(p, cch, wsz))
        {
            CoTaskMemFree(p);
            p = nullptr;
        }
        return p;
    }

    // Return an OLESTR via an out parameter
    static HRESULT CreateOLESTR(
        wchar_t const* const wsz,
        LPOLESTR* const pOut) noexcept
    {
        if (!wsz) return E_INVALIDARG;
        if (!pOut) return E_POINTER;
        *pOut = ReturnOLESTR(wsz);
        return *pOut ? S_OK : E_OUTOFMEMORY;
    }
};

///////////////////////////////////////////////////////////////////////////////
//
// TestHeapUnique: Tests for std::unique_ptr with ComHeapDeleter
//

class TestHeapUnique : public TestHeap { };

TEST_F(TestHeapUnique, DefaultConstruct)
{
    unique_olestr a;
    ASSERT_FALSE(a);
}

TEST_F(TestHeapUnique, FromNullptr)
{
    unique_olestr a(nullptr);
    ASSERT_FALSE(a);
}

TEST_F(TestHeapUnique, FromPointer)
{
    unique_olestr a(ReturnOLESTR(L"ABCD"));
    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"ABCD");
}

TEST_F(TestHeapUnique, MoveConstruct)
{
    unique_olestr a(ReturnOLESTR(L"ABCD"));
    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"ABCD");

    unique_olestr b(std::move(a));
    ASSERT_TRUE(b);
    ASSERT_STREQ(b.get(), L"ABCD");
}

TEST_F(TestHeapUnique, NullptrAssign)
{
    unique_olestr a(ReturnOLESTR(L"ABCD"));
    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"ABCD");

    a = nullptr;
    ASSERT_FALSE(a);
}

TEST_F(TestHeapUnique, MoveAssign)
{
    unique_olestr a(ReturnOLESTR(L"ABCD"));
    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"ABCD");

    unique_olestr b(ReturnOLESTR(L"EFGH"));
    ASSERT_TRUE(b);
    ASSERT_STREQ(b.get(), L"EFGH");

    b = std::move(a);
    ASSERT_TRUE(b);
    ASSERT_STREQ(b.get(), L"ABCD");
}

TEST_F(TestHeapUnique, Get)
{
    unique_olestr a(ReturnOLESTR(L"ABCD"));
    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"ABCD");

    auto const b = a.get();     // Do not call CoTaskMemFree on b
    ASSERT_TRUE(b);
    ASSERT_STREQ(b, L"ABCD");
}

TEST_F(TestHeapUnique, ResetNullptr)
{
    unique_olestr a(ReturnOLESTR(L"ABCD"));
    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"ABCD");

    a.reset();
    ASSERT_FALSE(a);
}

TEST_F(TestHeapUnique, ResetOrig)
{
    unique_olestr a;
    ASSERT_FALSE(a);

    a.reset(ReturnOLESTR(L"ABCD"));
    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"ABCD");
}

TEST_F(TestHeapUnique, ResetReplace)
{
    unique_olestr a(ReturnOLESTR(L"ABCD"));
    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"ABCD");

    a.reset(ReturnOLESTR(L"EFGH"));
    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"EFGH");
}

TEST_F(TestHeapUnique, Release)
{
    unique_olestr a(ReturnOLESTR(L"ABCD"));
    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"ABCD");

    auto const b = a.release();

    ASSERT_FALSE(a);

    EXPECT_STREQ(b, L"ABCD");
    CoTaskMemFree(b);           // Call CoTaskMemFree on b
}

TEST_F(TestHeapUnique, SwapMember)
{
    unique_olestr a(ReturnOLESTR(L"ABCD"));
    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"ABCD");
    auto const pa = a.get();

    unique_olestr b(ReturnOLESTR(L"EFGH"));
    ASSERT_TRUE(b);
    ASSERT_STREQ(b.get(), L"EFGH");
    auto const pb = b.get();

    a.swap(b);

    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"EFGH");
    ASSERT_EQ(a.get(), pb);

    ASSERT_TRUE(b);
    ASSERT_STREQ(b.get(), L"ABCD");
    ASSERT_EQ(b.get(), pa);
}

TEST_F(TestHeapUnique, SwapFree)
{
    unique_olestr a(ReturnOLESTR(L"ABCD"));
    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"ABCD");
    auto const pa = a.get();

    unique_olestr b(ReturnOLESTR(L"EFGH"));
    ASSERT_TRUE(b);
    ASSERT_STREQ(b.get(), L"EFGH");
    auto const pb = b.get();

    swap(a, b);

    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"EFGH");
    ASSERT_EQ(a.get(), pb);

    ASSERT_TRUE(b);
    ASSERT_STREQ(b.get(), L"ABCD");
    ASSERT_EQ(b.get(), pa);
}

TEST_F(TestHeapUnique, PutOrig)
{
    LPOLESTR tmp = nullptr;
    ASSERT_HRESULT_SUCCEEDED(CreateOLESTR(L"ABCD", &tmp));

    unique_olestr a(std::move(tmp));

    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"ABCD");
}

TEST_F(TestHeapUnique, PutReplace)
{
    unique_olestr a(ReturnOLESTR(L"ABCD"));
    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"ABCD");

    LPOLESTR tmp = nullptr;
    ASSERT_HRESULT_SUCCEEDED(CreateOLESTR(L"EFGH", &tmp));

    a.reset(std::move(tmp));

    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"EFGH");
}

///////////////////////////////////////////////////////////////////////////////
//
// TestHeapShared: Tests for std::shared_ptr with ComHeapDeleter
//

class TestHeapShared : public TestHeap { };

TEST_F(TestHeapShared, FromNullptr)
{
    shared_olestr a(nullptr, olestr_deleter());
    ASSERT_FALSE(a);
    ASSERT_EQ(a.use_count(), 1);    // Custom deleter so shared_ptr not empty
}

TEST_F(TestHeapShared, FromPointer)
{
    shared_olestr a(ReturnOLESTR(L"ABCD"), olestr_deleter());
    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"ABCD");
    ASSERT_EQ(a.use_count(), 1);
}

TEST_F(TestHeapShared, CopyConstruct)
{
    shared_olestr a(ReturnOLESTR(L"ABCD"), olestr_deleter());
    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"ABCD");
    ASSERT_EQ(a.use_count(), 1);

    shared_olestr b(a);
    ASSERT_TRUE(b);
    ASSERT_STREQ(b.get(), L"ABCD");
    ASSERT_EQ(a, b);
    ASSERT_EQ(a.use_count(), 2);
    ASSERT_EQ(b.use_count(), 2);
}

TEST_F(TestHeapShared, MoveConstruct)
{
    shared_olestr a(ReturnOLESTR(L"ABCD"), olestr_deleter());
    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"ABCD");
    ASSERT_EQ(a.use_count(), 1);

    shared_olestr b(std::move(a));
    ASSERT_TRUE(b);
    ASSERT_STREQ(b.get(), L"ABCD");
    ASSERT_EQ(b.use_count(), 1);
}

TEST_F(TestHeapShared, UniquePtrConstruct)
{
    unique_olestr a(ReturnOLESTR(L"ABCD"));
    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"ABCD");

    shared_olestr b(std::move(a));
    ASSERT_TRUE(b);
    ASSERT_STREQ(b.get(), L"ABCD");
    ASSERT_EQ(b.use_count(), 1);
}

TEST_F(TestHeapShared, NullptrAssign)
{
    shared_olestr a(ReturnOLESTR(L"ABCD"), olestr_deleter());
    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"ABCD");
    ASSERT_EQ(a.use_count(), 1);

    a = nullptr;                    // Note that this removes the deleter
    ASSERT_FALSE(a);
    ASSERT_EQ(a.use_count(), 0);    // The shared_ptr is empty
}

TEST_F(TestHeapShared, CopyAssign)
{
    shared_olestr a(ReturnOLESTR(L"ABCD"), olestr_deleter());
    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"ABCD");
    ASSERT_EQ(a.use_count(), 1);

    shared_olestr b(ReturnOLESTR(L"EFGH"), olestr_deleter());
    ASSERT_TRUE(b);
    ASSERT_STREQ(b.get(), L"EFGH");
    ASSERT_EQ(b.use_count(), 1);

    b = a;
    ASSERT_TRUE(b);
    ASSERT_STREQ(b.get(), L"ABCD");
    ASSERT_EQ(a, b);
    ASSERT_EQ(a.use_count(), 2);
    ASSERT_EQ(b.use_count(), 2);
}

TEST_F(TestHeapShared, MoveAssign)
{
    shared_olestr a(ReturnOLESTR(L"ABCD"), olestr_deleter());
    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"ABCD");
    ASSERT_EQ(a.use_count(), 1);

    shared_olestr b(ReturnOLESTR(L"EFGH"), olestr_deleter());
    ASSERT_TRUE(b);
    ASSERT_STREQ(b.get(), L"EFGH");
    ASSERT_EQ(b.use_count(), 1);

    b = std::move(a);
    ASSERT_TRUE(b);
    ASSERT_STREQ(b.get(), L"ABCD");
    ASSERT_EQ(b.use_count(), 1);
}

TEST_F(TestHeapShared, UniquePtrAssign)
{
    unique_olestr a(ReturnOLESTR(L"ABCD"));
    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"ABCD");

    shared_olestr b(ReturnOLESTR(L"EFGH"), olestr_deleter());
    ASSERT_TRUE(b);
    ASSERT_STREQ(b.get(), L"EFGH");
    ASSERT_EQ(b.use_count(), 1);

    b = std::move(a);
    ASSERT_TRUE(b);
    ASSERT_STREQ(b.get(), L"ABCD");
    ASSERT_EQ(b.use_count(), 1);
}

TEST_F(TestHeapShared, Get)
{
    shared_olestr a(ReturnOLESTR(L"ABCD"), olestr_deleter());
    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"ABCD");
    ASSERT_EQ(a.use_count(), 1);

    auto const b = a.get();     // Do not call CoTaskMemFree on b
    ASSERT_TRUE(b);
    ASSERT_STREQ(b, L"ABCD");
}

TEST_F(TestHeapShared, ResetNoarg)
{
    shared_olestr a(ReturnOLESTR(L"ABCD"), olestr_deleter());
    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"ABCD");
    ASSERT_EQ(a.use_count(), 1);

    a.reset();
    ASSERT_FALSE(a);
    ASSERT_EQ(a.use_count(), 0);
}

TEST_F(TestHeapShared, ResetReplace)
{
    shared_olestr a(ReturnOLESTR(L"ABCD"), olestr_deleter());
    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"ABCD");
    ASSERT_EQ(a.use_count(), 1);

    a.reset(ReturnOLESTR(L"EFGH"), olestr_deleter());
    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"EFGH");
    ASSERT_EQ(a.use_count(), 1);
}

TEST_F(TestHeapShared, SwapMember)
{
    shared_olestr a(ReturnOLESTR(L"ABCD"), olestr_deleter());
    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"ABCD");
    ASSERT_EQ(a.use_count(), 1);
    auto const pa = a.get();

    shared_olestr b(ReturnOLESTR(L"EFGH"), olestr_deleter());
    ASSERT_TRUE(b);
    ASSERT_STREQ(b.get(), L"EFGH");
    ASSERT_EQ(b.use_count(), 1);
    auto const pb = b.get();

    a.swap(b);

    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"EFGH");
    ASSERT_EQ(a.use_count(), 1);
    ASSERT_EQ(a.get(), pb);

    ASSERT_TRUE(b);
    ASSERT_STREQ(b.get(), L"ABCD");
    ASSERT_EQ(b.use_count(), 1);
    ASSERT_EQ(b.get(), pa);
}

TEST_F(TestHeapShared, SwapFree)
{
    shared_olestr a(ReturnOLESTR(L"ABCD"), olestr_deleter());
    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"ABCD");
    ASSERT_EQ(a.use_count(), 1);
    auto const pa = a.get();

    shared_olestr b(ReturnOLESTR(L"EFGH"), olestr_deleter());
    ASSERT_TRUE(b);
    ASSERT_STREQ(b.get(), L"EFGH");
    ASSERT_EQ(b.use_count(), 1);
    auto const pb = b.get();

    swap(a, b);

    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"EFGH");
    ASSERT_EQ(a.use_count(), 1);
    ASSERT_EQ(a.get(), pb);

    ASSERT_TRUE(b);
    ASSERT_STREQ(b.get(), L"ABCD");
    ASSERT_EQ(b.use_count(), 1);
    ASSERT_EQ(b.get(), pa);
}

TEST_F(TestHeapShared, PutOrig)
{
    LPOLESTR tmp = nullptr;
    ASSERT_HRESULT_SUCCEEDED(CreateOLESTR(L"ABCD", &tmp));

    shared_olestr a(std::move(tmp), olestr_deleter());

    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"ABCD");
    ASSERT_EQ(a.use_count(), 1);
}

TEST_F(TestHeapShared, PutReplace)
{
    shared_olestr a(ReturnOLESTR(L"ABCD"), olestr_deleter());
    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"ABCD");
    ASSERT_EQ(a.use_count(), 1);

    LPOLESTR tmp = nullptr;
    ASSERT_HRESULT_SUCCEEDED(CreateOLESTR(L"EFGH", &tmp));

    a.reset(std::move(tmp), olestr_deleter());

    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"EFGH");
    ASSERT_EQ(a.use_count(), 1);
}

///////////////////////////////////////////////////////////////////////////////
