# justfile for Adaptive DCT Image Compressor

# Default recipe
default: build

# Set build variables
CC := "gcc"
CFLAGS := "-Wall -Wextra -Werror -pedantic -std=c99 -Iinclude -g"
LDFLAGS := "-lm"

SRC_DIR := "src"
INCLUDE_DIR := "include"
TEST_DIR := "tests"
BUILD_DIR := "build"

# Create directories
dirs:
    mkdir -p {{BUILD_DIR}}

# Build objects
build-util: dirs
    {{CC}} {{CFLAGS}} -c {{SRC_DIR}}/utils.c -o {{BUILD_DIR}}/util.o

# Build other objects
build-dct: build-util
    {{CC}} {{CFLAGS}} -c {{SRC_DIR}}/dct.c -o {{BUILD_DIR}}/dct.o
    {{CC}} {{CFLAGS}} -c {{SRC_DIR}}/quantization.c -o {{BUILD_DIR}}/quantization.o
    {{CC}} {{CFLAGS}} -c {{SRC_DIR}}/entropy.c -o {{BUILD_DIR}}/entropy.o

# Build test executables
build-test-dct: build-dct
    {{CC}} {{CFLAGS}} {{BUILD_DIR}}/dct.o {{BUILD_DIR}}/util.o {{TEST_DIR}}/test_dct.c -o {{BUILD_DIR}}/test_dct {{LDFLAGS}}
    {{CC}} {{CFLAGS}} {{BUILD_DIR}}/quantization.o {{BUILD_DIR}}/dct.o {{BUILD_DIR}}/util.o {{TEST_DIR}}/test_quantization.c -o {{BUILD_DIR}}/test_quantization {{LDFLAGS}}
    {{CC}} {{CFLAGS}} {{BUILD_DIR}}/entropy.o {{BUILD_DIR}}/dct.o {{BUILD_DIR}}/quantization.o {{BUILD_DIR}}/util.o {{TEST_DIR}}/test_entropy.c -o {{BUILD_DIR}}/test_entropy {{LDFLAGS}}


# Build all targets
build: build-test-dct

# Run all tests
test: build
    @echo "Running DCT tests..."
    {{BUILD_DIR}}/test_dct
    {{BUILD_DIR}}/test_quantization
    {{BUILD_DIR}}/test_entropy

# Clean build files
clean:
    rm -rf {{BUILD_DIR}}
    rm -rf cmake-build-debug
    clear