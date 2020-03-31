cmake_minimum_required(VERSION 3.1)
project(Papr)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(/usr/include/readline)
add_subdirectory(libs/yaml-cpp)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
# set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")
# set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall")

add_executable(
    papr
    papr.cpp
)

target_link_libraries(
    papr
    PRIVATE
    readline
    yaml-cpp
)