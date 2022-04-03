cmake_minimum_required(VERSION 3.2)

if (NOT DEFINED ENV{TRAVIS_BUILD_NUMBER})
    set(ENV{TRAVIS_BUILD_NUMBER} 42)
endif ()

project(hw11_mapreduce VERSION 0.0.$ENV{TRAVIS_BUILD_NUMBER})

# Build googletest
exec_program(git ARGS submodule update --init --recursive)
get_filename_component(GTEST_SOURCE ../googletest ABSOLUTE)
add_subdirectory(
        "${GTEST_SOURCE}"
        "googletest"
)

# Boost
find_package(Boost COMPONENTS system thread REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

# Targets
add_executable(
        mapreduce main.cpp
        prefix.h prefix.cpp
        mapreduce.h mapreduce.cpp
)

add_executable(
        test_mapreduce
        tests/test_mapreduce.cpp
        prefix.h prefix.cpp
        mapreduce.h mapreduce.cpp
)

get_filename_component(GTEST_FIXTURE ./tests ABSOLUTE)

target_include_directories(test_mapreduce PRIVATE .)

set_target_properties(mapreduce test_mapreduce PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        )

target_link_libraries(mapreduce
        ${Boost_SYSTEM_LIBRARY}
        ${Boost_THREAD_LIBRARY}
        ${CMAKE_THREAD_LIBS_INIT}
        )
target_link_libraries(test_mapreduce PRIVATE
        gtest gtest_main
        )

if (MSVC)
    target_compile_options(
            mapreduce PRIVATE
            /W4
    )
    target_compile_options(
            test_mapreduce PRIVATE
            /W4
    )
else ()
    target_compile_options(
            mapreduce PRIVATE
            -Wall -Wextra -pedantic -Werror -Wno-deprecated
    )
    target_compile_options(
            test_mapreduce PRIVATE
            -Wall -Wextra -pedantic -Werror -Wno-deprecated
    )
endif ()


install(TARGETS mapreduce RUNTIME DESTINATION bin)

set(CPACK_GENERATOR DEB)

set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")

set(CPACK_PACKAGE_CONTACT j@vladnf.ru)

include(CPack)

enable_testing()

add_test(NAME test_mapreduce COMMAND test_mapreduce WORKING_DIRECTORY ${GTEST_FIXTURE})
