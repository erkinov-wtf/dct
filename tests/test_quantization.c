/**
 * test_quantization.c - Test file for Quantization implementation
 * Part of Adaptive DCT Image Compressor
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <utils.h>
#include "../include/quantization.h"

// Test helper function to print a matrix
void print_matrix(const char* name, double **matrix, int size) {
    printf("%s matrix (%dx%d):\n", name, size, size);
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            printf("%8.2f ", matrix[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}

// Test helper function to print int matrix
void print_int_matrix(const char* name, int **matrix, int size) {
    printf("%s matrix (%dx%d):\n", name, size, size);
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            printf("%4d ", matrix[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}

// Test quantization matrix generation
void test_quant_matrix_generation(void) {
    printf("=== Testing Quantization Matrix Generation ===\n");

    int block_size = 8;

    // Test with different quality values
    int qualities[] = {10, 50, 90};

    for (int i = 0; i < 3; i++) {
        int quality = qualities[i];
        printf("Testing with quality = %d\n", quality);

        double **quant_matrix = generate_quant_matrix(block_size, quality);
        print_matrix("Quantization", quant_matrix, block_size);

        double **dequant_matrix = generate_dequant_matrix(quant_matrix, block_size);
        print_matrix("Dequantization", dequant_matrix, block_size);

        free_array(quant_matrix, block_size);
        free_array(dequant_matrix, block_size);
    }
}

// Test basic quantization and dequantization
void test_basic_quantization(void) {
    printf("\n=== Testing Basic Quantization/Dequantization ===\n");

    int block_size = 8;
    int quality = 50;

    // Create sample DCT coefficients
    double **dct_coeffs = alloc_array(block_size, block_size);
    // Fill with some sample data
    for (int i = 0; i < block_size; i++) {
        for (int j = 0; j < block_size; j++) {
            // Create a gradient pattern
            dct_coeffs[i][j] = 100.0 * exp(-(i*i + j*j) / 10.0);
        }
    }

    print_matrix("Original DCT", dct_coeffs, block_size);

    // Initialize quantization context
    QuantContext *ctx = quant_init(block_size, quality, 0); // No adaptive quantization

    // Allocate memory for quantized coefficients
    int **quant_coeffs = alloc_int_array(block_size, block_size);

    // Perform quantization
    quantize(ctx, dct_coeffs, quant_coeffs, 0.0);
    print_int_matrix("Quantized", quant_coeffs, block_size);

    // Allocate memory for dequantized coefficients
    double **dequant_coeffs = alloc_array(block_size, block_size);

    // Perform dequantization
    dequantize(ctx, quant_coeffs, dequant_coeffs, 0.0);
    print_matrix("Dequantized", dequant_coeffs, block_size);

    // Calculate error
    double error = 0.0;
    for (int i = 0; i < block_size; i++) {
        for (int j = 0; j < block_size; j++) {
            error += fabs(dct_coeffs[i][j] - dequant_coeffs[i][j]);
        }
    }
    printf("Total absolute error: %.2f\n", error);
    printf("Average absolute error: %.2f\n", error / (block_size * block_size));

    // Cleanup
    free_array(dct_coeffs, block_size);
    free_array(dequant_coeffs, block_size);
    free_int_array(quant_coeffs, block_size);
    quant_free(ctx);
}

// Test adaptive quantization
void test_adaptive_quantization(void) {
    printf("\n=== Testing Adaptive Quantization ===\n");

    int block_size = 8;
    int quality = 50;

    // Create two sample blocks with different variances
    double **flat_block = alloc_array(block_size, block_size);
    double **detailed_block = alloc_array(block_size, block_size);

    // Flat block - low variance
    for (int i = 0; i < block_size; i++) {
        for (int j = 0; j < block_size; j++) {
            flat_block[i][j] = 100.0 + (rand() % 10);
        }
    }

    // Detailed block - high variance
    for (int i = 0; i < block_size; i++) {
        for (int j = 0; j < block_size; j++) {
            detailed_block[i][j] = 100.0 + (rand() % 100);
        }
    }

    double flat_variance = calculate_block_variance(flat_block, block_size);
    double detailed_variance = calculate_block_variance(detailed_block, block_size);

    printf("Flat block variance: %.2f\n", flat_variance);
    printf("Detailed block variance: %.2f\n", detailed_variance);

    // Initialize quantization context with adaptive mode
    QuantContext *ctx = quant_init(block_size, quality, 1);

    // Allocate memory for quantized coefficients
    int **quant_flat = alloc_int_array(block_size, block_size);
    int **quant_detailed = alloc_int_array(block_size, block_size);

    // Perform quantization
    quantize(ctx, flat_block, quant_flat, flat_variance);
    quantize(ctx, detailed_block, quant_detailed, detailed_variance);

    // Allocate memory for dequantized coefficients
    double **dequant_flat = alloc_array(block_size, block_size);
    double **dequant_detailed = alloc_array(block_size, block_size);

    // Perform dequantization
    dequantize(ctx, quant_flat, dequant_flat, flat_variance);
    dequantize(ctx, quant_detailed, dequant_detailed, detailed_variance);

    // Calculate errors
    double flat_error = 0.0;
    double detailed_error = 0.0;

    for (int i = 0; i < block_size; i++) {
        for (int j = 0; j < block_size; j++) {
            flat_error += fabs(flat_block[i][j] - dequant_flat[i][j]);
            detailed_error += fabs(detailed_block[i][j] - dequant_detailed[i][j]);
        }
    }

    printf("Flat block total error: %.2f\n", flat_error);
    printf("Flat block average error: %.2f\n", flat_error / (block_size * block_size));
    printf("Detailed block total error: %.2f\n", detailed_error);
    printf("Detailed block average error: %.2f\n", detailed_error / (block_size * block_size));

    // We expect the detailed block to have less error because adaptive quantization
    // should preserve more detail in high-variance areas
    printf("Is detailed block error less than flat block error? %s\n",
           (detailed_error < flat_error) ? "Yes (Good!)" : "No (Something might be wrong)");

    // Cleanup
    free_array(flat_block, block_size);
    free_array(detailed_block, block_size);
    free_array(dequant_flat, block_size);
    free_array(dequant_detailed, block_size);
    free_int_array(quant_flat, block_size);
    free_int_array(quant_detailed, block_size);
    quant_free(ctx);
}

// Main test function
int main(void) {
    printf("Running quantization tests...\n\n");

    test_quant_matrix_generation();
    test_basic_quantization();
    test_adaptive_quantization();

    printf("\nAll tests completed.\n");
    return 0;
}

