/**
 * test_entropy.c - Test suite for entropy coding implementation
 * Part of Adaptive DCT Image Compressor
 */

#include <entropy.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Test utility functions
void print_array(const char* name, int* array, int size) {
    printf("%s: [", name);
    for (int i = 0; i < size; i++) {
        printf("%d", array[i]);
        if (i < size - 1) printf(", ");
    }
    printf("]\n");
}

// Test zigzag scan
void test_zigzag_scan(void) {
    printf("Testing zigzag scan...\n");

    int block_size = 4;
    int** block = alloc_int_array(block_size, block_size);
    int* zigzag = (int*)malloc(block_size * block_size * sizeof(int));

    // Initialize block with sequential values
    int val = 0;
    for (int i = 0; i < block_size; i++) {
        for (int j = 0; j < block_size; j++) {
            block[i][j] = val++;
        }
    }

    // Perform zigzag scan
    zigzag_scan(block, zigzag, block_size);

    // Expected zigzag sequence for 4x4 block with sequential values
    int expected[16] = {0, 1, 5, 6, 2, 4, 7, 12, 3, 8, 11, 13, 9, 10, 14, 15};

    print_array("Actual", zigzag, block_size * block_size);
    print_array("Expected", expected, block_size * block_size);


    // Verify results
    for (int i = 0; i < block_size * block_size; i++) {
        assert(zigzag[i] == expected[i]);
    }

    printf("Zigzag scan test passed!\n");

    // Clean up
    free_int_array(block, block_size);
    free(zigzag);
}

// Test run-length encoding
void test_run_length_encode(void) {
    printf("Testing run-length encoding...\n");

    int size = 16;
    int zigzag[16] = {100, 0, 0, 50, 0, 0, 0, 0, 25, 0, 0, 0, 0, 0, 0, 0};
    RLESymbol* rle_symbols = (RLESymbol*)malloc(size * sizeof(RLESymbol));
    int rle_count = 0;

    // Perform RLE
    run_length_encode(zigzag, size, rle_symbols, &rle_count);

    // Expected result: (0,100), (2,50), (4,25), (0,0)
    assert(rle_count == 4);
    assert(rle_symbols[0].run_length == 0 && rle_symbols[0].value == 100);
    assert(rle_symbols[1].run_length == 2 && rle_symbols[1].value == 50);
    assert(rle_symbols[2].run_length == 4 && rle_symbols[2].value == 25);
    assert(rle_symbols[3].run_length == 0 && rle_symbols[3].value == 0); // EOB marker

    printf("Run-length encoding test passed!\n");

    // Clean up
    free(rle_symbols);
}

// Test Huffman coding
void test_huffman_coding(void) {
    printf("Testing Huffman coding...\n");

    // Sample symbols and frequencies
    int symbols[5] = {10, 20, 30, 40, 50};
    unsigned frequencies[5] = {45, 13, 12, 16, 9};
    int count = 5;

    // Build Huffman codes
    HuffCode* codes = build_huffman_codes(symbols, frequencies, count);

    // Verify codes are assigned (most frequent symbol should have shortest code)
    assert(codes[10].length <= codes[40].length);
    assert(codes[40].length <= codes[20].length);
    assert(codes[20].length <= codes[30].length);
    assert(codes[30].length <= codes[50].length);

    // Free codes
    for (int i = 0; i < count; i++) {
        int symbol = symbols[i];
        if (codes[symbol].bits) {
            free(codes[symbol].bits);
        }
    }
    free(codes);

    printf("Huffman coding test passed!\n");
}

// Test BitWriter and BitReader
void test_bit_io(void) {
    printf("Testing bit I/O operations...\n");

    // Create buffer and writer
    int buffer_size = 10;
    unsigned char* buffer = (unsigned char*)calloc(buffer_size, sizeof(unsigned char));
    BitWriter* writer = bit_writer_init(buffer, buffer_size);

    // Write bit pattern
    // Write 10101010 01010101 11110000
    int pattern1[8] = {1, 0, 1, 0, 1, 0, 1, 0};
    int pattern2[8] = {0, 1, 0, 1, 0, 1, 0, 1};
    int pattern3[8] = {1, 1, 1, 1, 0, 0, 0, 0};

    for (int i = 0; i < 8; i++) bit_writer_write_bit(writer, pattern1[i]);
    for (int i = 0; i < 8; i++) bit_writer_write_bit(writer, pattern2[i]);
    for (int i = 0; i < 8; i++) bit_writer_write_bit(writer, pattern3[i]);

    // Close writer
    int bytes_used = bit_writer_free(writer);
    assert(bytes_used == 3);

    // Check buffer contents
    assert(buffer[0] == 0xAA); // 10101010
    assert(buffer[1] == 0x55); // 01010101
    assert(buffer[2] == 0xF0); // 11110000

    // Create reader
    BitReader* reader = bit_reader_init(buffer, buffer_size);

    // Read and verify bits
    for (int i = 0; i < 8; i++) {
        int bit;
        bit_reader_read_bit(reader, &bit);
        assert(bit == pattern1[i]);
    }

    for (int i = 0; i < 8; i++) {
        int bit;
        bit_reader_read_bit(reader, &bit);
        assert(bit == pattern2[i]);
    }

    for (int i = 0; i < 8; i++) {
        int bit;
        bit_reader_read_bit(reader, &bit);
        assert(bit == pattern3[i]);
    }

    // Clean up
    bit_reader_free(reader);
    free(buffer);

    printf("Bit I/O test passed!\n");
}

// Test full encode/decode cycle
void test_entropy_codec(void) {
    printf("Testing full entropy encode/decode cycle...\n");

    int block_size = 8;
    int total_size = block_size * block_size;

    // Create sample block with test pattern
    int** block = alloc_int_array(block_size, block_size);
    for (int i = 0; i < block_size; i++) {
        for (int j = 0; j < block_size; j++) {
            if (i < 2 && j < 2) {
                // DC and low frequency components
                block[i][j] = 100 - (i * 10 + j * 5);
            } else if (i < 4 && j < 4) {
                // Mid frequency components
                block[i][j] = (i == j) ? 10 : 0;
            } else {
                // High frequency components - mostly zeros
                block[i][j] = 0;
            }
        }
    }

    // Print original block for debugging
    printf("Original block:\n");
    for (int i = 0; i < block_size; i++) {
        for (int j = 0; j < block_size; j++) {
            printf("%4d ", block[i][j]);
        }
        printf("\n");
    }

    // Initialize entropy context
    EntropyContext* ctx = entropy_init(block_size, 0); // Use Huffman coding

    // Allocate buffer for compressed data
    int buffer_size = block_size * block_size * 4; // Overallocate for safety
    unsigned char* buffer = (unsigned char*)malloc(buffer_size);

    // Encode block
    int output_size = buffer_size;
    int result = entropy_encode_block(ctx, block, buffer, &output_size);
    assert(result == 0);

    printf("Compressed size: %d bytes (%.2f%% of original)\n",
           output_size, (output_size * 100.0) / (total_size * sizeof(int)));

    // Decode block
    int** decoded_block = alloc_int_array(block_size, block_size);
    result = entropy_decode_block(ctx, buffer, output_size, decoded_block);
    assert(result == 0);

    // Print decoded block for debugging
    printf("Decoded block:\n");
    for (int i = 0; i < block_size; i++) {
        for (int j = 0; j < block_size; j++) {
            printf("%4d ", decoded_block[i][j]);
        }
        printf("\n");
    }

    // Verify decoded block matches original and log differences
    int mismatch_count = 0;
    for (int i = 0; i < block_size; i++) {
        for (int j = 0; j < block_size; j++) {
            if (block[i][j] != decoded_block[i][j]) {
                mismatch_count++;
                printf("Mismatch at position [%d][%d]: Original=%d, Decoded=%d\n",
                       i, j, block[i][j], decoded_block[i][j]);
            }
        }
    }

    if (mismatch_count > 0) {
        printf("Total mismatches: %d\n", mismatch_count);
        assert(0); // Fail the test
    } else {
        printf("All values match!\n");
    }

    // Clean up
    entropy_free(ctx);
    free_int_array(block, block_size);
    free_int_array(decoded_block, block_size);
    free(buffer);

    printf("Entropy codec test passed!\n");
}

// Test zigzag pattern generation
void test_zigzag_pattern(void) {
    printf("Testing zigzag pattern generation...\n");

    int block_size = 8;
    int** block = alloc_int_array(block_size, block_size);
    int* zigzag = (int*)malloc(block_size * block_size * sizeof(int));

    // Fill block with sequential values
    int val = 0;
    for (int i = 0; i < block_size; i++) {
        for (int j = 0; j < block_size; j++) {
            block[i][j] = val++;
        }
    }

    // Call zigzag scan
    zigzag_scan(block, zigzag, block_size);

    // Expected zigzag pattern matching your generate_zigzag_pattern function
    int expected[64] = {
            0,  1,  5,  6, 14, 15, 27, 28,
            2,  4,  7, 13, 16, 26, 29, 42,
            3,  8, 12, 17, 25, 30, 41, 43,
            9, 11, 18, 24, 31, 40, 44, 53,
            10, 19, 23, 32, 39, 45, 52, 54,
            20, 22, 33, 38, 46, 51, 55, 60,
            21, 34, 37, 47, 50, 56, 59, 61,
            35, 36, 48, 49, 57, 58, 62, 63
    };

    // Print actual vs expected values
    printf("\nComparing generated vs expected zigzag patterns:\n");
    for (int i = 0; i < block_size * block_size; i++) {
        printf("Index %2d: Got %2d, Expected %2d\n", i, zigzag[i], expected[i]);
    }

    // Verify against expected pattern
    for (int i = 0; i < block_size * block_size; i++) {
        if (zigzag[i] != expected[i]) {
            printf("Error at index %d: expected %d, got %d\n", i, expected[i], zigzag[i]);
            assert(0);
        }
    }

    free_int_array(block, block_size);
    free(zigzag);
    printf("Zigzag pattern test passed!\n");
}

// Test encoder with edge cases
void test_encoder_edge_cases(void) {
    printf("Testing encoder with edge cases...\n");

    int block_size = 4;
    EntropyContext* ctx = entropy_init(block_size, 0);

    // Test case 1: All zeros
    int** zeros_block = alloc_int_array(block_size, block_size);
    int buffer_size = 100;
    unsigned char* buffer = (unsigned char*)malloc(buffer_size);
    int output_size = buffer_size;

    int result = entropy_encode_block(ctx, zeros_block, buffer, &output_size);
    assert(result == 0);
    printf("All zeros block compressed to %d bytes\n", output_size);

    // Test case 2: Only DC coefficient
    int** dc_only_block = alloc_int_array(block_size, block_size);
    dc_only_block[0][0] = 100;
    output_size = buffer_size;

    result = entropy_encode_block(ctx, dc_only_block, buffer, &output_size);
    assert(result == 0);
    printf("DC-only block compressed to %d bytes\n", output_size);

    // Test case 3: Random values (higher entropy)
    int** random_block = alloc_int_array(block_size, block_size);
    for (int i = 0; i < block_size; i++) {
        for (int j = 0; j < block_size; j++) {
            random_block[i][j] = rand() % 20 - 10; // Values between -10 and 9
        }
    }
    output_size = buffer_size;

    result = entropy_encode_block(ctx, random_block, buffer, &output_size);
    assert(result == 0);
    printf("Random values block compressed to %d bytes\n", output_size);

    // Clean up
    entropy_free(ctx);
    free_int_array(zeros_block, block_size);
    free_int_array(dc_only_block, block_size);
    free_int_array(random_block, block_size);
    free(buffer);

    printf("Encoder edge cases test passed!\n");
}

// Test priority queue operations
void test_priority_queue(void) {
    printf("Testing priority queue operations...\n");

    // Create a priority queue
    HuffmanPQ* pq = pq_init(10);

    // Create and add some nodes
    HuffNode* n1 = (HuffNode*)malloc(sizeof(HuffNode));
    n1->frequency = 10;
    n1->symbol = 1;
    n1->left = n1->right = NULL;

    HuffNode* n2 = (HuffNode*)malloc(sizeof(HuffNode));
    n2->frequency = 5;
    n2->symbol = 2;
    n2->left = n2->right = NULL;

    HuffNode* n3 = (HuffNode*)malloc(sizeof(HuffNode));
    n3->frequency = 15;
    n3->symbol = 3;
    n3->left = n3->right = NULL;

    // Push nodes in random order
    pq_push(pq, n1);
    pq_push(pq, n3);
    pq_push(pq, n2);

    // Pop nodes and verify order (min heap: lowest frequency first)
    HuffNode* node = pq_pop(pq);
    assert(node->frequency == 5);
    assert(node->symbol == 2);

    node = pq_pop(pq);
    assert(node->frequency == 10);
    assert(node->symbol == 1);

    node = pq_pop(pq);
    assert(node->frequency == 15);
    assert(node->symbol == 3);

    // Verify queue is empty
    assert(pq->size == 0);

    // Clean up
    free(n1);
    free(n2);
    free(n3);
    pq_free(pq);

    printf("Priority queue test passed!\n");
}

int main(void) {
    printf("Running entropy coding tests...\n");

    // Run all tests
    test_zigzag_scan();
    test_run_length_encode();
    test_huffman_coding();
    test_bit_io();
    test_zigzag_pattern();
    test_priority_queue();
    test_encoder_edge_cases();
    test_entropy_codec();

    printf("All tests passed!\n");
    return 0;
}

