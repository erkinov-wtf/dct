/**
 * entropy.c - Implementation of Entropy Coding functions
 * Part of Adaptive DCT Image Compressor
 */

#include <entropy.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/**
 * Initialize entropy coding context with given block size
 */
EntropyContext *entropy_init(int block_size, int use_arithmetic) {
    EntropyContext *ctx = (EntropyContext *) malloc(sizeof(EntropyContext));
    if (!ctx) {
        fprintf(stderr, "Failed to allocate entropy context\n");
        return NULL;
    }

    ctx->block_size = block_size;
    ctx->huffman_codes = NULL;
    ctx->symbols_count = 0;
    ctx->use_arithmetic = use_arithmetic;

    return ctx;
}

/**
 * Free entropy context resources
 */
void entropy_free(EntropyContext *ctx) {
    if (!ctx) return;

    if (ctx->huffman_codes) {
        for (int i = 0; i < ctx->symbols_count; i++) {
            if (ctx->huffman_codes[i].bits) {
                free(ctx->huffman_codes[i].bits);
            }
        }
        free(ctx->huffman_codes);
    }

    free(ctx);
}

/**
 * Zigzag scan patterns for different block sizes
 * Only 4x4 and 8x8 defined here - larger sizes follow same pattern
 */
static const int zigzag_pattern_4x4[] = {
        0, 1, 5, 6,
        2, 4, 7, 12,
        3, 8, 11, 13,
        9, 10, 14, 15
};

static const int zigzag_pattern_8x8[] = {
        0, 1, 5, 6, 14, 15, 27, 28,
        2, 4, 7, 13, 16, 26, 29, 42,
        3, 8, 12, 17, 25, 30, 41, 43,
        9, 11, 18, 24, 31, 40, 44, 53,
        10, 19, 23, 32, 39, 45, 52, 54,
        20, 22, 33, 38, 46, 51, 55, 60,
        21, 34, 37, 47, 50, 56, 59, 61,
        35, 36, 48, 49, 57, 58, 62, 63
};

/**
 * Generate zigzag pattern for any block size
 */
static int *generate_zigzag_pattern(int block_size) {
    // Return predefined patterns for common sizes
    if (block_size == 4) {
        int *pattern = (int *) malloc(16 * sizeof(int));
        memcpy(pattern, zigzag_pattern_4x4, 16 * sizeof(int));
        return pattern;
    } else if (block_size == 8) {
        int *pattern = (int *) malloc(64 * sizeof(int));
        memcpy(pattern, zigzag_pattern_8x8, 64 * sizeof(int));
        return pattern;
    }

    // Generate pattern for other sizes
    int n = block_size * block_size;
    int *pattern = (int *) malloc(n * sizeof(int));

    int row = 0, col = 0;
    int index = 0;

    // Variables for tracking direction
    int going_up = 1;

    while (index < n) {
        // Fill current position
        pattern[row * block_size + col] = index++;

        if (going_up) {
            // Moving diagonally up and right
            if (col == block_size - 1) {
                // Reached right edge, move down
                row++;
                going_up = 0;
            } else if (row == 0) {
                // Reached top edge, move right
                col++;
                going_up = 0;
            } else {
                // Regular diagonal move
                row--;
                col++;
            }
        } else {
            // Moving diagonally down and left
            if (row == block_size - 1) {
                // Reached bottom edge, move right
                col++;
                going_up = 1;
            } else if (col == 0) {
                // Reached left edge, move down
                row++;
                going_up = 1;
            } else {
                // Regular diagonal move
                row++;
                col--;
            }
        }
    }

    return pattern;
}

/**
 * Perform zigzag scanning of a block
 */
void zigzag_scan(int **block, int *output, int block_size) {
    int *pattern = generate_zigzag_pattern(block_size);
    int n = block_size * block_size;

    for (int i = 0; i < n; i++) {
        int pos = pattern[i];
        int row = pos / block_size;
        int col = pos % block_size;
        output[i] = block[row][col];
    }

    free(pattern);
}

/**
 * Perform run-length encoding on zigzag scanned coefficients
 */
void run_length_encode(int *zigzag_data, int length, RLESymbol *rle_symbols, int *rle_count) {
    int count = 0;
    int zero_run = 0;

    for (int i = 0; i < length; i++) {
        if (zigzag_data[i] == 0) {
            zero_run++;
        } else {
            // Store the run length and value
            rle_symbols[count].run_length = zero_run;
            rle_symbols[count].value = zigzag_data[i];
            count++;
            zero_run = 0;
        }
    }

    // Handle trailing zeros with a special EOB (End Of Block) marker
    // Here we use a run length of 0 and value of 0 to indicate EOB
    if (zero_run > 0) {
        rle_symbols[count].run_length = 0;
        rle_symbols[count].value = 0;  // EOB marker
        count++;
    }

    *rle_count = count;
}

/**
 * Priority queue functions for Huffman coding
 */
HuffmanPQ *pq_init(int capacity) {
    HuffmanPQ *pq = (HuffmanPQ *) malloc(sizeof(HuffmanPQ));
    pq->nodes = (HuffNode **) malloc(capacity * sizeof(HuffNode *));
    pq->capacity = capacity;
    pq->size = 0;
    return pq;
}

void pq_push(HuffmanPQ *pq, HuffNode *node) {
    if (pq->size == pq->capacity) {
        // Resize if needed
        pq->capacity *= 2;
        pq->nodes = (HuffNode **) realloc(pq->nodes, pq->capacity * sizeof(HuffNode *));
    }

    // Insert at the end
    int i = pq->size++;
    pq->nodes[i] = node;

    // Bubble up
    while (i > 0) {
        int parent = (i - 1) / 2;
        if (pq->nodes[parent]->frequency <= pq->nodes[i]->frequency) {
            break;
        }
        // Swap
        HuffNode *temp = pq->nodes[parent];
        pq->nodes[parent] = pq->nodes[i];
        pq->nodes[i] = temp;
        i = parent;
    }
}

HuffNode *pq_pop(HuffmanPQ *pq) {
    if (pq->size == 0) return NULL;

    HuffNode *min = pq->nodes[0];
    pq->nodes[0] = pq->nodes[--pq->size];

    // Bubble down
    int i = 0;
    while (1) {
        int left = 2 * i + 1;
        int right = 2 * i + 2;
        int smallest = i;

        if (left < pq->size && pq->nodes[left]->frequency < pq->nodes[smallest]->frequency) {
            smallest = left;
        }

        if (right < pq->size && pq->nodes[right]->frequency < pq->nodes[smallest]->frequency) {
            smallest = right;
        }

        if (smallest == i) break;

        // Swap
        HuffNode *temp = pq->nodes[i];
        pq->nodes[i] = pq->nodes[smallest];
        pq->nodes[smallest] = temp;
        i = smallest;
    }

    return min;
}

void pq_free(HuffmanPQ *pq) {
    free(pq->nodes);
    free(pq);
}

/**
 * Create a new Huffman tree node
 */
static HuffNode *create_huffman_node(int symbol, unsigned frequency) {
    HuffNode *node = (HuffNode *) malloc(sizeof(HuffNode));
    if (!node) {
        fprintf(stderr, "Failed to allocate Huffman node\n");
        return NULL;
    }

    node->symbol = symbol;
    node->frequency = frequency;
    node->left = NULL;
    node->right = NULL;

    return node;
}

/**
 * Recursively free Huffman tree
 */
static void free_huffman_tree(HuffNode *node) {
    if (!node) return;

    free_huffman_tree(node->left);
    free_huffman_tree(node->right);
    free(node);
}

/**
 * Recursively generate Huffman codes
 */
static void generate_codes(HuffNode *node, unsigned char *code, int depth, HuffCode *codes) {
    if (!node) return;

    if (!node->left && !node->right) {
        // Leaf node - store the code
        int symbol = node->symbol;

        // Check for valid symbol range
        if (symbol < 0 || symbol >= 65536) {
            fprintf(stderr, "Symbol %d out of valid range (0-65535)\n", symbol);
            return;
        }

        codes[symbol].length = depth;
        codes[symbol].bits = (unsigned char *) malloc(depth + 1);

        memcpy(codes[symbol].bits, code, depth);
        codes[symbol].bits[depth] = '\0'; // Null-terminate for easier debugging
    } else {
        // Internal node - recurse
        if (depth >= 255) {
            fprintf(stderr, "Huffman code too long (>255 bits)\n");
            return;
        }

        // Left branch (0)
        code[depth] = '0';
        generate_codes(node->left, code, depth + 1, codes);

        // Right branch (1)
        code[depth] = '1';
        generate_codes(node->right, code, depth + 1, codes);
    }
}

/**
 * Build Huffman codes for given symbols and frequencies
 */
HuffCode *build_huffman_codes(int *symbols, unsigned *frequencies, int count) {
    // Create leaf nodes for each symbol
    HuffmanPQ *pq = pq_init(count);
    for (int i = 0; i < count; i++) {
        HuffNode *node = create_huffman_node(symbols[i], frequencies[i]);
        pq_push(pq, node);
    }

    // Build the Huffman tree
    while (pq->size > 1) {
        HuffNode *left = pq_pop(pq);
        HuffNode *right = pq_pop(pq);

        // Create a internal node with combined frequency
        HuffNode *parent = create_huffman_node(-1, left->frequency + right->frequency);
        parent->left = left;
        parent->right = right;

        pq_push(pq, parent);
    }

    // Get the root of the tree
    HuffNode *root = pq_pop(pq);

    // Find the maximum symbol value to determine the size of the code table
    int max_symbol = -1;
    for (int i = 0; i < count; i++) {
        if (symbols[i] > max_symbol) {
            max_symbol = symbols[i];
        }
    }

    // Allocate code table
    HuffCode *codes = (HuffCode *) calloc(65536, sizeof(HuffCode)); // 16-bit symbols max

    // Generate codes
    if (root) {
        unsigned char *code = (unsigned char *) malloc(256); // Temporary code array
        generate_codes(root, code, 0, codes);
        free(code);
    }

    // Clean up
    free_huffman_tree(root);
    pq_free(pq);

    return codes;
}

/**
 * Bit I/O functions for entropy coding
 */
BitWriter *bit_writer_init(unsigned char *buffer, int capacity) {
    BitWriter *writer = (BitWriter *) malloc(sizeof(BitWriter));
    writer->buffer = buffer;
    writer->capacity = capacity;
    writer->byte_pos = 0;
    writer->bit_pos = 0;

    // Initialize buffer to zeros
    memset(buffer, 0, capacity);

    return writer;
}

int bit_writer_write_bit(BitWriter *writer, int bit) {
    if (writer->byte_pos >= writer->capacity) {
        return -1; // Buffer overflow
    }

    // Set the bit in the current byte
    if (bit) {
        writer->buffer[writer->byte_pos] |= (1 << (7 - writer->bit_pos));
    }

    // Move to the next bit position
    writer->bit_pos++;
    if (writer->bit_pos == 8) {
        writer->bit_pos = 0;
        writer->byte_pos++;
    }

    return 0;
}

int bit_writer_write_bits(BitWriter *writer, unsigned char *bits, int length) {
    for (int i = 0; i < length; i++) {
        int bit = (bits[i] == '1') ? 1 : 0;
        if (bit_writer_write_bit(writer, bit) < 0) {
            return -1;
        }
    }
    return 0;
}

int bit_writer_free(BitWriter *writer) {
    int bytes_used = writer->byte_pos;
    if (writer->bit_pos > 0) {
        bytes_used++; // Account for partial byte
    }
    free(writer);
    return bytes_used;
}

BitReader *bit_reader_init(unsigned char *buffer, int capacity) {
    BitReader *reader = (BitReader *) malloc(sizeof(BitReader));
    reader->buffer = buffer;
    reader->capacity = capacity;
    reader->byte_pos = 0;
    reader->bit_pos = 0;
    return reader;
}

int bit_reader_read_bit(BitReader *reader, int *bit) {
    if (reader->byte_pos >= reader->capacity) {
        return -1; // Buffer underflow
    }

    // Get the bit from the current byte
    *bit = (reader->buffer[reader->byte_pos] >> (7 - reader->bit_pos)) & 1;

    // Move to the next bit position
    reader->bit_pos++;
    if (reader->bit_pos == 8) {
        reader->bit_pos = 0;
        reader->byte_pos++;
    }

    return 0;
}

void bit_reader_free(BitReader *reader) {
    free(reader);
}

/**
 * Encode a symbol table for huffman decoding
 */
static int encode_symbol_table(BitWriter *writer, int *symbols, int *original_symbols, int count, HuffCode *codes) {
    // Write the number of symbols
    for (int i = 0; i < 16; i++) {
        int bit = (count >> (15 - i)) & 1;
        bit_writer_write_bit(writer, bit);
    }

    // For each symbol, write its original value and code length
    for (int i = 0; i < count; i++) {
        int symbol = symbols[i];
        int original = original_symbols[i];
        int length = codes[symbol].length;

        // Write 32-bit original symbol value
        for (int j = 0; j < 32; j++) {
            int bit = (original >> (31 - j)) & 1;
            bit_writer_write_bit(writer, bit);
        }

        // Write 8-bit code length
        for (int j = 0; j < 8; j++) {
            int bit = (length >> (7 - j)) & 1;
            bit_writer_write_bit(writer, bit);
        }
    }

    return 0;
}

/**
 * Decode a symbol table for huffman decoding
 */
static int decode_symbol_table(BitReader *reader, int **symbols, int **original_symbols, HuffCode **codes, int *count) {
    // Read the number of symbols
    int num_symbols = 0;
    for (int i = 0; i < 16; i++) {
        int bit;
        if (bit_reader_read_bit(reader, &bit) < 0) {
            return -1;
        }
        num_symbols = (num_symbols << 1) | bit;
    }

    // Allocate arrays
    *symbols = (int *) malloc(num_symbols * sizeof(int));
    *original_symbols = (int *) malloc(num_symbols * sizeof(int));
    *codes = (HuffCode *) calloc(65536, sizeof(HuffCode)); // 16-bit symbols

    // Read the symbols and their code lengths
    for (int i = 0; i < num_symbols; i++) {
        // Simple sequential index as symbol
        (*symbols)[i] = i;

        // Read 32-bit original symbol value
        int original = 0;
        for (int j = 0; j < 32; j++) {
            int bit;
            if (bit_reader_read_bit(reader, &bit) < 0) {
                free(*symbols);
                free(*original_symbols);
                free(*codes);
                return -1;
            }
            original = (original << 1) | bit;
        }
        (*original_symbols)[i] = original;

        // Read 8-bit code length
        int length = 0;
        for (int j = 0; j < 8; j++) {
            int bit;
            if (bit_reader_read_bit(reader, &bit) < 0) {
                free(*symbols);
                free(*original_symbols);
                free(*codes);
                return -1;
            }
            length = (length << 1) | bit;
        }

        (*codes)[i].length = length;
        // We don't need to store the actual bits for decoding
    }

    *count = num_symbols;
    return 0;
}

/**
 * Encode a block using RLE and Huffman coding
 */
int entropy_encode_block(EntropyContext *ctx, int **block, unsigned char *output, int *output_size) {
    int block_size = ctx->block_size;
    int total_size = block_size * block_size;

    // Step 1: Zigzag scan to reorder coefficients
    int *zigzag_data = (int *) malloc(total_size * sizeof(int));
    zigzag_scan(block, zigzag_data, block_size);

    // Step 2: Run-length encode the zigzag data
    RLESymbol *rle_symbols = (RLESymbol *) malloc(total_size * sizeof(RLESymbol));
    int rle_count = 0;
    run_length_encode(zigzag_data, total_size, rle_symbols, &rle_count);

    // Step 3: Calculate symbol frequencies for Huffman coding
#define MAX_SYMBOLS 65536  // 16-bit max

    // Map original symbols to small indices
    int *original_symbols = (int *) malloc(rle_count * sizeof(int));
    int *symbol_indices = (int *) malloc(rle_count * sizeof(int));
    unsigned *frequencies = (unsigned *) calloc(rle_count, sizeof(unsigned));

    // First pass: gather unique symbols
    int unique_count = 0;
    for (int i = 0; i < rle_count; i++) {
        int run = rle_symbols[i].run_length;
        int value = rle_symbols[i].value;

        // Encode sign + magnitude for value (keep within reasonable range)
        int encoded_value;
        if (value == 0) {
            encoded_value = 0;
        } else if (value > 0) {
            encoded_value = (value < 32768) ? (value << 1) : 65534;  // Cap at 16-bit limit
        } else {
            encoded_value = ((-value) < 32768) ? (((-value) << 1) | 1) : 65535;
        }

        // Cap run length
        if (run > 255) run = 255;

        // Create original symbol (for later lookup)
        original_symbols[i] = (run << 16) | encoded_value;

        // Look up if symbol already exists
        int j;
        for (j = 0; j < unique_count; j++) {
            if (original_symbols[j] == original_symbols[i]) {
                frequencies[j]++;
                break;
            }
        }

        // New symbol
        if (j == unique_count) {
            // If more than MAX_SYMBOLS unique symbols, we'd need a more complex solution
            if (unique_count >= MAX_SYMBOLS) {
                fprintf(stderr, "Too many unique symbols!\n");
                // In production code, you'd implement a more robust solution here
                break;
            }

            symbol_indices[unique_count] = unique_count;  // Simple sequential index
            frequencies[unique_count] = 1;
            unique_count++;
        }

        // Store the symbol index for later use
        symbol_indices[i] = j;
    }

    // Step 4: Build Huffman codes using the smaller symbol indices
    int *unique_indices = (int *) malloc(unique_count * sizeof(int));
    for (int i = 0; i < unique_count; i++) {
        unique_indices[i] = i;  // Simple sequential indices
    }

    HuffCode *huffman_codes = build_huffman_codes(unique_indices, frequencies, unique_count);

    // Step 5: Encode the data
    BitWriter *writer = bit_writer_init(output, *output_size);

    // Write header with symbol table
    encode_symbol_table(writer, unique_indices, original_symbols, unique_count, huffman_codes);

    // Write encoded data
    for (int i = 0; i < rle_count; i++) {
        int index = symbol_indices[i];
        bit_writer_write_bits(writer, huffman_codes[index].bits, huffman_codes[index].length);
    }

    // Get final size
    *output_size = bit_writer_free(writer);

    // Clean up
    free(zigzag_data);
    free(rle_symbols);
    free(original_symbols);
    free(symbol_indices);
    free(unique_indices);

    // Free Huffman codes
    for (int i = 0; i < unique_count; i++) {
        if (huffman_codes[i].bits) {
            free(huffman_codes[i].bits);
        }
    }
    free(huffman_codes);
    free(frequencies);

    return 0;
}

/**
 * Build a decoding Huffman tree from symbol table
 */
static HuffNode *build_decoding_tree(int *symbols, HuffCode *codes, int count) {
    HuffNode *root = create_huffman_node(-1, 0);

    // Generate canonical Huffman codes
    // In this approach, we assign codes in order of symbols
    unsigned code_value = 0;
    int last_len = 0;

    // Sort symbols by code length (simple bubble sort for demonstration)
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            if (codes[symbols[j]].length > codes[symbols[j + 1]].length) {
                // Swap
                int temp = symbols[j];
                symbols[j] = symbols[j + 1];
                symbols[j + 1] = temp;
            }
        }
    }

    // Generate and insert codes
    for (int i = 0; i < count; i++) {
        int symbol = symbols[i];
        int length = codes[symbol].length;

        if (length == 0) continue; // Skip symbols with no code

        // Shift code if moving to longer length
        if (last_len != 0 && length > last_len) {
            code_value <<= (length - last_len);
        }

        // Convert code_value to bit string
        unsigned char *code_bits = (unsigned char *) malloc(length);
        for (int j = 0; j < length; j++) {
            code_bits[j] = ((code_value >> (length - j - 1)) & 1) ? '1' : '0';
        }

        // Insert into tree
        HuffNode *node = root;
        for (int j = 0; j < length; j++) {
            if (code_bits[j] == '0') {
                if (!node->left) {
                    node->left = create_huffman_node(-1, 0);
                }
                node = node->left;
            } else {
                if (!node->right) {
                    node->right = create_huffman_node(-1, 0);
                }
                node = node->right;
            }
        }

        // Set the symbol at the leaf
        node->symbol = symbol;

        free(code_bits);

        // Update for next code
        code_value++;
        last_len = length;
    }

    return root;
}

/**
 * Decode an entropy-encoded bitstream back to coefficients
 */
int entropy_decode_block(EntropyContext *ctx, unsigned char *input, int input_size, int **output) {
    int block_size = ctx->block_size;
    int total_size = block_size * block_size;

    // Initialize bit reader
    BitReader *reader = bit_reader_init(input, input_size);

    // Read the symbol table
    int *symbols;
    int *original_symbols;
    HuffCode *codes;
    int count;
    if (decode_symbol_table(reader, &symbols, &original_symbols, &codes, &count) < 0) {
        bit_reader_free(reader);
        return -1;
    }

    // Build Huffman decoding tree
    HuffNode *root = build_decoding_tree(symbols, codes, count);

    // Decode the data
    int *zigzag_data = (int *) calloc(total_size, sizeof(int));
    int zigzag_pos = 0;

    while (zigzag_pos < total_size) {
        // Decode one symbol (RLE pair)
        HuffNode *node = root;
        while (node->left || node->right) {
            int bit;
            if (bit_reader_read_bit(reader, &bit) < 0) {
                // End of input - fill rest with zeros
                break;
            }

            if (bit == 0) {
                if (node->left) node = node->left;
                else break; // Invalid code
            } else {
                if (node->right) node = node->right;
                else break; // Invalid code
            }
        }

        if (!node->left && !node->right) {
            // Leaf node - we have a symbol
            int symbol = node->symbol;
            int original = original_symbols[symbol];

            // Extract run length and value
            int run = (original >> 16) & 0xFFFF;
            int encoded_value = original & 0xFFFF;

            // Decode sign + magnitude
            int value;
            if (encoded_value == 0) {
                value = 0;
            } else {
                int magnitude = encoded_value >> 1;
                int sign = encoded_value & 1;
                value = sign ? -magnitude : magnitude;
            }

            // Add zeros for the run
            for (int i = 0; i < run && zigzag_pos < total_size; i++) {
                zigzag_data[zigzag_pos++] = 0;
            }

            // Add the value
            if (zigzag_pos < total_size) {
                zigzag_data[zigzag_pos++] = value;
            }

            // Check for EOB marker (run=0, value=0)
            if (run == 0 && value == 0) {
                // Fill rest with zeros
                while (zigzag_pos < total_size) {
                    zigzag_data[zigzag_pos++] = 0;
                }
                break;  // We're done
            }
        } else {
            // Invalid code or end of input
            break;
        }
    }

    // Fill the rest with zeros if needed
    while (zigzag_pos < total_size) {
        zigzag_data[zigzag_pos++] = 0;
    }

    // Create output buffer for 2D coefficients if not provided
    //int allocate_output = 0;
    if (output[0] == NULL) {
       // allocate_output = 1;
        for (int i = 0; i < block_size; i++) {
            output[i] = (int *) malloc(block_size * sizeof(int));
        }
    }

    // Reverse zigzag scan to get 2D coefficients
    int *pattern = generate_zigzag_pattern(block_size);

    // Initialize output with zeros
    for (int i = 0; i < block_size; i++) {
        for (int j = 0; j < block_size; j++) {
            output[i][j] = 0;
        }
    }

    // Fill with zigzag data
    for (int i = 0; i < total_size; i++) {
        int pos = pattern[i];
        int row = pos / block_size;
        int col = pos % block_size;
        if (row < block_size && col < block_size) {
            output[row][col] = zigzag_data[i];
        }
    }

    // Clean up
    free(zigzag_data);
    free(pattern);
    free(symbols);
    free(original_symbols);
    free(codes);
    free_huffman_tree(root);
    bit_reader_free(reader);

    return 0;
}
