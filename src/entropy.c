/**
 * entropy.c - Implementation file for Entropy Coding
 * Part of Adaptive DCT Image Compressor
 */

#include "../include/entropy.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <utils.h>

#define INITIAL_CAPACITY 64
#define MAX_HUFFMAN_CODE_LEN 32

// Priority queue for Huffman tree construction
typedef struct {
    HuffNode **nodes;
    int size;
    int capacity;
} PriorityQueue;

// Priority queue functions
PriorityQueue* pq_create(int capacity) {
    PriorityQueue *pq = (PriorityQueue*)malloc(sizeof(PriorityQueue));
    pq->nodes = (HuffNode**)malloc(capacity * sizeof(HuffNode*));
    pq->size = 0;
    pq->capacity = capacity;
    return pq;
}

void pq_free(PriorityQueue *pq) {
    free(pq->nodes);
    free(pq);
}

void pq_push(PriorityQueue *pq, HuffNode *node) {
    int i = pq->size++;
    
    // Simple insertion sort to maintain priority queue
    while (i > 0 && pq->nodes[(i - 1) / 2]->frequency > node->frequency) {
        pq->nodes[i] = pq->nodes[(i - 1) / 2];
        i = (i - 1) / 2;
    }
    
    pq->nodes[i] = node;
}

HuffNode* pq_pop(PriorityQueue *pq) {
    if (pq->size == 0) return NULL;
    
    HuffNode *min = pq->nodes[0];
    pq->nodes[0] = pq->nodes[--pq->size];
    
    // Heapify
    int i = 0;
    while (i * 2 + 1 < pq->size) {
        int smallest = i;
        int left = i * 2 + 1;
        int right = i * 2 + 2;
        
        if (left < pq->size && pq->nodes[left]->frequency < pq->nodes[smallest]->frequency)
            smallest = left;
            
        if (right < pq->size && pq->nodes[right]->frequency < pq->nodes[smallest]->frequency)
            smallest = right;
            
        if (smallest == i) break;
        
        // Swap
        HuffNode *temp = pq->nodes[i];
        pq->nodes[i] = pq->nodes[smallest];
        pq->nodes[smallest] = temp;
        
        i = smallest;
    }
    
    return min;
}

// Helper functions for Huffman coding
HuffNode* create_huff_node(int symbol, unsigned frequency) {
    HuffNode *node = (HuffNode*)malloc(sizeof(HuffNode));
    node->symbol = symbol;
    node->frequency = frequency;
    node->left = NULL;
    node->right = NULL;
    return node;
}

void free_huffman_tree(HuffNode *root) {
    if (root == NULL) return;
    free_huffman_tree(root->left);
    free_huffman_tree(root->right);
    free(root);
}

/**
 * Generate Huffman codes by traversing the Huffman tree
 */
void generate_codes(HuffNode *root, char *prefix, int depth, HuffCode *codes, int *index) {
    if (root == NULL) return;
    
    // If leaf node, store the code
    if (root->left == NULL && root->right == NULL) {
        codes[*index].symbol = root->symbol;
        codes[*index].code = (char*)malloc((depth + 1) * sizeof(char));
        strcpy(codes[*index].code, prefix);
        (*index)++;
        return;
    }
    
    // Traverse left (append '0')
    char left_prefix[MAX_HUFFMAN_CODE_LEN];
    strcpy(left_prefix, prefix);
    strcat(left_prefix, "0");
    generate_codes(root->left, left_prefix, depth + 1, codes, index);
    
    // Traverse right (append '1')
    char right_prefix[MAX_HUFFMAN_CODE_LEN];
    strcpy(right_prefix, prefix);
    strcat(right_prefix, "1");
    generate_codes(root->right, right_prefix, depth + 1, codes, index);
}

/**
 * Initialize entropy coding context
 */
EntropyContext* entropy_init(int use_huffman) {
    EntropyContext *ctx = (EntropyContext*)malloc(sizeof(EntropyContext));
    ctx->use_huffman = use_huffman;
    ctx->capacity = INITIAL_CAPACITY;
    ctx->count = 0;
    ctx->symbols = (RLESymbol*)malloc(ctx->capacity * sizeof(RLESymbol));
    ctx->huffman_codes = NULL;
    ctx->huffman_size = 0;
    return ctx;
}

/**
 * Free entropy coding context resources
 */
void entropy_free(EntropyContext *ctx) {
    free(ctx->symbols);
    
    if (ctx->huffman_codes) {
        for (int i = 0; i < ctx->huffman_size; i++) {
            free(ctx->huffman_codes[i].code);
        }
        free(ctx->huffman_codes);
    }
    
    free(ctx);
}

/**
 * Convert block to zigzag scan order
 */
void block_to_zigzag(int **block, int *zigzag, int block_size) {
    int index = 0;
    
    // Implement zigzag traversal
    for (int sum = 0; sum <= 2 * (block_size - 1); sum++) {
        // For even sums, traverse up-right
        if (sum % 2 == 0) {
            for (int i = (sum < block_size) ? sum : block_size - 1; 
                 i >= 0 && (sum - i) < block_size; i--) {
                zigzag[index++] = block[i][sum - i];
            }
        } 
        // For odd sums, traverse down-left
        else {
            for (int i = (sum < block_size) ? 0 : sum - block_size + 1; 
                 i < block_size && (sum - i) >= 0; i++) {
                zigzag[index++] = block[i][sum - i];
            }
        }
    }
}

/**
 * Convert zigzag scan order back to block
 */
void zigzag_to_block(int *zigzag, int **block, int block_size) {
    int index = 0;
    
    // Zero the block first
    for (int i = 0; i < block_size; i++) {
        for (int j = 0; j < block_size; j++) {
            block[i][j] = 0;
        }
    }
    
    // Implement reverse zigzag traversal
    for (int sum = 0; sum <= 2 * (block_size - 1); sum++) {
        // For even sums, traverse up-right
        if (sum % 2 == 0) {
            for (int i = (sum < block_size) ? sum : block_size - 1; 
                 i >= 0 && (sum - i) < block_size; i--) {
                block[i][sum - i] = zigzag[index++];
            }
        } 
        // For odd sums, traverse down-left
        else {
            for (int i = (sum < block_size) ? 0 : sum - block_size + 1; 
                 i < block_size && (sum - i) >= 0; i++) {
                block[i][sum - i] = zigzag[index++];
            }
        }
    }
}

/**
 * Run-Length Encode quantized DCT coefficients
 * Uses zigzag scan pattern to encode coefficients
 */
int run_length_encode(EntropyContext *ctx, int **quant_coeffs, int block_size) {
    // Reset the context
    ctx->count = 0;
    
    // Convert block to zigzag order
    int size = block_size * block_size;
    int *zigzag = (int*)malloc(size * sizeof(int));
    block_to_zigzag(quant_coeffs, zigzag, block_size);
    
    // Perform RLE
    int zero_count = 0;
    
    for (int i = 0; i < size; i++) {
        // If we encountered a non-zero value or reached the end
        if (zigzag[i] != 0 || i == size - 1) {
            // If we're at the end and the value is zero
            if (i == size - 1 && zigzag[i] == 0) {
                zero_count++;
            }
            
            // Make sure we have enough capacity
            if (ctx->count >= ctx->capacity) {
                ctx->capacity *= 2;
                ctx->symbols = (RLESymbol*)realloc(ctx->symbols, ctx->capacity * sizeof(RLESymbol));
            }
            
            // Add the symbol
            ctx->symbols[ctx->count].value = zigzag[i];
            ctx->symbols[ctx->count].run_length = zero_count;
            ctx->count++;
            
            // Reset zero count for next run
            zero_count = 0;
        } else {
            zero_count++;
        }
    }
    
    free(zigzag);
    return ctx->count;
}

/**
 * Build Huffman codes from RLE symbols
 */
void build_huffman_codes(EntropyContext *ctx) {
    if (!ctx->use_huffman || ctx->count == 0) return;
    
    // First, count frequencies
    int max_symbol = 0;
    for (int i = 0; i < ctx->count; i++) {
        if (abs(ctx->symbols[i].value) > max_symbol) {
            max_symbol = abs(ctx->symbols[i].value);
        }
    }
    
    // Frequency table (include sign, so double the range)
    int symbol_count = 2 * max_symbol + 2;  // +1 for zero, +1 because we're 1-indexing
    unsigned *freq = (unsigned*)calloc(symbol_count, sizeof(unsigned));
    
    // Map values to symbols: -N to N-1, 0 to N, +N to N+N
    for (int i = 0; i < ctx->count; i++) {
        int symbol;
        if (ctx->symbols[i].value == 0) {
            symbol = max_symbol + 1;  // Middle point
        } else if (ctx->symbols[i].value < 0) {
            symbol = max_symbol + 1 + ctx->symbols[i].value;  // Left side
        } else {
            symbol = max_symbol + 1 + ctx->symbols[i].value;  // Right side
        }
        freq[symbol]++;
    }
    
    // Build priority queue
    PriorityQueue *pq = pq_create(symbol_count);
    for (int i = 0; i < symbol_count; i++) {
        if (freq[i] > 0) {
            pq_push(pq, create_huff_node(i - (max_symbol + 1), freq[i]));
        }
    }
    
    // Build Huffman tree
    while (pq->size > 1) {
        HuffNode *left = pq_pop(pq);
        HuffNode *right = pq_pop(pq);
        
        HuffNode *parent = create_huff_node(-1, left->frequency + right->frequency);
        parent->left = left;
        parent->right = right;
        
        pq_push(pq, parent);
    }
    
    // Get root of Huffman tree
    HuffNode *root = pq_pop(pq);
    
    // Generate Huffman codes
    ctx->huffman_size = 0;
    for (int i = 0; i < symbol_count; i++) {
        if (freq[i] > 0) ctx->huffman_size++;
    }
    
    ctx->huffman_codes = (HuffCode*)malloc(ctx->huffman_size * sizeof(HuffCode));
    
    int index = 0;
    char prefix[MAX_HUFFMAN_CODE_LEN] = "";
    generate_codes(root, prefix, 0, ctx->huffman_codes, &index);
    
    // Clean up
    free(freq);
    free_huffman_tree(root);
    pq_free(pq);
}

/**
 * Decode RLE symbols back to coefficient block
 */
void run_length_decode(EntropyContext *ctx, int **quant_coeffs, int block_size) {
    int size = block_size * block_size;
    int *zigzag = (int*)malloc(size * sizeof(int));
    
    // Initialize all to zero
    for (int i = 0; i < size; i++) {
        zigzag[i] = 0;
    }
    
    // Decode RLE symbols to zigzag
    int pos = 0;
    for (int i = 0; i < ctx->count; i++) {
        // Skip zeros
        pos += ctx->symbols[i].run_length;
        
        // Make sure we don't exceed array bounds
        if (pos < size) {
            zigzag[pos++] = ctx->symbols[i].value;
        }
    }
    
    // Convert zigzag back to block
    zigzag_to_block(zigzag, quant_coeffs, block_size);
    
    free(zigzag);
}

/**
 * Get estimated size of encoded data in bits
 */
int get_encoded_size(EntropyContext *ctx) {
    int total_bits = 0;
    
    // If we're using Huffman coding
    if (ctx->use_huffman && ctx->huffman_codes) {
        // Each symbol is encoded with its Huffman code
        for (int i = 0; i < ctx->count; i++) {
            int symbol_value = ctx->symbols[i].value;
            
            // Find the code for this symbol
            char *code = NULL;
            for (int j = 0; j < ctx->huffman_size; j++) {
                if (ctx->huffman_codes[j].symbol == symbol_value) {
                    code = ctx->huffman_codes[j].code;
                    break;
                }
            }
            
            if (code) {
                // Add code length
                total_bits += strlen(code);
            } else {
                // Fallback if code not found
                total_bits += 8;
            }
            
            // Add bits for run length (fixed-length encoding)
            total_bits += 8;  // Assuming 8 bits for run length
        }
    } else {
        // Simple fixed-length encoding (no Huffman)
        // Each RLE symbol is a (value, run_length) pair
        total_bits = ctx->count * (16 + 8);  // 16 bits for value, 8 for run length
    }
    
    return total_bits;
}


