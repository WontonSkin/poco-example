
cmake_minimum_required(VERSION 3.0)
project(JSON-testrunner)

set(PROJECT_ROOT_DIR /root/shareDir/poco-sample)

include(${PROJECT_ROOT_DIR}/cmake/PocoMacros.cmake)

# Sources
file(GLOB SRCS_G "src/*.cpp")
POCO_SOURCES_AUTO(TEST_SRCS ${SRCS_G})

# Headers
file(GLOB_RECURSE HDRS_G "src/*.h")
POCO_HEADERS_AUTO(TEST_SRCS ${HDRS_G})

add_executable(JSON-testrunner ${TEST_SRCS})

target_link_libraries(JSON-testrunner PocoJSONd CppUnitd PocoFoundationd)


