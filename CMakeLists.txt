cmake_minimum_required(VERSION 3.29)
project(AdaptiveDCT C)

set(CMAKE_C_STANDARD 23)

include_directories(include)

add_executable(
        AdaptiveDCT

        src/utils.c
        src/dct.c
        src/quantization.c
        src/entropy.c
        src/image.c

        tests/test_dct.c
        tests/test_quantization.c
        tests/test_entropy.c
        tests/test_image.c
)
