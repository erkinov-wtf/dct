/**
 * test_entropy.c - Test file for Entropy Coding implementation
 * Part of Adaptive DCT Image Compressor
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <utils.h>
#include "../include/entropy.h"
#include "../include/dct.h"
#include "../include/quantization.h"

// Test helper function to print RLE symbols
void print_rle_symbols(EntropyContext *ctx) {
    printf("RLE Symbols (%d total):\n", ctx->count);
    printf("%-8s %-8s\n", "Value", "Run Length");
    printf("-----------------\n");
    for (int i = 0; i < ctx->count; i++) {
        printf("%-8d %-8d\n", ctx->symbols[i].value, ctx->symbols[i].run_length);
    }
    printf("\n");
}

// Test helper function to print Huffman codes
void print_huffman_codes(EntropyContext *ctx) {
    printf("Huffman Codes (%d total):\n", ctx->huffman_size);
    printf("%-8s %-15s\n", "Symbol", "Code");
    printf("-----------------------\n");
    for (int i = 0; i < ctx->huffman_size; i++) {
        printf("%-8d %-15s\n", ctx->huffman_codes[i].symbol, ctx->huffman_codes[i].code);
    }
    printf("\n");
}

// Test zigzag scanning
void test_zigzag_scan(void) {
    printf("=== Testing Zigzag Scan ===\n");
    
    // Create a sample 8x8 block with sequential values
    int block_size = 8;
    int **block = alloc_int_array(block_size, block_size);
    int value = 1;
    for (int i = 0; i < block_size; i++) {
        for (int j = 0; j < block_size; j++) {
            block[i][j] = value++;
        }
    }
    
    // Print original block
    printf("Original block:\n");
    for (int i = 0; i < block_size; i++) {
        for (int j = 0; j < block_size; j++) {
            printf("%3d ", block[i][j]);
        }
        printf("\n");
    }
    printf("\n");
    
    // Convert to zigzag
    int size = block_size * block_size;
    int *zigzag = (int*)malloc(size * sizeof(int));
    block_to_zigzag(block, zigzag, block_size);
    
    // Print zigzag order
    printf("Zigzag scan order:\n");
    for (int i = 0; i < size; i++) {
        printf("%3d ", zigzag[i]);
        if ((i + 1) % block_size == 0) printf("\n");
    }
    printf("\n");
    
    // Convert back to block
    int **result = alloc_int_array(block_size, block_size);
    zigzag_to_block(zigzag, result, block_size);
    
    // Print result block
    printf("Reconstructed block:\n");
    for (int i = 0; i < block_size; i++) {
        for (int j = 0; j < block_size; j++) {
            printf("%3d ", result[i][j]);
        }
        printf("\n");
    }
    printf("\n");
    
    // Verify reconstruction
    int errors = 0;
    for (int i = 0; i < block_size; i++) {
        for (int j = 0; j < block_size; j++) {
            if (block[i][j] != result[i][j]) {
                errors++;
            }
        }
    }
    
    if (errors == 0) {
        printf("Zigzag scan test PASSED! Reconstructed block matches original.\n\n");
    } else {
        printf("Zigzag scan test FAILED! %d errors found.\n\n", errors);
    }
    
    // Clean up
    free_int_array(block, block_size);
    free_int_array(result, block_size);
    free(zigzag);
}

// Test run-length encoding
void test_run_length_encoding(void) {
    printf("=== Testing Run-Length Encoding ===\n");
    
    // Create a sample 8x8 block with mostly zeros
    int block_size = 8;
    int **block = alloc_int_array(block_size, block_size);
    
    // Initialize to zeros
    for (int i = 0; i < block_size; i++) {
        for (int j = 0; j < block_size; j++) {
            block[i][j] = 0;
        }
    }
    
    // Add some non-zero values in typical DCT coefficient pattern
    block[0][0] = 100;  // DC coefficient
    block[0][1] = 12;
    block[1][0] = 9;
    block[2][0] = -5;
    block[1][1] = 8;
    block[0][2] = 3;
    block[3][3] = -7;
    block[4][2] = 2;
    block[7][7] = 1;  // High frequency component
    
    // Print original block
    printf("Original block with sparse non-zero values:\n");
    for (int i = 0; i < block_size; i++) {
        for (int j = 0; j < block_size; j++) {
            printf("%4d ", block[i][j]);
        }
        printf("\n");
    }
    printf("\n");
    
    // Encode using RLE
    EntropyContext *ctx = entropy_init(0);  // No Huffman yet
    int symbol_count = run_length_encode(ctx, block, block_size);
    
    printf("Number of RLE symbols: %d (out of %d values)\n", 
           symbol_count, block_size * block_size);
    printf("Compression ratio: %.2f:1\n\n", 
           (float)(block_size * block_size) / symbol_count);
    
    // Print RLE symbols
    print_rle_symbols(ctx);
    
    // Decode back to block
    int **decoded = alloc_int_array(block_size, block_size);
    run_length_decode(ctx, decoded, block_size);
    
    // Print decoded block
    printf("Decoded block:\n");
    for (int i = 0; i < block_size; i++) {
        for (int j = 0; j < block_size; j++) {
            printf("%4d ", decoded[i][j]);
        }
        printf("\n");
    }
    printf("\n");
    
    // Verify reconstruction
    int errors = 0;
    for (int i = 0; i < block_size; i++) {
        for (int j = 0; j < block_size; j++) {
            if (block[i][j] != decoded[i][j]) {
                errors++;
                printf("Error at [%d][%d]: expected %d, got %d\n", 
                       i, j, block[i][j], decoded[i][j]);
            }
        }
    }
    
    if (errors == 0) {
        printf("RLE test PASSED! Decoded block matches original.\n\n");
    } else {
        printf("RLE test FAILED! %d errors found.\n\n", errors);
    }
    
    // Clean up
    free_int_array(block, block_size);
    free_int_array(decoded, block_size);
    entropy_free(ctx);
}

// Test Huffman coding
void test_huffman_coding(void) {
    printf("=== Testing Huffman Coding ===\n");
    
    // Create a sample 8x8 block with mostly zeros
    int block_size = 8;
    int **block = alloc_int_array(block_size, block_size);
    
    // Initialize with a pattern that has some frequency bias
    for (int i = 0; i < block_size; i++) {
        for (int j = 0; j < block_size; j++) {
            if (i < 2 && j < 2) {
                block[i][j] = 10 - i - j;  // Some larger values
            } else if (i % 2 == 0 && j % 2 == 0) {
                block[i][j] = 2;  // Repeating value
            } else if (i % 3 == 0 || j % 3 == 0) {
                block[i][j] = -1;  // Repeating negative value
            } else {
                block[i][j] = 0;  // Zeros (most common)
            }
        }
    }
    
    // Print original block
    printf("Original block with frequency-biased values:\n");
    for (int i = 0; i < block_size; i++) {
        for (int j = 0; j < block_size; j++) {
            printf("%4d ", block[i][j]);
        }
        printf("\n");
    }
    printf("\n");
    
    // Encode using RLE and Huffman
    EntropyContext *ctx = entropy_init(1);  // Use Huffman coding
    int symbol_count = run_length_encode(ctx, block, block_size);
    printf("RLE produced %d symbols\n\n", symbol_count);
    
    // Print RLE symbols
    print_rle_symbols(ctx);
    
    // Build Huffman codes
    build_huffman_codes(ctx);
    
    // Print Huffman codes
    print_huffman_codes(ctx);
    
    // Calculate encoded size
    int bit_size = get_encoded_size(ctx);
    printf("Estimated bit size with Huffman coding: %d bits\n", bit_size);
    printf("Without Huffman coding it would be: %d bits\n", symbol_count * 24);
    printf("Huffman compression ratio: %.2f:1\n\n", 
           (float)(symbol_count * 24) / bit_size);
    
    // Decode back to block
    int **decoded = alloc_int_array(block_size, block_size);
    run_length_decode(ctx, decoded, block_size);
    
    // Verify reconstruction
    int errors = 0;
    for (int i = 0; i < block_size; i++) {
        for (int j = 0; j < block_size; j++) {
            if (block[i][j] != decoded[i][j]) {
                errors++;
                printf("Error at [%d][%d]: expected %d, got %d\n", 
                       i, j, block[i][j], decoded[i][j]);
            }
        }
    }
    
    if (errors == 0) {
        printf("Huffman test PASSED! Decoded block matches original.\n\n");
    } else {
        printf("Huffman test FAILED! %d errors found.\n\n", errors);
    }
    
    // Clean up
    free_int_array(block, block_size);
    free_int_array(decoded, block_size);
    entropy_free(ctx);
}

// Test with real DCT coefficients
void test_with_dct_coefficients(void) {
    printf("=== Testing with Real DCT Coefficients ===\n");
    
    // Create a DCT context
    int block_size = 8;
    DCTContext *dct_ctx = dct_init(block_size);
    
    // Create a quantization context
    int quality = 50;
    QuantContext *quant_ctx = quant_init(block_size, quality, 0);
    
    // Sample 8x8 input block (from test_dct.c)
    unsigned char pixel_block[64] = {
        52, 55, 61, 66, 70, 61, 64, 73,
        63, 59, 55, 90, 109, 85, 69, 72,
        62, 59, 68, 113, 144, 104, 66, 73,
        63, 58, 71, 122, 154, 106, 70, 69,
        67, 61, 68, 104, 126, 88, 68, 70,
        79, 65, 60, 70, 77, 68, 58, 75,
        85, 71, 64, 59, 55, 61, 65, 83,
        87, 79, 69, 68, 65, 76, 78, 94
    };
    
    // Create input block from pixels
    double **input_block = alloc_array(block_size, block_size);
    for (int i = 0; i < block_size; i++) {
        for (int j = 0; j < block_size; j++) {
            input_block[i][j] = (double)pixel_block[i * block_size + j] - 128.0;
        }
    }
    
    // Perform forward DCT
    double **dct_coeffs = alloc_array(block_size, block_size);
    dct_forward(dct_ctx, input_block, dct_coeffs);
    
    // Quantize the coefficients
    int **quant_coeffs = alloc_int_array(block_size, block_size);
    double block_variance = calculate_block_variance(input_block, block_size);
    quantize(quant_ctx, dct_coeffs, quant_coeffs, block_variance);
    
    // Print quantized DCT coefficients
    printf("Quantized DCT coefficients:\n");
    for (int i = 0; i < block_size; i++) {
        for (int j = 0; j < block_size; j++) {
            printf("%4d ", quant_coeffs[i][j]);
        }
        printf("\n");
    }
    printf("\n");
    
    // Encode with RLE and Huffman
    EntropyContext *entropy_ctx = entropy_init(1);  // Use Huffman
    int symbol_count = run_length_encode(entropy_ctx, quant_coeffs, block_size);
    
    printf("RLE encoded to %d symbols (out of %d coefficients)\n", 
           symbol_count, block_size * block_size);
    printf("RLE compression ratio: %.2f:1\n\n", 
           (float)(block_size * block_size) / symbol_count);
    
    // Build Huffman codes
    build_huffman_codes(entropy_ctx);
    
    // Get encoded size
    int bit_size = get_encoded_size(entropy_ctx);
    printf("Estimated bit size: %d bits (%.2f bytes)\n", 
           bit_size, bit_size / 8.0);
    printf("Original pixel data size: %d bytes\n", block_size * block_size);
    printf("Total compression ratio (DCT + Quantization + Entropy): %.2f:1\n\n", 
           (float)(block_size * block_size * 8) / bit_size);
    
    // Decode back to coefficients
    int **decoded_coeffs = alloc_int_array(block_size, block_size);
    run_length_decode(entropy_ctx, decoded_coeffs, block_size);
    
    // Verify coefficient reconstruction
    int errors = 0;
    for (int i = 0; i < block_size; i++) {
        for (int j = 0; j < block_size; j++) {
            if (quant_coeffs[i][j] != decoded_coeffs[i][j]) {
                errors++;
            }
        }
    }
    
    if (errors == 0) {
        printf("Full pipeline test PASSED! Decoded coefficients match original.\n");
    } else {
        printf("Full pipeline test FAILED! %d errors found.\n", errors);
    }
    
    // Continue the pipeline to get back to pixels
    double **dequant_coeffs = alloc_array(block_size, block_size);
    dequantize(quant_ctx, decoded_coeffs, dequant_coeffs, block_variance);
    
    double **output_block = alloc_array(block_size, block_size);
    dct_inverse(dct_ctx, dequant_coeffs, output_block);
    
    // Calculate PSNR against original pixels
    double mse = 0.0;
    for (int i = 0; i < block_size; i++) {
        for (int j = 0; j < block_size; j++) {
            double original = pixel_block[i * block_size + j];
            double reconstructed = output_block[i][j] + 128.0;
            
            // Clamp to 0-255
            if (reconstructed < 0) reconstructed = 0;
            if (reconstructed > 255) reconstructed = 255;
            
            double error = original - reconstructed;
            mse += error * error;
        }
    }
    mse /= (block_size * block_size);
    
    double psnr = 10 * log10(255 * 255 / mse);
    printf("PSNR: %.2f dB\n\n", psnr);
    
    // Clean up
    free_array(input_block, block_size);
    free_array(dct_coeffs, block_size);
    free_array(dequant_coeffs, block_size);
    free_array(output_block, block_size);
    free_int_array(quant_coeffs, block_size);
    free_int_array(decoded_coeffs, block_size);
    dct_free(dct_ctx);
    quant_free(quant_ctx);
    entropy_free(entropy_ctx);
}

int main(void) {
    printf("======================================\n");
    printf("     Entropy Coding Tests\n");
    printf("======================================\n\n");
    
    test_zigzag_scan();
    test_run_length_encoding();
    test_huffman_coding();
    test_with_dct_coefficients();
    
    printf("All tests completed!\n");
    return 0;
} 

