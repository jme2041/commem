// test_commem.cpp: Driver for commem tests ///////////////////////////////////
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
#include <unordered_map>

static DWORD s_dwMainThreadId = 0;

///////////////////////////////////////////////////////////////////////////////
//
// Malloc spy
//

class Spy : public IMallocSpy {

    // Map base addresses to size of the memory block
    std::unordered_map<void*, size_t> m_map;

    // Total number of bytes allocated
    size_t m_cbTotal = 0;

    // Store request parameters between the calls to Pre and Post.
    // COM guarantees that the process from Pre through Post is thread safe.
    void* m_pRequest = nullptr;
    size_t m_cbRequest = 0;

    // Register a block of memory by adding it to the map
    // Let exceptions propagate, which will fail the test
    void RegisterMemory(void* const p, size_t const cb)
    {
        m_map.insert({ p, cb });
        m_cbTotal += cb;
    }

    // Unregister a block of memory by removing it from the map
    // Let exceptions propagate, which will fail the test
    void UnregisterMemory(void* const p)
    {
        auto const i = m_map.find(p);
        if (i != m_map.end())
        {
            m_cbTotal -= i->second;
            m_map.erase(i);
        }
    }

public:
    Spy(Spy const&) = delete;
    Spy(Spy&&) = delete;
    Spy& operator=(Spy const&) = delete;
    Spy& operator=(Spy&&) = delete;

    Spy()
    {
        // Disable the BSTR cache so double SysFreeString crashes the tests
        auto const dll = LoadLibraryW(L"oleaut32.dll");
        if (dll)
        {
            auto const f = static_cast<FARPROC>(GetProcAddress(dll, "SetOaNoCache"));
            if (!f) throw std::runtime_error("GetProcAddress failed");
            f();
            if (!FreeLibrary(dll)) throw std::runtime_error("FreeLibrary failed");
        }

        // Register this object as the malloc spy
        if (FAILED(CoRegisterMallocSpy(this)))
            throw std::runtime_error("CoRegisterMallocSpy failed");
    }

    virtual ~Spy() noexcept
    {
        if (FAILED(CoRevokeMallocSpy())) std::terminate();
    }

    size_t NBlocks() const noexcept { return m_map.size(); }
    size_t NBytes() const noexcept { return m_cbTotal; }

    STDMETHODIMP QueryInterface(REFIID riid, void** const ppv) noexcept override
    {
        if (!ppv) return E_POINTER;
        if (riid == IID_IUnknown) *ppv = static_cast<IUnknown*>(this);
        else if (riid == IID_IMallocSpy) *ppv = static_cast<IMallocSpy*>(this);
        else return static_cast<void>(*ppv = nullptr), E_NOINTERFACE;
        reinterpret_cast<IUnknown*>(this)->AddRef();
        return S_OK;
    }

    STDMETHODIMP_(ULONG) AddRef() noexcept override
    {
        return 2;
    }

    STDMETHODIMP_(ULONG) Release() noexcept override
    {
        return 1;
    }

    STDMETHODIMP_(SIZE_T) PreAlloc(SIZE_T const cbRequest) noexcept override
    {
        m_cbRequest = cbRequest;
        return cbRequest;
    }

    STDMETHODIMP_(void*) PostAlloc(void* const pActual) override
    {
        if (pActual) RegisterMemory(pActual, m_cbRequest);
        return pActual;
    }

    STDMETHODIMP_(void*) PreFree(
        void* const pRequest,
        BOOL const fSpyed) noexcept override
    {
        if (fSpyed) m_pRequest = pRequest;
        return pRequest;
    }

    STDMETHODIMP_(void) PostFree(BOOL const fSpyed) override
    {
        if (fSpyed) UnregisterMemory(m_pRequest);
    }

    STDMETHODIMP_(SIZE_T) PreRealloc(
        void* const pRequest,
        SIZE_T const cbRequest,
        void** const ppNewRequest,
        BOOL const fSpyed) noexcept override
    {
        assert(ppNewRequest);
        if (fSpyed)
        {
            m_pRequest = pRequest;
            m_cbRequest = cbRequest;
        }
        *ppNewRequest = pRequest;
        return cbRequest;
    }

    STDMETHODIMP_(void*) PostRealloc(
        void* const pActual,
        BOOL const fSpyed) override
    {
        if (fSpyed)
        {
            if (m_cbRequest == 0 || pActual) UnregisterMemory(m_pRequest);
            if (pActual) RegisterMemory(pActual, m_cbRequest);
        }
        return pActual;
    }

    STDMETHODIMP_(void*) PreGetSize(void* const pRequest, BOOL) noexcept override
    {
        return pRequest;
    }

    STDMETHODIMP_(SIZE_T) PostGetSize(SIZE_T const cbActual, BOOL) noexcept override
    {
        return cbActual;
    }

    STDMETHODIMP_(void*) PreDidAlloc(void* const pRequest, BOOL) noexcept override
    {
        return pRequest;
    }

    STDMETHODIMP_(int) PostDidAlloc(void*, BOOL, int const fActual) noexcept override
    {
        return fActual;
    }

    STDMETHODIMP_(void) PreHeapMinimize() noexcept override {}

    STDMETHODIMP_(void) PostHeapMinimize() noexcept override {}
};

///////////////////////////////////////////////////////////////////////////////
//
// TestCommem methods: These check each test for memory leaks
//
// It is safe to store the number of blocks and number of bytes at the start of
// a test as instance member variables because the tests are serialized.
//

static Spy* s_pSpy = nullptr;

void TestCommem::SetUp() noexcept
{
    assert(s_pSpy);

    // All tests must start from the main thread
    ASSERT_EQ(s_dwMainThreadId, GetCurrentThreadId());

    m_startBlocks = s_pSpy->NBlocks();
    m_startBytes = s_pSpy->NBytes();
}

void TestCommem::TearDown() noexcept
{
    assert(s_pSpy);
    EXPECT_EQ(s_pSpy->NBlocks(), m_startBlocks);
    EXPECT_EQ(s_pSpy->NBytes(), m_startBytes);
}

///////////////////////////////////////////////////////////////////////////////
//
// ComInitializer: CoInitializeEx and CoUninitialize wrapper
//

struct ComInitializer {

    ComInitializer()
    {
        if (FAILED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED)))
            throw std::runtime_error("CoInitializeEx failed");
    }

    ~ComInitializer()
    {
        CoUninitialize();
    }

    ComInitializer(ComInitializer const&) = delete;
    ComInitializer(ComInitializer&&) = delete;
    ComInitializer& operator=(ComInitializer const&) = delete;
    ComInitializer& operator=(ComInitializer&&) = delete;
};

///////////////////////////////////////////////////////////////////////////////
//
// main: Application entry point
//

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);

    // If listing tests, do so and leave before calling any COM functions
    if (::testing::FLAGS_gtest_list_tests) return RUN_ALL_TESTS();

    try
    {
        Spy spy;
        s_pSpy = &spy;

        ComInitializer com;

        // Store the STA thread's ID before running tests
        // This doesn't need a lock because it is written here, prior to the
        // start of the tests from which it is read.

        s_dwMainThreadId = GetCurrentThreadId();

        // Allocate and free a BSTR
        // The first BSTR allocation sets up some internal data in COM
        // (typically, 440 bytes). Do it here so that it doesn't interfere with
        // per-test memory accounting during the BSTR tests.

        SysFreeString(SysAllocString(L"1234"));

        return RUN_ALL_TESTS();
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}

///////////////////////////////////////////////////////////////////////////////
