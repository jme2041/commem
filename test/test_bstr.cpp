// test_bstr.cpp: Tests for commem::BStringDeleter ////////////////////////////
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
// TestBString: Base class for tests of BStringDeleter
//

class TestBString : public TestCommem {
protected:

    // Return a BSTR via an out parameter
    static HRESULT CreateBSTR(
        const wchar_t* const wsz,
        BSTR* const pOut) noexcept
    {
        if (!wsz) return E_INVALIDARG;
        if (!pOut) return E_POINTER;
        *pOut = SysAllocString(wsz);
        return *pOut ? S_OK : E_OUTOFMEMORY;
    }
};

///////////////////////////////////////////////////////////////////////////////
//
// TestBStringUnique: Tests for std::unique_ptr with BStringDeleter
//

class TestBStringUnique : public TestBString {
protected:

    // Accept a VARIANT containing a BSTR
    static HRESULT UseBSTR(VARIANTARG v) noexcept
    {
        // The VARIANT is owned by the caller. Do not steal ownership.

        if (V_VT(&v) != VT_BSTR) return E_INVALIDARG;

        // Process the BSTR here...

        return S_OK;
    }

    // Return a BSTR via a VARIANT
    static HRESULT CreateBSTRVar(
        const wchar_t* const wsz,
        VARIANT* const pOut) noexcept
    {
        auto hr = VariantClear(pOut);
        if (FAILED(hr)) return hr;

        BSTR tmp = nullptr;
        hr = CreateBSTR(wsz, &tmp);
        if (FAILED(hr)) return hr;

        unique_bstr bstr(std::move(tmp));

        // Process the BSTR here...

        // Release the BSTR when storing its pointer in the VARIANT
        V_VT(pOut) = VT_BSTR;
        V_BSTR(pOut) = bstr.release();

        return S_OK;
    }
};

TEST_F(TestBStringUnique, DefaultConstruct)
{
    unique_bstr a;
    ASSERT_FALSE(a);
}

TEST_F(TestBStringUnique, FromNullptr)
{
    unique_bstr a(nullptr);
    ASSERT_FALSE(a);
}

TEST_F(TestBStringUnique, FromPointer)
{
    unique_bstr a(SysAllocString(L"ABCD"));
    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"ABCD");     // This works because no embedded NULs
}

TEST_F(TestBStringUnique, MoveConstruct)
{
    unique_bstr a(SysAllocString(L"ABCD"));
    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"ABCD");

    unique_bstr b(std::move(a));
    ASSERT_TRUE(b);
    ASSERT_STREQ(b.get(), L"ABCD");
}

TEST_F(TestBStringUnique, NullptrAssign)
{
    unique_bstr a(SysAllocString(L"ABCD"));
    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"ABCD");

    a = nullptr;
    ASSERT_FALSE(a);
}

TEST_F(TestBStringUnique, MoveAssign)
{
    unique_bstr a(SysAllocString(L"ABCD"));
    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"ABCD");

    unique_bstr b(SysAllocString(L"EFGH"));
    ASSERT_TRUE(b);
    ASSERT_STREQ(b.get(), L"EFGH");

    b = std::move(a);
    ASSERT_TRUE(b);
    ASSERT_STREQ(b.get(), L"ABCD");
}

TEST_F(TestBStringUnique, Get)
{
    unique_bstr a(SysAllocString(L"ABCD"));
    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"ABCD");

    auto const b = a.get();     // Do not call SysFreeString on b
    ASSERT_TRUE(b);
    ASSERT_STREQ(b, L"ABCD");
}

TEST_F(TestBStringUnique, ResetNullptr)
{
    unique_bstr a(SysAllocString(L"ABCD"));
    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"ABCD");

    a.reset();
    ASSERT_FALSE(a);
}

TEST_F(TestBStringUnique, ResetOrig)
{
    unique_bstr a;
    ASSERT_FALSE(a);

    a.reset(SysAllocString(L"ABCD"));
    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"ABCD");
}

TEST_F(TestBStringUnique, ResetReplace)
{
    unique_bstr a(SysAllocString(L"ABCD"));
    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"ABCD");

    a.reset(SysAllocString(L"EFGH"));
    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"EFGH");
}

TEST_F(TestBStringUnique, Release)
{
    unique_bstr a(SysAllocString(L"ABCD"));
    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"ABCD");

    auto const b = a.release();

    ASSERT_FALSE(a);

    EXPECT_STREQ(b, L"ABCD");
    SysFreeString(b);           // Call SysFreeString on b
}

TEST_F(TestBStringUnique, SwapMember)
{
    unique_bstr a(SysAllocString(L"ABCD"));
    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"ABCD");
    auto const pa = a.get();

    unique_bstr b(SysAllocString(L"EFGH"));
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

TEST_F(TestBStringUnique, SwapFree)
{
    unique_bstr a(SysAllocString(L"ABCD"));
    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"ABCD");
    auto const pa = a.get();

    unique_bstr b(SysAllocString(L"EFGH"));
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

TEST_F(TestBStringUnique, PutOrig)
{
    LPOLESTR tmp = nullptr;
    ASSERT_HRESULT_SUCCEEDED(CreateBSTR(L"ABCD", &tmp));

    unique_bstr a(std::move(tmp));

    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"ABCD");
}

TEST_F(TestBStringUnique, PutReplace)
{
    unique_bstr a(SysAllocString(L"ABCD"));
    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"ABCD");

    LPOLESTR tmp = nullptr;
    ASSERT_HRESULT_SUCCEEDED(CreateBSTR(L"EFGH", &tmp));

    a.reset(std::move(tmp));

    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"EFGH");
}

TEST_F(TestBStringUnique, Copy)
{
    unique_bstr a(SysAllocString(L"ABCD"));
    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"ABCD");

    // This demonstrates how to copy a BSTR into a new unique_bstr
    unique_bstr b(SysAllocStringLen(a.get(), SysStringLen(a.get())));
    ASSERT_TRUE(b);
    ASSERT_STREQ(b.get(), L"ABCD");

    // Pointers should not be equal
    ASSERT_NE(a, b);

    // Strings should be equal
    ASSERT_EQ(VarBstrCmp(
        a.get(),
        b.get(),
        LOCALE_USER_DEFAULT,
        0), VARCMP_EQ);

    b.reset(SysAllocString(L"EFGH"));
    ASSERT_TRUE(b);
    ASSERT_STREQ(b.get(), L"EFGH");

    // Now, the strings should not be equal
    ASSERT_EQ(VarBstrCmp(
        a.get(),
        b.get(),
        LOCALE_USER_DEFAULT,
        0), VARCMP_LT);
}

TEST_F(TestBStringUnique, VariantIn)
{
    unique_bstr a(SysAllocString(L"ABCD"));
    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"ABCD");

    // Process the BSTR here...

    // Release the unique_bstr when transferring ownership to a VARIANT
    VARIANT v;
    VariantInit(&v);
    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = a.release();

    EXPECT_HRESULT_SUCCEEDED(UseBSTR(v));

    // Clearing the VARIANT frees the BSTR
    ASSERT_HRESULT_SUCCEEDED(VariantClear(&v));
}

TEST_F(TestBStringUnique, VariantOut)
{
    VARIANT v;
    VariantInit(&v);

    ASSERT_HRESULT_SUCCEEDED(CreateBSTRVar(L"ABCD", &v));

    ASSERT_EQ(V_VT(&v), VT_BSTR);
    ASSERT_STREQ(V_BSTR(&v), L"ABCD");

    // If taking ownership from the VARIANT, manually clear the VARIANT
    unique_bstr a(V_BSTR(&v));
    V_VT(&v) = VT_EMPTY;
    V_BSTR(&v) = nullptr;

    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"ABCD");

    // When the VARIANT is cleared, it doesn't double delete the BSTR
    ASSERT_HRESULT_SUCCEEDED(VariantClear(&v));
}

///////////////////////////////////////////////////////////////////////////////
//
// TestBStringShared: Tests for std::shared_ptr with BStringDeleter
//

class TestBStringShared : public TestBString { };

TEST_F(TestBStringShared, FromNullptr)
{
    shared_bstr a(nullptr, BStringDeleter());
    ASSERT_FALSE(a);
    ASSERT_EQ(a.use_count(), 1);    // Custom deleter so shared_ptr not empty
}

TEST_F(TestBStringShared, FromPointer)
{
    shared_bstr a(SysAllocString(L"ABCD"), BStringDeleter());
    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"ABCD");
    ASSERT_EQ(a.use_count(), 1);
}

TEST_F(TestBStringShared, CopyConstruct)
{
    shared_bstr a(SysAllocString(L"ABCD"), BStringDeleter());
    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"ABCD");
    ASSERT_EQ(a.use_count(), 1);

    shared_bstr b(a);
    ASSERT_TRUE(b);
    ASSERT_STREQ(b.get(), L"ABCD");
    ASSERT_EQ(a, b);
    ASSERT_EQ(a.use_count(), 2);
    ASSERT_EQ(b.use_count(), 2);
}

TEST_F(TestBStringShared, MoveConstruct)
{
    shared_bstr a(SysAllocString(L"ABCD"), BStringDeleter());
    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"ABCD");
    ASSERT_EQ(a.use_count(), 1);

    shared_bstr b(std::move(a));
    ASSERT_TRUE(b);
    ASSERT_STREQ(b.get(), L"ABCD");
    ASSERT_EQ(b.use_count(), 1);
}

TEST_F(TestBStringShared, UniquePtrConstruct)
{
    unique_bstr a(SysAllocString(L"ABCD"));
    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"ABCD");

    shared_bstr b(std::move(a));
    ASSERT_TRUE(b);
    ASSERT_STREQ(b.get(), L"ABCD");
    ASSERT_EQ(b.use_count(), 1);
}

TEST_F(TestBStringShared, NullptrAssign)
{
    shared_bstr a(SysAllocString(L"ABCD"), BStringDeleter());
    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"ABCD");
    ASSERT_EQ(a.use_count(), 1);

    a = nullptr;                    // Note that this removes the deleter
    ASSERT_FALSE(a);
    ASSERT_EQ(a.use_count(), 0);    // The shared_ptr is empty
}

TEST_F(TestBStringShared, CopyAssign)
{
    shared_bstr a(SysAllocString(L"ABCD"), BStringDeleter());
    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"ABCD");
    ASSERT_EQ(a.use_count(), 1);

    shared_bstr b(SysAllocString(L"EFGH"), BStringDeleter());
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

TEST_F(TestBStringShared, MoveAssign)
{
    shared_bstr a(SysAllocString(L"ABCD"), BStringDeleter());
    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"ABCD");
    ASSERT_EQ(a.use_count(), 1);

    shared_bstr b(SysAllocString(L"EFGH"), BStringDeleter());
    ASSERT_TRUE(b);
    ASSERT_STREQ(b.get(), L"EFGH");
    ASSERT_EQ(b.use_count(), 1);

    b = std::move(a);
    ASSERT_TRUE(b);
    ASSERT_STREQ(b.get(), L"ABCD");
    ASSERT_EQ(b.use_count(), 1);
}

TEST_F(TestBStringShared, UniquePtrAssign)
{
    unique_bstr a(SysAllocString(L"ABCD"));
    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"ABCD");

    shared_bstr b(SysAllocString(L"EFGH"), BStringDeleter());
    ASSERT_TRUE(b);
    ASSERT_STREQ(b.get(), L"EFGH");
    ASSERT_EQ(b.use_count(), 1);

    b = std::move(a);
    ASSERT_TRUE(b);
    ASSERT_STREQ(b.get(), L"ABCD");
    ASSERT_EQ(b.use_count(), 1);
}

TEST_F(TestBStringShared, Get)
{
    shared_bstr a(SysAllocString(L"ABCD"), BStringDeleter());
    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"ABCD");
    ASSERT_EQ(a.use_count(), 1);

    auto const b = a.get();     // Do not call CoTaskMemFree on b
    ASSERT_TRUE(b);
    ASSERT_STREQ(b, L"ABCD");
}

TEST_F(TestBStringShared, ResetNoarg)
{
    shared_bstr a(SysAllocString(L"ABCD"), BStringDeleter());
    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"ABCD");
    ASSERT_EQ(a.use_count(), 1);

    a.reset();
    ASSERT_FALSE(a);
    ASSERT_EQ(a.use_count(), 0);
}

TEST_F(TestBStringShared, ResetReplace)
{
    shared_bstr a(SysAllocString(L"ABCD"), BStringDeleter());
    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"ABCD");
    ASSERT_EQ(a.use_count(), 1);

    a.reset(SysAllocString(L"EFGH"), BStringDeleter());
    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"EFGH");
    ASSERT_EQ(a.use_count(), 1);
}

TEST_F(TestBStringShared, SwapMember)
{
    shared_bstr a(SysAllocString(L"ABCD"), BStringDeleter());
    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"ABCD");
    ASSERT_EQ(a.use_count(), 1);
    auto const pa = a.get();

    shared_bstr b(SysAllocString(L"EFGH"), BStringDeleter());
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

TEST_F(TestBStringShared, SwapFree)
{
    shared_bstr a(SysAllocString(L"ABCD"), BStringDeleter());
    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"ABCD");
    ASSERT_EQ(a.use_count(), 1);
    auto const pa = a.get();

    shared_bstr b(SysAllocString(L"EFGH"), BStringDeleter());
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

TEST_F(TestBStringShared, PutOrig)
{
    LPOLESTR tmp = nullptr;
    ASSERT_HRESULT_SUCCEEDED(CreateBSTR(L"ABCD", &tmp));

    shared_bstr a(std::move(tmp), BStringDeleter());

    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"ABCD");
    ASSERT_EQ(a.use_count(), 1);
}

TEST_F(TestBStringShared, PutReplace)
{
    shared_bstr a(SysAllocString(L"ABCD"), BStringDeleter());
    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"ABCD");
    ASSERT_EQ(a.use_count(), 1);

    LPOLESTR tmp = nullptr;
    ASSERT_HRESULT_SUCCEEDED(CreateBSTR(L"EFGH", &tmp));

    a.reset(std::move(tmp), BStringDeleter());

    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"EFGH");
    ASSERT_EQ(a.use_count(), 1);
}

TEST_F(TestBStringShared, Copy)
{
    shared_bstr a(SysAllocString(L"ABCD"), BStringDeleter());
    ASSERT_TRUE(a);
    ASSERT_STREQ(a.get(), L"ABCD");
    ASSERT_EQ(a.use_count(), 1);

    // This demonstrates how to copy a BSTR into a new shared_bstr
    shared_bstr b(SysAllocStringLen(a.get(), SysStringLen(a.get())),
        BStringDeleter());
    ASSERT_TRUE(b);
    ASSERT_STREQ(b.get(), L"ABCD");
    ASSERT_EQ(b.use_count(), 1);

    // Pointers should not be equal
    ASSERT_NE(a, b);

    // Strings should be equal
    ASSERT_EQ(VarBstrCmp(
        a.get(),
        b.get(),
        LOCALE_USER_DEFAULT,
        0), VARCMP_EQ);

    b.reset(SysAllocString(L"EFGH"), BStringDeleter());
    ASSERT_TRUE(b);
    ASSERT_STREQ(b.get(), L"EFGH");
    ASSERT_EQ(b.use_count(), 1);

    // Now, the strings should not be equal
    ASSERT_EQ(VarBstrCmp(
        a.get(),
        b.get(),
        LOCALE_USER_DEFAULT,
        0), VARCMP_LT);
}

///////////////////////////////////////////////////////////////////////////////
