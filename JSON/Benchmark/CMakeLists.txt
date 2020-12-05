



cmake_minimum_required(VERSION 3.0)

project(Benchmark)

#set(CMAKE_CXX_FLAGS   "-std=c++11")             # c++11
#set(CMAKE_CXX_FLAGS   "-g")                     # 调试信息
#set(CMAKE_CXX_FLAGS   "-Wall")                  # 开启所有警告
#set(CMAKE_CXX_FLAGS_DEBUG   "-O0" )             # 调试包不优化
#set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG " )   # release包优化

#include_directories(${CMAKE_CURRENT_LIST_DIR}/include)

#link_directories(${CMAKE_CURRENT_LIST_DIR}/lib)

aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/src TEST_SRCS)

#add_executable(Benchmark src/Benchmark.cpp)
add_executable(${PROJECT_NAME} ${TEST_SRCS})

#target_link_libraries(Benchmark PocoJSONd PocoFoundationd)
target_link_libraries(${PROJECT_NAME} PocoJSONd PocoFoundationd)


