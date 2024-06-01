# CMakeLists.txt ##############################################################
#
# commem: COM Memory Management
#
# commem is released under the MIT license.
#
# Copyright 2024 Jeffrey M. Engelmann
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

cmake_minimum_required(VERSION 3.28)
project(commem LANGUAGES CXX VERSION 1.0.0.0)
set(CMAKE_CXX_STANDARD 17)

option(COMMEM_TESTS OFF)
if(COMMEM_TESTS)
    enable_testing()
    set(INSTALL_GTEST OFF CACHE BOOL "Install GoogleTest")
    set(gtest_disable_pthreads ON CACHE BOOL "Do not use pthreads for GoogleTest")
    add_subdirectory(extern/googletest)
    include(GoogleTest)

    # Build test_commem.exe
    add_executable(test_commem
        "test/test_commem.cpp"
        "test/test_heap.cpp"
        "test/test_bstr.cpp"
        "test/test_safearray.cpp")
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(test_commem PRIVATE /W4 /WX)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(test_commem PRIVATE -Wall -Wextra -Wpedantic -Werror)
        if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
            target_compile_options(test_commem PRIVATE -O3)
            target_compile_definitions(test_commem PRIVATE NDEBUG)
        endif()
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(test_commem PRIVATE -Wall -Wextra -Wpedantic -Werror
            -Wno-c++98-compat
            -Wno-c++98-compat-pedantic
            -Wno-global-constructors)
    endif()
    target_include_directories(test_commem PRIVATE "include")
    target_link_libraries(test_commem PRIVATE gtest gmock)

    # Discover and run tests
    gtest_discover_tests(test_commem)
endif()

###############################################################################
