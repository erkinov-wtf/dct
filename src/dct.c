/**
 * dct.c - Implementation file for Discrete Cosine Transform
 * Part of Adaptive DCT Image Compressor
 */
#include <dct.h>

DCTContext *dct_init(int block_size) {
    DCTContext *ctx = (DCTContext *) malloc(sizeof(DCTContext));
    if (!ctx) {
        fprintf(stderr, "Memory allocation failed, when creating new context\n");
        exit(EXIT_FAILURE);
    }

    ctx->block_size = block_size;
    ctx->dct_matrix = alloc_array(block_size, block_size);
    ctx->transposed_dct = alloc_array(block_size, block_size);

    // computing
    for (int i = 0; i < block_size; ++i) {
        for (int j = 0; j < block_size; ++j) {
            double alpha;
            if (i == 0) {
                alpha = 1.0 / sqrt(block_size);
            } else {
                alpha = sqrt(2.0 / block_size);
            }

            ctx->dct_matrix[i][j] = alpha * cos((PI * (2 * j + 1) * i) / (2.0 * block_size));
        }
    }

    // precompute
    for (int i = 0; i < block_size; ++i) {
        for (int j = 0; j < block_size; ++j) {
            ctx->transposed_dct[i][j] = ctx->dct_matrix[j][i];
        }
    }

    return ctx;
}


void dct_free(DCTContext *ctx) {
    if (ctx) {
        free_array(ctx->dct_matrix, ctx->block_size);
        free_array(ctx->transposed_dct, ctx->block_size);
        free(ctx);
    }
}


void dct_forward(DCTContext *ctx, double **input, double **output) {
    int size = ctx->block_size;
    double **temp = alloc_array(size, size);

    // First perform DCT across rows: temp = input * DCT^T
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            temp[i][j] = 0.0;
            for (int k = 0; k < size; k++) {
                temp[i][j] += input[i][k] * ctx->transposed_dct[k][j];
            }
        }
    }

    // Then perform DCT across columns: output = DCT * temp
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            output[i][j] = 0.0;
            for (int k = 0; k < size; k++) {
                output[i][j] += ctx->dct_matrix[i][k] * temp[k][j];
            }
        }
    }

    free_array(temp, size);
}


void dct_inverse(DCTContext *ctx, double **input, double **output) {
    int size = ctx->block_size;
    double **temp = alloc_array(size, size);

    // First perform IDCT across columns: temp = DCT^T * input
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            temp[i][j] = 0.0;
            for (int k = 0; k < size; k++) {
                temp[i][j] += ctx->transposed_dct[i][k] * input[k][j];
            }
        }
    }

    // Then perform IDCT across rows: output = temp * DCT
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            output[i][j] = 0.0;
            for (int k = 0; k < size; k++) {
                output[i][j] += temp[i][k] * ctx->dct_matrix[k][j];
            }
        }
    }

    free_array(temp, size);
}


// func to create and init block from pixels
double **create_block_from_pixels(unsigned char *pixels, int width, int row_start, int col_start, int block_size) {
    double **block = alloc_array(block_size, block_size);

    for (int i = 0; i < block_size; ++i) {
        for (int j = 0; j < block_size; ++j) {
            int pixel_index = (row_start + i) * width + (col_start + j);
            block[i][j] = (double) pixels[pixel_index] - 128.0;
        }
    }

    return block;
}


void copy_block_to_coefficients(double **block, int **coefficients, int block_size) {
    for (int i = 0; i < block_size; ++i) {
        for (int j = 0; j < block_size; ++j) {
            coefficients[i][j] = (int) round(block[i][j]);
        }
    }
}

