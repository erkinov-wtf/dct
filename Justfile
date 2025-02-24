# justfile for Adaptive DCT Image Compressor

# Default recipe
default: build

# Set build variables
CC := "gcc"
CFLAGS := "-Wall -Wextra -Werror -pedantic -std=c99 -Iinclude"
LDFLAGS := "-lm"

SRC_DIR := "src"
INCLUDE_DIR := "include"
TEST_DIR := "tests"
BUILD_DIR := "build"

# Create directories
dirs:
    mkdir -p {{BUILD_DIR}}

# Build objects
build-dct: dirs
    {{CC}} {{CFLAGS}} -c {{SRC_DIR}}/dct.c -o {{BUILD_DIR}}/dct.o
    {{CC}} {{CFLAGS}} -c {{SRC_DIR}}/quantization.c -o {{BUILD_DIR}}/quantization.o

# Build test executables
build-test-dct: build-dct
    {{CC}} {{CFLAGS}} {{BUILD_DIR}}/dct.o {{TEST_DIR}}/test_dct.c -o {{BUILD_DIR}}/test_dct {{LDFLAGS}}
    {{CC}} {{CFLAGS}} {{BUILD_DIR}}/quantization.o {{TEST_DIR}}/test_quantization.c -o {{BUILD_DIR}}/test_quantization {{LDFLAGS}}

# Build all targets
build: build-test-dct

# Run all tests
test: build
    @echo "Running DCT tests..."
    {{BUILD_DIR}}/test_dct
    {{BUILD_DIR}}/test_quantization

# Clean build files
clean:
    rm -rf {{BUILD_DIR}}