cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(psrs CXX)

# 设置C++标准为C++11
set(CMAKE_CXX_STANDARD 11)

# 查找MPI库
find_package(MPI REQUIRED)

# 添加头文件路径
include_directories(${CMAKE_SOURCE_DIR}/include)

# 添加所有源文件
file(GLOB SOURCES "${CMAKE_SOURCE_DIR}/src/*.cpp")
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/executable)
# 生成可执行文件
add_executable(psrs ${SOURCES})

# 包含MPI头文件
include_directories(${MPI_CXX_INCLUDE_PATH})

# 链接MPI库
target_link_libraries(psrs ${MPI_CXX_LIBRARIES})


