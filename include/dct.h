/**
 * dct.h - Header file for Discrete Cosine Transform implementation
 * Part of Adaptive DCT Image Compressor
 */

#ifndef DCT_H
#define DCT_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define PI 3.14159265358979323846

/**
 * Structure to hold DCT context information
 * This helps with supporting variable block sizes
 */
typedef struct {
    int block_size;          // Block size (4, 8, 16, etc.)
    double **dct_matrix;     // Pre-computed DCT matrix
    double **transposed_dct; // Transposed DCT matrix for fast IDCT
} DCTContext;

/**
 * Initialize DCT context with given block size
 * This precomputes the DCT matrix for faster transforms
 *
 * @param block_size Size of the block (must be power of 2: 4, 8, 16, etc.)
 * @return Initialized DCT context
 */
DCTContext* dct_init(int block_size);

/**
 * Free DCT context resources
 *
 * @param ctx DCT context to free
 */
void dct_free(DCTContext *ctx);

/**
 * Forward DCT transform (DCT-II)
 * Takes input block and writes frequency coefficients to output block
 *
 * @param ctx DCT context containing precomputed matrices
 * @param input Input block in spatial domain (size: block_size x block_size)
 * @param output Output block for frequency coefficients (size: block_size x block_size)
 */
void dct_forward(DCTContext *ctx, double **input, double **output);

/**
 * Inverse DCT transform (IDCT)
 * Takes frequency coefficients and reconstructs the spatial domain block
 *
 * @param ctx DCT context containing precomputed matrices
 * @param input Input block of frequency coefficients (size: block_size x block_size)
 * @param output Output block for reconstructed spatial data (size: block_size x block_size)
 */
void dct_inverse(DCTContext *ctx, double **input, double **output);

/**
 * Helper function to allocate 2D arrays
 *
 * @param rows Number of rows
 * @param cols Number of columns
 * @return Allocated 2D array
 */
double** alloc_array(int rows, int cols);

/**
 * Helper function to free 2D arrays
 *
 * @param array Array to free
 * @param rows Number of rows
 */
void free_array(double **array, int rows);

/**
 * Helper function to create and initialize a block from pixel data
 *
 * @param pixels Raw pixel data array
 * @param width Width of the image
 * @param row_start Starting row position in the image
 * @param col_start Starting column position in the image
 * @param block_size Size of the block to create
 * @return Initialized block with pixel data centered around zero
 */
double** create_block_from_pixels(unsigned char *pixels, int width, int row_start, int col_start, int block_size);

/**
 * Helper function to copy coefficients back to integer array
 *
 * @param block Source block with coefficient values
 * @param coefficients Destination integer array
 * @param block_size Size of the block
 */
void copy_block_to_coefficients(double **block, int **coefficients, int block_size);

#endif /* DCT_H */

