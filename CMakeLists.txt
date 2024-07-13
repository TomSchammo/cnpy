cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
project(cnpy VERSION 0.0.1 LANGUAGES CXX)

if (NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 20)
endif ()

option(BUILD_TESTS "Build tests" OFF)

find_package(ZLIB REQUIRED)

add_library(cnpy src/cnpy.cpp)
target_include_directories(cnpy
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_link_libraries(cnpy PUBLIC ZLIB::ZLIB)

if (BUILD_TESTS)
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
endif ()

if (NOT BUILD_TESTS)
    include(GNUInstallDirs)

    install(TARGETS cnpy
            EXPORT cnpyTargets
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
    install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

    install(EXPORT cnpyTargets
            FILE cnpyTargets.cmake
            NAMESPACE cnpy::
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cnpy
    )

    include(CMakePackageConfigHelpers)

    write_basic_package_version_file(
            "${CMAKE_CURRENT_BINARY_DIR}/cnpyConfigVersion.cmake"
            VERSION ${PROJECT_VERSION}
            COMPATIBILITY SameMajorVersion
    )

    configure_file(cmake/cnpyConfig.cmake.in
            "${CMAKE_CURRENT_BINARY_DIR}/cnpyConfig.cmake"
            @ONLY
    )

    install(FILES
            "${CMAKE_CURRENT_BINARY_DIR}/cnpyConfig.cmake"
            "${CMAKE_CURRENT_BINARY_DIR}/cnpyConfigVersion.cmake"
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cnpy
    )
endif ()
