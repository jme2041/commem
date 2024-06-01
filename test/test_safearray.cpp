// test_safearray.cpp: Tests for commem::SafeArrayDeleter deleter /////////////
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
// TestSafeArray: Base class for tests of SafeArrayDeleter
//

class TestSafeArray : public TestCommem {
protected:

    // Get the VARTYPE of a SAFEARRAY. Return VT_ERROR upon failure.
    static VARTYPE SafeArrayGetVartype(LPSAFEARRAY psa) noexcept
    {
        VARTYPE vt = VT_ERROR;
        if (FAILED(::SafeArrayGetVartype(psa, &vt))) return VT_ERROR;
        return vt;
    }

    // Return a SafeArray via an out parameter
    static HRESULT CreateSafeArray(
        VARTYPE const vt,
        LPSAFEARRAY* const pOut) noexcept
    {
        if (!pOut) return E_POINTER;
        *pOut = SafeArrayCreateVector(vt, 0, 10);
        return *pOut ? S_OK : E_OUTOFMEMORY;
    }
};

///////////////////////////////////////////////////////////////////////////////
//
// TestSafeArrayUnique: Tests for std::unique_ptr with SafeArrayDeleter
//

class TestSafeArrayUnique : public TestSafeArray {
protected:

    // Accept a VARIANT containing a SAFEARRAY
    static HRESULT UseSafeArray(VARIANTARG v) noexcept
    {
        // The VARIANT is owned by the caller. Do not steal ownership.

        if (!(V_VT(&v) & VT_ARRAY)) return E_INVALIDARG;

        // Process the SAFEARRAY here...

        return S_OK;
    }

    // Return a SAFEARRAY via a VARIANT
    static HRESULT CreateSafeArrayVar(
        VARTYPE const vt,
        VARIANT* const pOut) noexcept
    {
        auto hr = VariantClear(pOut);
        if (FAILED(hr)) return hr;

        LPSAFEARRAY tmp = nullptr;
        hr = CreateSafeArray(vt, &tmp);
        if (FAILED(hr)) return hr;

        unique_safearray sa(std::move(tmp));

        // Process the SAFEARRAY here...

        // Release the SAFEARRAY when storing its pointer in the VARIANT
        V_VT(pOut) = vt | VT_ARRAY;     // Assume array type hasn't changed
        V_ARRAY(pOut) = sa.release();

        return S_OK;
    }
};

TEST_F(TestSafeArrayUnique, DefaultConstruct)
{
    unique_safearray a;
    ASSERT_FALSE(a);
}

TEST_F(TestSafeArrayUnique, FromNullptr)
{
    unique_safearray a(nullptr);
    ASSERT_FALSE(a);
}

TEST_F(TestSafeArrayUnique, FromPointer)
{
    unique_safearray a(SafeArrayCreateVector(VT_I4, 0, 10));
    ASSERT_TRUE(a);
    ASSERT_EQ(SafeArrayGetDim(a.get()), 1);     // Is the SAFEARRAY valid?
}

TEST_F(TestSafeArrayUnique, MoveConstruct)
{
    unique_safearray a(SafeArrayCreateVector(VT_I4, 0, 10));
    ASSERT_TRUE(a);
    ASSERT_EQ(SafeArrayGetDim(a.get()), 1);

    unique_safearray b(std::move(a));
    ASSERT_TRUE(b);
    ASSERT_EQ(SafeArrayGetDim(b.get()), 1);
}

TEST_F(TestSafeArrayUnique, NullptrAssign)
{
    unique_safearray a(SafeArrayCreateVector(VT_I4, 0, 10));
    ASSERT_TRUE(a);
    ASSERT_EQ(SafeArrayGetDim(a.get()), 1);

    a = nullptr;
    ASSERT_FALSE(a);
}

TEST_F(TestSafeArrayUnique, MoveAssign)
{
    unique_safearray a(SafeArrayCreateVector(VT_I4, 0, 10));
    ASSERT_TRUE(a);
    ASSERT_EQ(SafeArrayGetVartype(a.get()), VT_I4);

    unique_safearray b(SafeArrayCreateVector(VT_UI4, 0, 10));
    ASSERT_TRUE(b);
    ASSERT_EQ(SafeArrayGetVartype(b.get()), VT_UI4);

    b = std::move(a);
    ASSERT_TRUE(b);
    ASSERT_EQ(SafeArrayGetVartype(b.get()), VT_I4);
}

TEST_F(TestSafeArrayUnique, Get)
{
    unique_safearray a(SafeArrayCreateVector(VT_I4, 0, 10));
    ASSERT_TRUE(a);
    ASSERT_EQ(SafeArrayGetDim(a.get()), 1);

    auto const b = a.get();     // Do not call SafeArrayDestroy on b
    ASSERT_TRUE(b);
    ASSERT_EQ(SafeArrayGetDim(b), 1);
}

TEST_F(TestSafeArrayUnique, ResetNullptr)
{
    unique_safearray a(SafeArrayCreateVector(VT_I4, 0, 10));
    ASSERT_TRUE(a);
    ASSERT_EQ(SafeArrayGetDim(a.get()), 1);

    a.reset();
    ASSERT_FALSE(a);
}

TEST_F(TestSafeArrayUnique, ResetOrig)
{
    unique_safearray a;
    ASSERT_FALSE(a);

    a.reset(SafeArrayCreateVector(VT_I4, 0, 10));
    ASSERT_TRUE(a);
    ASSERT_EQ(SafeArrayGetDim(a.get()), 1);
}

TEST_F(TestSafeArrayUnique, ResetReplace)
{
    unique_safearray a(SafeArrayCreateVector(VT_I4, 0, 10));
    ASSERT_TRUE(a);
    ASSERT_EQ(SafeArrayGetVartype(a.get()), VT_I4);

    a.reset(SafeArrayCreateVector(VT_UI4, 0, 10));
    ASSERT_TRUE(a);
    ASSERT_EQ(SafeArrayGetVartype(a.get()), VT_UI4);
}

TEST_F(TestSafeArrayUnique, Release)
{
    unique_safearray a(SafeArrayCreateVector(VT_I4, 0, 10));
    ASSERT_TRUE(a);
    ASSERT_EQ(SafeArrayGetDim(a.get()), 1);

    auto const b = a.release();

    ASSERT_FALSE(a);

    EXPECT_EQ(SafeArrayGetDim(b), 1);
    SafeArrayDestroy(b);        // Call SafeArrayDestroy on b
}

TEST_F(TestSafeArrayUnique, SwapMember)
{
    unique_safearray a(SafeArrayCreateVector(VT_I4, 0, 10));
    ASSERT_TRUE(a);
    ASSERT_EQ(SafeArrayGetVartype(a.get()), VT_I4);
    auto const pa = a.get();

    unique_safearray b(SafeArrayCreateVector(VT_UI4, 0, 10));
    ASSERT_TRUE(b);
    ASSERT_EQ(SafeArrayGetVartype(b.get()), VT_UI4);
    auto const pb = b.get();

    a.swap(b);

    ASSERT_TRUE(a);
    ASSERT_EQ(SafeArrayGetVartype(a.get()), VT_UI4);
    ASSERT_EQ(a.get(), pb);

    ASSERT_TRUE(b);
    ASSERT_EQ(SafeArrayGetVartype(b.get()), VT_I4);
    ASSERT_EQ(b.get(), pa);
}

TEST_F(TestSafeArrayUnique, SwapFree)
{
    unique_safearray a(SafeArrayCreateVector(VT_I4, 0, 10));
    ASSERT_TRUE(a);
    ASSERT_EQ(SafeArrayGetVartype(a.get()), VT_I4);
    auto const pa = a.get();

    unique_safearray b(SafeArrayCreateVector(VT_UI4, 0, 10));
    ASSERT_TRUE(b);
    ASSERT_EQ(SafeArrayGetVartype(b.get()), VT_UI4);
    auto const pb = b.get();

    swap(a, b);

    ASSERT_TRUE(a);
    ASSERT_EQ(SafeArrayGetVartype(a.get()), VT_UI4);
    ASSERT_EQ(a.get(), pb);

    ASSERT_TRUE(b);
    ASSERT_EQ(SafeArrayGetVartype(b.get()), VT_I4);
    ASSERT_EQ(b.get(), pa);
}

TEST_F(TestSafeArrayUnique, PutOrig)
{
    LPSAFEARRAY tmp = nullptr;
    ASSERT_HRESULT_SUCCEEDED(CreateSafeArray(VT_I4, &tmp));

    unique_safearray a(std::move(tmp));

    ASSERT_TRUE(a);
    ASSERT_EQ(SafeArrayGetDim(a.get()), 1);
}

TEST_F(TestSafeArrayUnique, PutReplace)
{
    unique_safearray a(SafeArrayCreateVector(VT_I4, 0, 10));
    ASSERT_TRUE(a);
    ASSERT_EQ(SafeArrayGetVartype(a.get()), VT_I4);

    LPSAFEARRAY tmp = nullptr;
    ASSERT_HRESULT_SUCCEEDED(CreateSafeArray(VT_UI4, &tmp));

    a.reset(std::move(tmp));

    ASSERT_TRUE(a);
    ASSERT_EQ(SafeArrayGetVartype(a.get()), VT_UI4);
}

TEST_F(TestSafeArrayUnique, Copy)
{
    unique_safearray a(SafeArrayCreateVector(VT_I4, 0, 10));
    ASSERT_TRUE(a);
    ASSERT_EQ(SafeArrayGetVartype(a.get()), VT_I4);

    // This demonstrates how to copy a SAFEARRAY into a new unique_safearray
    LPSAFEARRAY psa;
    ASSERT_HRESULT_SUCCEEDED(SafeArrayCopy(a.get(), &psa));

    unique_safearray b(std::move(psa));
    ASSERT_TRUE(b);

    // Pointers should not be equal
    ASSERT_NE(a, b);

    // Internal data pointers should not be equal
    ASSERT_NE(a->pvData, b->pvData);

    // VARTYPEs should be equal
    ASSERT_EQ(SafeArrayGetVartype(b.get()), VT_I4);
    ASSERT_EQ(SafeArrayGetVartype(a.get()), SafeArrayGetVartype(b.get()));

    b.reset(SafeArrayCreateVector(VT_UI4, 0, 10));
    ASSERT_TRUE(b);

    // Now, the VARTYPES should not be equal
    ASSERT_EQ(SafeArrayGetVartype(b.get()), VT_UI4);
    ASSERT_NE(SafeArrayGetVartype(a.get()), SafeArrayGetVartype(b.get()));
}

TEST_F(TestSafeArrayUnique, VariantIn)
{
    unique_safearray a(SafeArrayCreateVector(VT_I4, 0, 10));
    ASSERT_TRUE(a);
    ASSERT_EQ(SafeArrayGetVartype(a.get()), VT_I4);

    // Process the SAFEARRAY here...

    // Release the unique_safearray when transferring ownership to a VARIANT
    VARIANT v;
    VariantInit(&v);
    V_VT(&v) = SafeArrayGetVartype(a.get()) | VT_ARRAY;
    V_ARRAY(&v) = a.release();

    EXPECT_HRESULT_SUCCEEDED(UseSafeArray(v));

    // Clearing the VARIANT frees the SAFEARRAY
    ASSERT_HRESULT_SUCCEEDED(VariantClear(&v));
}

TEST_F(TestSafeArrayUnique, VariantOut)
{
    VARIANT v;
    VariantInit(&v);

    ASSERT_HRESULT_SUCCEEDED(CreateSafeArrayVar(VT_I4, &v));

    ASSERT_EQ(V_VT(&v), VT_I4 | VT_ARRAY);
    ASSERT_EQ(SafeArrayGetVartype(V_ARRAY(&v)), VT_I4);

    // If taking ownership from the VARIANT, manually clear the VARIANT
    unique_safearray a(V_ARRAY(&v));
    V_VT(&v) = VT_EMPTY;
    V_ARRAY(&v) = nullptr;

    ASSERT_TRUE(a);
    ASSERT_EQ(SafeArrayGetVartype(a.get()), VT_I4);

    // When the VARIANT is cleared, it doesn't double delete the SAFEARRAY
    ASSERT_HRESULT_SUCCEEDED(VariantClear(&v));
}

///////////////////////////////////////////////////////////////////////////////
//
// TestSafeArrayShared: Tests for std::shared_ptr with SafeArrayDeleter
//

class TestSafeArrayShared : public TestSafeArray { };

TEST_F(TestSafeArrayShared, FromNullptr)
{
    shared_safearray a(nullptr, SafeArrayDeleter());
    ASSERT_FALSE(a);
    ASSERT_EQ(a.use_count(), 1);    // Custom deleter so shared_ptr not empty
}

TEST_F(TestSafeArrayShared, FromPointer)
{
    shared_safearray a(SafeArrayCreateVector(VT_I4, 10, 0), SafeArrayDeleter());
    ASSERT_TRUE(a);
    ASSERT_EQ(SafeArrayGetDim(a.get()), 1);     // Is the SAFEARRAY valid?
    ASSERT_EQ(a.use_count(), 1);
}

TEST_F(TestSafeArrayShared, CopyConstruct)
{
    shared_safearray a(SafeArrayCreateVector(VT_I4, 10, 0), SafeArrayDeleter());
    ASSERT_TRUE(a);
    ASSERT_EQ(SafeArrayGetDim(a.get()), 1);
    ASSERT_EQ(a.use_count(), 1);

    shared_safearray b(a);
    ASSERT_TRUE(b);
    ASSERT_EQ(SafeArrayGetDim(b.get()), 1);
    ASSERT_EQ(a, b);
    ASSERT_EQ(a.use_count(), 2);
    ASSERT_EQ(b.use_count(), 2);
}

TEST_F(TestSafeArrayShared, MoveConstruct)
{
    shared_safearray a(SafeArrayCreateVector(VT_I4, 0, 10), SafeArrayDeleter());
    ASSERT_TRUE(a);
    ASSERT_EQ(SafeArrayGetDim(a.get()), 1);
    ASSERT_EQ(a.use_count(), 1);

    shared_safearray b(std::move(a));
    ASSERT_TRUE(b);
    ASSERT_EQ(SafeArrayGetDim(b.get()), 1);
    ASSERT_EQ(b.use_count(), 1);
}

TEST_F(TestSafeArrayShared, UniquePtrConstruct)
{
    unique_safearray a(SafeArrayCreateVector(VT_I4, 0, 10));
    ASSERT_TRUE(a);
    ASSERT_EQ(SafeArrayGetDim(a.get()), 1);

    shared_safearray b(std::move(a));
    ASSERT_TRUE(b);
    ASSERT_EQ(SafeArrayGetDim(b.get()), 1);
    ASSERT_EQ(b.use_count(), 1);
}

TEST_F(TestSafeArrayShared, NullptrAssign)
{
    shared_safearray a(SafeArrayCreateVector(VT_I4, 0, 10), SafeArrayDeleter());
    ASSERT_TRUE(a);
    ASSERT_EQ(SafeArrayGetDim(a.get()), 1);
    ASSERT_EQ(a.use_count(), 1);

    a = nullptr;                    // Note that this removes the deleter
    ASSERT_FALSE(a);
    ASSERT_EQ(a.use_count(), 0);    // The shared_ptr is empty
}

TEST_F(TestSafeArrayShared, CopyAssign)
{
    shared_safearray a(SafeArrayCreateVector(VT_I4, 0, 10), SafeArrayDeleter());
    ASSERT_TRUE(a);
    ASSERT_EQ(SafeArrayGetVartype(a.get()), VT_I4);
    ASSERT_EQ(a.use_count(), 1);

    shared_safearray b(SafeArrayCreateVector(VT_UI4, 0, 10), SafeArrayDeleter());
    ASSERT_TRUE(b);
    ASSERT_EQ(SafeArrayGetVartype(b.get()), VT_UI4);
    ASSERT_EQ(b.use_count(), 1);

    b = a;
    ASSERT_TRUE(b);
    ASSERT_EQ(SafeArrayGetVartype(b.get()), VT_I4);
    ASSERT_EQ(a.use_count(), 2);
    ASSERT_EQ(b.use_count(), 2);
}

TEST_F(TestSafeArrayShared, MoveAssign)
{
    shared_safearray a(SafeArrayCreateVector(VT_I4, 0, 10), SafeArrayDeleter());
    ASSERT_TRUE(a);
    ASSERT_EQ(SafeArrayGetVartype(a.get()), VT_I4);
    ASSERT_EQ(a.use_count(), 1);

    shared_safearray b(SafeArrayCreateVector(VT_UI4, 0, 10), SafeArrayDeleter());
    ASSERT_TRUE(b);
    ASSERT_EQ(SafeArrayGetVartype(b.get()), VT_UI4);
    ASSERT_EQ(b.use_count(), 1);

    b = std::move(a);
    ASSERT_TRUE(b);
    ASSERT_EQ(SafeArrayGetVartype(b.get()), VT_I4);
    ASSERT_EQ(b.use_count(), 1);
}

TEST_F(TestSafeArrayShared, UniquePtrAssign)
{
    unique_safearray a(SafeArrayCreateVector(VT_I4, 0, 10));
    ASSERT_TRUE(a);
    ASSERT_EQ(SafeArrayGetVartype(a.get()), VT_I4);

    shared_safearray b(SafeArrayCreateVector(VT_UI4, 0, 10), SafeArrayDeleter());
    ASSERT_TRUE(b);
    ASSERT_EQ(SafeArrayGetVartype(b.get()), VT_UI4);
    ASSERT_EQ(b.use_count(), 1);

    b = std::move(a);
    ASSERT_TRUE(b);
    ASSERT_EQ(SafeArrayGetVartype(b.get()), VT_I4);
    ASSERT_EQ(b.use_count(), 1);
}

TEST_F(TestSafeArrayShared, Get)
{
    shared_safearray a(SafeArrayCreateVector(VT_I4, 0, 10), SafeArrayDeleter());
    ASSERT_TRUE(a);
    ASSERT_EQ(SafeArrayGetDim(a.get()), 1);
    ASSERT_EQ(a.use_count(), 1);

    auto const b = a.get();     // Do not call SafeArrayDestroy on b
    ASSERT_TRUE(b);
    ASSERT_EQ(SafeArrayGetDim(b), 1);
}

TEST_F(TestSafeArrayShared, ResetNoarg)
{
    shared_safearray a(SafeArrayCreateVector(VT_I4, 0, 10), SafeArrayDeleter());
    ASSERT_TRUE(a);
    ASSERT_EQ(SafeArrayGetDim(a.get()), 1);
    ASSERT_EQ(a.use_count(), 1);

    a.reset();
    ASSERT_FALSE(a);
    ASSERT_EQ(a.use_count(), 0);
}

TEST_F(TestSafeArrayShared, ResetReplace)
{
    shared_safearray a(SafeArrayCreateVector(VT_I4, 0, 10), SafeArrayDeleter());
    ASSERT_TRUE(a);
    ASSERT_EQ(SafeArrayGetVartype(a.get()), VT_I4);
    ASSERT_EQ(a.use_count(), 1);

    a.reset(SafeArrayCreateVector(VT_UI4, 0, 10), SafeArrayDeleter());
    ASSERT_TRUE(a);
    ASSERT_EQ(SafeArrayGetVartype(a.get()), VT_UI4);
    ASSERT_EQ(a.use_count(), 1);
}

TEST_F(TestSafeArrayShared, SwapMember)
{
    shared_safearray a(SafeArrayCreateVector(VT_I4, 0, 10), SafeArrayDeleter());
    ASSERT_TRUE(a);
    ASSERT_EQ(SafeArrayGetVartype(a.get()), VT_I4);
    ASSERT_EQ(a.use_count(), 1);
    auto const pa = a.get();

    shared_safearray b(SafeArrayCreateVector(VT_UI4, 0, 10), SafeArrayDeleter());
    ASSERT_TRUE(b);
    ASSERT_EQ(SafeArrayGetVartype(b.get()), VT_UI4);
    ASSERT_EQ(b.use_count(), 1);
    auto const pb = b.get();

    a.swap(b);

    ASSERT_TRUE(a);
    ASSERT_EQ(SafeArrayGetVartype(a.get()), VT_UI4);
    ASSERT_EQ(a.use_count(), 1);
    ASSERT_EQ(a.get(), pb);

    ASSERT_TRUE(b);
    ASSERT_EQ(SafeArrayGetVartype(b.get()), VT_I4);
    ASSERT_EQ(b.use_count(), 1);
    ASSERT_EQ(b.get(), pa);
}

TEST_F(TestSafeArrayShared, SwapFree)
{
    shared_safearray a(SafeArrayCreateVector(VT_I4, 0, 10), SafeArrayDeleter());
    ASSERT_TRUE(a);
    ASSERT_EQ(SafeArrayGetVartype(a.get()), VT_I4);
    ASSERT_EQ(a.use_count(), 1);
    auto const pa = a.get();

    shared_safearray b(SafeArrayCreateVector(VT_UI4, 0, 10), SafeArrayDeleter());
    ASSERT_TRUE(b);
    ASSERT_EQ(SafeArrayGetVartype(b.get()), VT_UI4);
    ASSERT_EQ(b.use_count(), 1);
    auto const pb = b.get();

    swap(a, b);

    ASSERT_TRUE(a);
    ASSERT_EQ(SafeArrayGetVartype(a.get()), VT_UI4);
    ASSERT_EQ(a.use_count(), 1);
    ASSERT_EQ(a.get(), pb);

    ASSERT_TRUE(b);
    ASSERT_EQ(SafeArrayGetVartype(b.get()), VT_I4);
    ASSERT_EQ(b.use_count(), 1);
    ASSERT_EQ(b.get(), pa);
}

TEST_F(TestSafeArrayShared, PutOrig)
{
    LPSAFEARRAY tmp = nullptr;
    ASSERT_HRESULT_SUCCEEDED(CreateSafeArray(VT_I4, &tmp));

    shared_safearray a(std::move(tmp), SafeArrayDeleter());

    ASSERT_TRUE(a);
    ASSERT_EQ(SafeArrayGetDim(a.get()), 1);
    ASSERT_EQ(a.use_count(), 1);
}

TEST_F(TestSafeArrayShared, PutReplace)
{
    shared_safearray a(SafeArrayCreateVector(VT_I4, 0, 10), SafeArrayDeleter());
    ASSERT_TRUE(a);
    ASSERT_EQ(SafeArrayGetVartype(a.get()), VT_I4);
    ASSERT_EQ(a.use_count(), 1);

    LPSAFEARRAY tmp = nullptr;
    ASSERT_HRESULT_SUCCEEDED(CreateSafeArray(VT_UI4, &tmp));

    a.reset(std::move(tmp), SafeArrayDeleter());

    ASSERT_TRUE(a);
    ASSERT_EQ(SafeArrayGetVartype(a.get()), VT_UI4);
    ASSERT_EQ(a.use_count(), 1);
}

TEST_F(TestSafeArrayShared, Copy)
{
    shared_safearray a(SafeArrayCreateVector(VT_I4, 0, 10), SafeArrayDeleter());
    ASSERT_TRUE(a);
    ASSERT_EQ(SafeArrayGetVartype(a.get()), VT_I4);
    ASSERT_EQ(a.use_count(), 1);

    // This demonstrates how to copy a SAFEARRAY into a new unique_safearray
    LPSAFEARRAY psa;
    ASSERT_HRESULT_SUCCEEDED(SafeArrayCopy(a.get(), &psa));

    shared_safearray b(std::move(psa), SafeArrayDeleter());
    ASSERT_TRUE(b);
    ASSERT_EQ(b.use_count(), 1);

    // Pointers should not be equal
    ASSERT_NE(a, b);

    // Internal data pointers should not be equal
    ASSERT_NE(a->pvData, b->pvData);

    // VARTYPEs should be equal
    ASSERT_EQ(SafeArrayGetVartype(b.get()), VT_I4);
    ASSERT_EQ(SafeArrayGetVartype(a.get()), SafeArrayGetVartype(b.get()));

    b.reset(SafeArrayCreateVector(VT_UI4, 0, 10), SafeArrayDeleter());
    ASSERT_TRUE(b);
    ASSERT_EQ(b.use_count(), 1);

    // Now, the VARTYPES should not be equal
    ASSERT_EQ(SafeArrayGetVartype(b.get()), VT_UI4);
    ASSERT_NE(SafeArrayGetVartype(a.get()), SafeArrayGetVartype(b.get()));
}

///////////////////////////////////////////////////////////////////////////////
//
// SafeArrayDeathTest: Verify termination by destructor if SAFEARRAY is locked
//

TEST(SafeArrayDeathTest, Unique)
{
    auto f = []()
        {
            std::unique_ptr<LPSAFEARRAY, SafeArrayDeleter> a(
                SafeArrayCreateVector(VT_I4, 0, 10));
            (void)SafeArrayLock(a.get());
        };

    ASSERT_DEATH(f(), "");
}

TEST(SafeArrayDeathTest, Shared)
{
    auto f = []()
        {
            std::shared_ptr<std::remove_pointer_t<LPSAFEARRAY>> a(
                SafeArrayCreateVector(VT_I4, 0, 10), SafeArrayDeleter());
            (void)SafeArrayLock(a.get());
        };

    ASSERT_DEATH(f(), "");
}

///////////////////////////////////////////////////////////////////////////////
