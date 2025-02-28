/**
 * quantization.h - Header file for Quantization implementation
 * Part of Adaptive DCT Image Compressor
 */

#ifndef QUANTIZATION_H
#define QUANTIZATION_H

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <utils.h>

/**
 * Structure to hold quantization context information
 */
typedef struct {
    int block_size;                // Block size (4, 8, 16, etc.)
    int quality;                   // Quality factor (1-100)
    double **quant_matrix;         // Quantization matrix
    double **dequant_matrix;       // Dequantization matrix (inverse)
    int adaptive;                  // Flag for adaptive quantization
} QuantContext;

/**
 * Initialize quantization context with given block size and quality
 *
 * @param block_size Size of the block (must be power of 2: 4, 8, 16, etc.)
 * @param quality Quality factor (1-100, higher is better quality)
 * @param adaptive Whether to use adaptive quantization (0 = off, 1 = on)
 * @return Initialized quantization context
 */
QuantContext* quant_init(int block_size, int quality, int adaptive);

/**
 * Free quantization context resources
 *
 * @param ctx Quantization context to free
 */
void quant_free(QuantContext *ctx);

/**
 * Generate standard JPEG-style quantization matrix for luminance
 *
 * @param block_size Size of the block
 * @param quality Quality factor (1-100)
 * @return Generated quantization matrix
 */
double** generate_quant_matrix(int block_size, int quality);

/**
 * Generate dequantization matrix (inverse of quantization matrix)
 *
 * @param quant_matrix Quantization matrix
 * @param block_size Size of the block
 * @return Generated dequantization matrix
 */
double** generate_dequant_matrix(double **quant_matrix, int block_size);

/**
 * Apply quantization to DCT coefficients
 *
 * @param ctx Quantization context
 * @param dct_coeffs Input DCT coefficients
 * @param quant_coeffs Output quantized coefficients
 * @param block_variance Variance of the block (for adaptive quantization)
 */
void quantize(QuantContext *ctx, double **dct_coeffs, int **quant_coeffs, double block_variance);

/**
 * Apply dequantization (inverse quantization)
 *
 * @param ctx Quantization context
 * @param quant_coeffs Input quantized coefficients
 * @param dct_coeffs Output dequantized coefficients
 * @param block_variance Variance of the block (for adaptive quantization)
 */
void dequantize(QuantContext *ctx, int **quant_coeffs, double **dct_coeffs, double block_variance);

/**
 * Calculate variance of a block for adaptive quantization
 *
 * @param block Input block
 * @param block_size Size of the block
 * @return Variance of the block
 */
double calculate_block_variance(double **block, int block_size);

/**
 * Adjust quantization matrix based on block variance
 *
 * @param ctx Quantization context
 * @param variance Block variance
 * @param is_quantize Whether this is for quantization (1) or dequantization (0)
 * @return Adjusted matrix for this block
 */
double** adjust_matrix_for_block(QuantContext *ctx, double variance, int is_quantize);

#endif /* QUANTIZATION_H */

