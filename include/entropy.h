/**
 * entropy.h - Header file for Entropy Coding implementation
 * Part of Adaptive DCT Image Compressor
 */

#ifndef ENTROPY_H
#define ENTROPY_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utils.h>

/**
 * Structure to represent Huffman tree node
 */
typedef struct HuffNode {
    int symbol;             // Symbol (coefficient value)
    unsigned frequency;     // Frequency of the symbol
    struct HuffNode *left;  // Left child
    struct HuffNode *right; // Right child
} HuffNode;

/**
 * Structure to represent a Huffman code
 */
typedef struct {
    int symbol;             // Symbol (coefficient value)
    char *code;             // Huffman code as string of '0's and '1's
} HuffCode;

/**
 * Structure to represent RLE encoding of a value
 */
typedef struct {
    int value;              // Coefficient value
    int run_length;         // Run length (number of consecutive zeros)
} RLESymbol;

/**
 * Structure to hold entropy coding context information
 */
typedef struct {
    int use_huffman;        // Flag to use Huffman coding (1) or just RLE (0)
    int capacity;           // Current capacity of RLE symbols array
    int count;              // Current count of RLE symbols
    RLESymbol *symbols;     // Array of RLE symbols
    HuffCode *huffman_codes; // Array of Huffman codes
    int huffman_size;       // Size of huffman_codes array
} EntropyContext;

/**
 * Initialize entropy coding context
 * 
 * @param use_huffman Flag to use Huffman coding (1) or just RLE (0)
 * @return Initialized entropy context
 */
EntropyContext* entropy_init(int use_huffman);

/**
 * Free entropy coding context resources
 *
 * @param ctx Entropy context to free
 */
void entropy_free(EntropyContext *ctx);

/**
 * Run-Length Encode quantized DCT coefficients
 * Uses zigzag scan pattern to encode coefficients
 *
 * @param ctx Entropy context
 * @param quant_coeffs Input quantized coefficients
 * @param block_size Size of the coefficient block
 * @return Number of RLE symbols generated
 */
int run_length_encode(EntropyContext *ctx, int **quant_coeffs, int block_size);

/**
 * Build Huffman codes from RLE symbols
 *
 * @param ctx Entropy context with RLE symbols
 */
void build_huffman_codes(EntropyContext *ctx);

/**
 * Decode RLE symbols back to coefficient block
 *
 * @param ctx Entropy context with RLE symbols
 * @param quant_coeffs Output quantized coefficients
 * @param block_size Size of the coefficient block
 */
void run_length_decode(EntropyContext *ctx, int **quant_coeffs, int block_size);

/**
 * Get estimated size of encoded data in bits
 *
 * @param ctx Entropy context with encoded data
 * @return Estimated size in bits
 */
int get_encoded_size(EntropyContext *ctx);

/**
 * Helper function to convert block to zigzag scan order
 *
 * @param block Input block
 * @param zigzag Output array in zigzag order
 * @param block_size Size of the block
 */
void block_to_zigzag(int **block, int *zigzag, int block_size);

/**
 * Helper function to convert zigzag scan order back to block
 *
 * @param zigzag Input array in zigzag order
 * @param block Output block
 * @param block_size Size of the block
 */
void zigzag_to_block(int *zigzag, int **block, int block_size);

#endif /* ENTROPY_H */ 


