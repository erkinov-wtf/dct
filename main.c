#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define PI 3.14159265358979323846

// DCT Context variables block
typedef struct {
    int block_size; // 4, 8, 16
    double **dct_matrix; // pre-computed DCT matrix
    double **transposed_dct; // transposed DCT matrix
} DCTContext;

// func to allocate 2D arrays
double **alloc_array(int rows, int cols) {
    double **new_array = (double **) malloc(rows * sizeof(double *));
    if (!new_array) {
        fprintf(stderr, "Memory allocation failed, when creating new 2D array\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < rows; ++i) {
        new_array[i] = (double *) malloc(cols * sizeof(double));
        if (!new_array[i]) {
            fprintf(stderr, "Memory allocation failed, when creating new 2D array\n");
            exit(EXIT_FAILURE);
        }
        memset(new_array[i], 0, cols * sizeof(double));
    }

    return new_array;
}

// func to free 2D arrays
void free_array(double **array, int rows) {
    for (int i = 0; i < rows; ++i) {
        free(array[i]);
    }
    free(array);
}

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

    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            temp[i][j] = 0.0;
            for (int k = 0; k < size; ++k) {
                temp[i][j] += ctx->dct_matrix[i][k] * input[k][j];
            }
        }
    }

    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            output[i][j] = 0.0;
            for (int k = 0; k < size; ++k) {
                output[i][j] += temp[i][k] * ctx->transposed_dct[k][j];
            }
        }
    }

    free_array(temp, size);
}

void dct_inverse(DCTContext *ctx, double **input, double **output) {
    int size = ctx->block_size;
    double **temp = alloc_array(size, size);

    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            temp[i][j] = 0.0;
            for (int k = 0; k < size; ++k) {
                temp[i][j] += ctx->transposed_dct[i][k] * input[k][k];
            }
        }
    }

    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            output[i][j] = 0.0;
            for (int k = 0; k < size; ++k) {
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

void example_dct_usage() {
    // Create and initialize DCT context for 8x8 blocks
    DCTContext *ctx = dct_init(8);

    // Sample 8x8 input block (simulating pixel values)
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
    double **input_block = alloc_array(8, 8);
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            input_block[i][j] = (double)pixel_block[i * 8 + j] - 128.0;
        }
    }

    // Allocate output block for DCT coefficients
    double **dct_coeffs = alloc_array(8, 8);

    // Perform forward DCT
    dct_forward(ctx, input_block, dct_coeffs);

    // Print some of the DCT coefficients
    printf("Some DCT Coefficients:\n");
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            printf("%8.2f ", dct_coeffs[i][j]);
        }
        printf("\n");
    }

    // Allocate output block for reconstructed image data
    double **reconstructed = alloc_array(8, 8);

    // Perform inverse DCT to get back the original image
    dct_inverse(ctx, dct_coeffs, reconstructed);

    // Print some of the reconstructed values
    printf("\nReconstructed Pixel Values (should be close to original):\n");
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            printf("%8.2f ", reconstructed[i][j] + 128.0);
        }
        printf("\n");
    }

    // Clean up
    free_array(input_block, 8);
    free_array(dct_coeffs, 8);
    free_array(reconstructed, 8);
    dct_free(ctx);
}

int main(void) {
    example_dct_usage();
    printf("\nDCT implementation completed successfully.\n");
    return 0;
}
