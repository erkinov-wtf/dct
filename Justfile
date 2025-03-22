# justfile for Adaptive DCT Image Compressor

# Default recipe: show help
default:
    @just --list

# Build variables
CC := "gcc"
CFLAGS := " -pedantic -std=c99 -Iinclude -g"
LDFLAGS := "-lm"

# Directory structure
SRC_DIR := "src"
INCLUDE_DIR := "include"
TEST_DIR := "tests"
BUILD_DIR := "build"
OBJ_DIR := "build/obj"

# Component list
COMPONENTS := "utils dct quantization entropy image"

# Create directories
setup:
    @mkdir -p {{BUILD_DIR}}
    @mkdir -p {{OBJ_DIR}}

# Build utilities (required by all components)
utils: setup
    @echo "Building utils..."
    @{{CC}} {{CFLAGS}} -c {{SRC_DIR}}/utils.c -o {{OBJ_DIR}}/utils.o

# Build individual components
dct: utils
    @echo "Building DCT component..."
    @{{CC}} {{CFLAGS}} -c {{SRC_DIR}}/dct.c -o {{OBJ_DIR}}/dct.o

quantization: utils
    @echo "Building Quantization component..."
    @{{CC}} {{CFLAGS}} -c {{SRC_DIR}}/quantization.c -o {{OBJ_DIR}}/quantization.o

entropy: utils
    @echo "Building Entropy component..."
    @{{CC}} {{CFLAGS}} -c {{SRC_DIR}}/entropy.c -o {{OBJ_DIR}}/entropy.o

image: utils
    @echo "Building Image component..."
    @{{CC}} {{CFLAGS}} -c {{SRC_DIR}}/image.c -o {{OBJ_DIR}}/image.o

# Build all components
build-components: utils dct quantization entropy image
    @echo "All components built successfully"

# Build test executables
test-dct: dct
    @echo "Building DCT test..."
    @{{CC}} {{CFLAGS}} {{OBJ_DIR}}/dct.o {{OBJ_DIR}}/utils.o {{TEST_DIR}}/test_dct.c -o {{BUILD_DIR}}/test_dct {{LDFLAGS}}

test-quantization: dct quantization
    @echo "Building Quantization test..."
    @{{CC}} {{CFLAGS}} {{OBJ_DIR}}/quantization.o {{OBJ_DIR}}/dct.o {{OBJ_DIR}}/utils.o {{TEST_DIR}}/test_quantization.c -o {{BUILD_DIR}}/test_quantization {{LDFLAGS}}

test-entropy: dct quantization entropy
    @echo "Building Entropy test..."
    @{{CC}} {{CFLAGS}} {{OBJ_DIR}}/entropy.o {{OBJ_DIR}}/dct.o {{OBJ_DIR}}/quantization.o {{OBJ_DIR}}/utils.o {{TEST_DIR}}/test_entropy.c -o {{BUILD_DIR}}/test_entropy {{LDFLAGS}}

test-image: image
    @echo "Building Image test..."
    @{{CC}} {{CFLAGS}} {{OBJ_DIR}}/image.o {{OBJ_DIR}}/utils.o {{TEST_DIR}}/test_image.c -o {{BUILD_DIR}}/test_image {{LDFLAGS}}

# Build all test executables
build-tests: test-dct test-quantization test-entropy test-image
    @echo "All test executables built successfully"

# Build everything
build: build-components build-tests
    @echo "Build complete"

# Run specific tests
run-test-dct: test-dct
    @echo "Running DCT tests..."
    @{{BUILD_DIR}}/test_dct

run-test-quantization: test-quantization
    @echo "Running Quantization tests..."
    @{{BUILD_DIR}}/test_quantization

run-test-entropy: test-entropy
    @echo "Running Entropy tests..."
    @{{BUILD_DIR}}/test_entropy

run-test-image: test-image
    @echo "Running Image tests..."
    @{{BUILD_DIR}}/test_image

# Run all tests
test: build-tests
    @echo "=== Running all tests ==="
    @echo "--- DCT Tests ---"
    @{{BUILD_DIR}}/test_dct
    @echo "--- Quantization Tests ---"
    @{{BUILD_DIR}}/test_quantization
    @echo "--- Entropy Tests ---"
    @{{BUILD_DIR}}/test_entropy
    @echo "--- Image Tests ---"
    @{{BUILD_DIR}}/test_image
    @echo "All tests completed!"

# Build the main executable
main: build-components
    @echo "Building main executable..."
    @{{CC}} {{CFLAGS}} $(find {{OBJ_DIR}} -name "*.o") {{SRC_DIR}}/main.c -o {{BUILD_DIR}}/adaptive_dct_compressor {{LDFLAGS}}
    @echo "Executable built: {{BUILD_DIR}}/adaptive_dct_compressor"

# Clean build files
clean:
    @echo "Cleaning build directory..."
    @rm -rf {{BUILD_DIR}}
    @rm -rf cmake-build-debug
    @echo "Clean complete"
    clear