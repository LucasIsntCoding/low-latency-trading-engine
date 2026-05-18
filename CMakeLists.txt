cmake_minimum_required(VERSION 3.20)
project(low_latency_trading_engine_simulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

add_compile_options(
    -Wall -Wextra -Wpedantic
    $<$<CONFIG:Release>:-O3>
    $<$<CONFIG:Release>:-march=native>
    $<$<CONFIG:Release>:-DNDEBUG>
)

include_directories(include)

add_executable(llt_engine src/main.cpp)
target_link_libraries(llt_engine PRIVATE pthread)

add_executable(test_order_book tests/test_order_book.cpp)
target_link_libraries(test_order_book PRIVATE pthread)

enable_testing()
add_test(NAME order_book COMMAND test_order_book)
