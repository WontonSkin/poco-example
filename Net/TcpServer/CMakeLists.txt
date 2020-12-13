
cmake_minimum_required(VERSION 3.0)

project(TcpServer)


#当 CMAKE_BUILD_TYPE 取值为Debug时，CMake会使用变量 CMAKE_CXX_FLAGS_DEBUG 中的选项作为编译选项来生成Makefile
#SET(CMAKE_BUILD_TYPE "Debug")
#SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")
#SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

#set(CMAKE_CXX_FLAGS   "-std=c++11")             # c++11
#set(CMAKE_CXX_FLAGS   "-g")                     # 调试信息
#set(CMAKE_CXX_FLAGS   "-Wall")                  # 开启所有警告
#set(CMAKE_CXX_FLAGS_DEBUG   "-O0" )             # 调试包不优化
#set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG " )   # release包优化

#set(PROJECT_ROOT_DIR /root/shareDir/poco-sample)
#include(${PROJECT_ROOT_DIR}/cmake/PocoMacros.cmake)

#include_directories(${CMAKE_CURRENT_LIST_DIR}/include)
#link_directories(${CMAKE_CURRENT_LIST_DIR}/lib)

aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/src TEST_SRCS)

add_executable(${PROJECT_NAME} ${TEST_SRCS})

target_link_libraries(${PROJECT_NAME} PUBLIC PocoNetd PocoUtild PocoJSONd PocoFoundationd)
