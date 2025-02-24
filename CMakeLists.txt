cmake_minimum_required(VERSION 3.29)
project(AdaptiveDCT C)

set(CMAKE_C_STANDARD 23)

include_directories(include)

add_executable(
        AdaptiveDCT

        src/dct.c
        src/quantization.c

        tests/test_dct.c
        tests/test_quantization.c
)
