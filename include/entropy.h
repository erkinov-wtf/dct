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
 * Huffman Tree Node structure
 */
typedef struct HuffNode {
    int symbol;             // Symbol (value) for leaf nodes, -1 for internal nodes
    unsigned frequency;     // Frequency of the symbol
    struct HuffNode *left;  // Left child
    struct HuffNode *right; // Right child
} HuffNode;

/**
 * Huffman Code structure
 */
typedef struct {
    unsigned char *bits;    // Bit representation (1s and 0s as ASCII)
    int length;             // Length of the code in bits
} HuffCode;

/**
 * Run-Length Encoding structure
 * Represents a run of zeros followed by a non-zero value
 */
typedef struct {
    int run_length;         // Number of zeros before the value
    int value;              // The non-zero value
} RLESymbol;

/**
 * Structure to hold entropy coding context information
 */
typedef struct {
    int block_size;             // Block size (4, 8, 16, etc.)
    HuffCode *huffman_codes;    // Array of Huffman codes (indexed by symbol)
    int symbols_count;          // Number of symbols in Huffman table
    int use_arithmetic;         // Whether to use arithmetic coding instead of Huffman
} EntropyContext;

/**
 * Initialize entropy coding context with given block size
 *
 * @param block_size Size of the block (must be power of 2: 4, 8, 16, etc.)
 * @param use_arithmetic Whether to use arithmetic coding (1) or Huffman coding (0)
 * @return Initialized entropy context
 */
EntropyContext* entropy_init(int block_size, int use_arithmetic);

/**
 * Free entropy context resources
 *
 * @param ctx Entropy context to free
 */
void entropy_free(EntropyContext *ctx);

/**
 * Perform zigzag scanning of a block to convert 2D array to 1D array
 * This reorders coefficients from low to high frequencies
 *
 * @param block 2D input block of quantized coefficients
 * @param output 1D output array (must be pre-allocated with block_size²)
 * @param block_size Size of the block
 */
void zigzag_scan(int **block, int *output, int block_size);

/**
 * Perform run-length encoding on zigzag scanned coefficients
 *
 * @param zigzag_data Input zigzag scanned coefficients
 * @param length Length of zigzag data (block_size²)
 * @param rle_symbols Output array of RLE symbols (must be pre-allocated)
 * @param rle_count Pointer to store the count of RLE symbols
 */
void run_length_encode(int *zigzag_data, int length, RLESymbol *rle_symbols, int *rle_count);

/**
 * Build Huffman codes for given symbols and frequencies
 *
 * @param symbols Array of symbols (can be RLE pair encoded as (run << 16) | value)
 * @param frequencies Array of frequencies for each symbol
 * @param count Number of unique symbols
 * @return Array of Huffman codes indexed by symbol
 */
HuffCode* build_huffman_codes(int *symbols, unsigned *frequencies, int count);

/**
 * Encode a block using RLE and Huffman coding
 *
 * @param ctx Entropy context
 * @param block Input block of quantized coefficients
 * @param output Output buffer for encoded bitstream
 * @param output_size Pointer to store the size of encoded data in bytes
 * @return 0 on success, non-zero on failure
 */
int entropy_encode_block(EntropyContext *ctx, int **block, unsigned char *output, int *output_size);

/**
 * Decode an entropy-encoded bitstream back to coefficients
 *
 * @param ctx Entropy context
 * @param input Input bitstream
 * @param input_size Size of input bitstream in bytes
 * @param output Output block for decoded coefficients (must be pre-allocated)
 * @return 0 on success, non-zero on failure
 */
int entropy_decode_block(EntropyContext *ctx, unsigned char *input, int input_size, int **output);

/**
 * Helper function for priority queue in Huffman coding (min heap)
 */
typedef struct {
    HuffNode **nodes;    // Array of node pointers
    int capacity;        // Maximum capacity
    int size;            // Current size
} HuffmanPQ;

/**
 * Initialize a priority queue for Huffman coding
 *
 * @param capacity Maximum number of nodes
 * @return Initialized priority queue
 */
HuffmanPQ* pq_init(int capacity);

/**
 * Add a node to the priority queue
 *
 * @param pq Priority queue
 * @param node Node to add
 */
void pq_push(HuffmanPQ *pq, HuffNode *node);

/**
 * Remove and return the minimum frequency node
 *
 * @param pq Priority queue
 * @return Node with minimum frequency
 */
HuffNode* pq_pop(HuffmanPQ *pq);

/**
 * Free priority queue resources
 *
 * @param pq Priority queue to free
 */
void pq_free(HuffmanPQ *pq);

/**
 * Helper functions for bit-level I/O
 */

/**
 * Structure for writing individual bits to a byte stream
 */
typedef struct {
    unsigned char *buffer;  // Output buffer
    int capacity;           // Buffer capacity in bytes
    int byte_pos;           // Current byte position
    int bit_pos;            // Current bit position (0-7)
} BitWriter;

/**
 * Initialize a bit writer
 *
 * @param buffer Output buffer
 * @param capacity Buffer capacity in bytes
 * @return Initialized bit writer
 */
BitWriter* bit_writer_init(unsigned char *buffer, int capacity);

/**
 * Write a single bit to the output stream
 *
 * @param writer Bit writer
 * @param bit Bit to write (0 or 1)
 * @return 0 on success, -1 on buffer overflow
 */
int bit_writer_write_bit(BitWriter *writer, int bit);

/**
 * Write multiple bits to the output stream
 *
 * @param writer Bit writer
 * @param bits Array of bits (0s and 1s as characters)
 * @param length Number of bits to write
 * @return 0 on success, -1 on buffer overflow
 */
int bit_writer_write_bits(BitWriter *writer, unsigned char *bits, int length);

/**
 * Free bit writer resources
 *
 * @param writer Bit writer to free
 * @return Number of bytes used in the buffer
 */
int bit_writer_free(BitWriter *writer);

/**
 * Structure for reading individual bits from a byte stream
 */
typedef struct {
    unsigned char *buffer;  // Input buffer
    int capacity;           // Buffer capacity in bytes
    int byte_pos;           // Current byte position
    int bit_pos;            // Current bit position (0-7)
} BitReader;

/**
 * Initialize a bit reader
 *
 * @param buffer Input buffer
 * @param capacity Buffer capacity in bytes
 * @return Initialized bit reader
 */
BitReader* bit_reader_init(unsigned char *buffer, int capacity);

/**
 * Read a single bit from the input stream
 *
 * @param reader Bit reader
 * @param bit Pointer to store the read bit
 * @return 0 on success, -1 on buffer underflow
 */
int bit_reader_read_bit(BitReader *reader, int *bit);

/**
 * Free bit reader resources
 *
 * @param reader Bit reader to free
 */
void bit_reader_free(BitReader *reader);

#endif /* ENTROPY_H */

