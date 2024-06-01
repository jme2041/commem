// test_commem.h: Base class for commem tests /////////////////////////////////
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
#ifndef TEST_COMMEM_H
#define TEST_COMMEM_H

#include "gtest/gtest.h"

class TestCommem : public ::testing::Test {
private:
    // Store the number of memory blocks and bytes at the beginning of each
    // test using instance variables. This is safe because the tests are
    // serialized (all tests are run from a single thread).

    size_t m_startBlocks = 0;
    size_t m_startBytes = 0;

protected:
    void SetUp() noexcept override;
    void TearDown() noexcept override;
};

#endif  // TEST_COMMEM_H

///////////////////////////////////////////////////////////////////////////////
