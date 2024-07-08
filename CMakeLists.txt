cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
project(cnpy VERSION 0.0.1 LANGUAGES CXX)

if (NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 20)
endif ()

option(BUILD_TESTS "Build tests" OFF)

find_package(ZLIB REQUIRED)

include_directories(${ZLIB_INCLUDE_DIRS})

add_library(cnpy SHARED "src/cnpy.cpp")
target_link_libraries(cnpy ${ZLIB_LIBRARIES})

if(BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
    )
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    add_executable(test test/test.cpp)
    target_link_libraries(test cnpy GTest::gtest_main)

    include(GoogleTest)
    gtest_discover_tests(test)
endif()

if(NOT TARGET cnpy)
    add_library(cnpy src/cnpy.cpp include/cnpy.hpp)
    target_include_directories(cnpy PUBLIC include)
endif()





