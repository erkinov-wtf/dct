/**
 * quantization.c - Implementation file for Quantization
 * Part of Adaptive DCT Image Compressor
 */
#include <quantization.h>

// Standard JPEG luminance quantization matrix (8x8)
static const int std_jpeg_luma_quant[8][8] = {
        {16, 11, 10, 16, 24,  40,  51,  61},
        {12, 12, 14, 19, 26,  58,  60,  55},
        {14, 13, 16, 24, 40,  57,  69,  56},
        {14, 17, 22, 29, 51,  87,  80,  62},
        {18, 22, 37, 56, 68,  109, 103, 77},
        {24, 35, 55, 64, 81,  104, 113, 92},
        {49, 64, 78, 87, 103, 121, 120, 101},
        {72, 92, 95, 98, 112, 100, 103, 99}
};

QuantContext *quant_init(int block_size, int quality, int adaptive) {
    QuantContext *ctx = (QuantContext *) malloc(sizeof(QuantContext));
    if (!ctx) {
        fprintf(stderr, "Memory allocation failed when creating quantization context\n");
        exit(EXIT_FAILURE);
    }

    if (quality < 1) {
        quality = 1;
    }
    if (quality > 100) {
        quality = 100;
    }

    ctx->block_size = block_size;
    ctx->quality = quality;
    ctx->adaptive = adaptive;

    ctx->quant_matrix = generate_quant_matrix(block_size, quality);
    ctx->dequant_matrix = generate_dequant_matrix(ctx->quant_matrix, block_size);

    return ctx;
}

void quant_free(QuantContext *ctx) {
    if (ctx) {
        free_array(ctx->quant_matrix, ctx->block_size);
        free_array(ctx->dequant_matrix, ctx->block_size);
        free(ctx);
    }
}

double **generate_quant_matrix(int block_size, int quality) {
    double **matrix = alloc_array(block_size, block_size);
    double scale_factor;

    if (quality < 50) {
        scale_factor = 5000.0 / quality;
    } else {
        scale_factor = 200.0 - 2 * quality;
    }
    scale_factor /= 100.0;

    // For standard 8x8 block, use JPEG table
    if (block_size == 8) {
        for (int i = 0; i < block_size; ++i) {
            for (int j = 0; j < block_size; ++j) {
                double value = std_jpeg_luma_quant[i][j] * scale_factor;

                if (value < 1.0) {
                    value = 1.0;
                }
                if (value > 255.0) {
                    value = 255.0;
                }

                matrix[i][j] = value;
            }
        }
    } else {
        // For other block sizes, generate a custom matrix
        // Higher frequencies (larger i+j) get larger values
        for (int i = 0; i < block_size; ++i) {
            for (int j = 0; j < block_size; ++j) {
                double distance = sqrt((double) (i * i + j * j));
                double value = (1.0 + distance) * scale_factor * 8.0;

                if (value < 1.0) {
                    value = 1.0;
                }
                if (value > 255.0) {
                    value = 255.0;
                }

                matrix[i][j] = value;
            }
        }
    }

    return matrix;
}

double **generate_dequant_matrix(double **quant_matrix, int block_size) {
    double **dequant = alloc_array(block_size, block_size);

    for (int i = 0; i < block_size; ++i) {
        for (int j = 0; j < block_size; ++j) {
            dequant[i][j] = 1.0 / quant_matrix[i][j];
        }
    }

    return dequant;
}

void quantize(QuantContext *ctx, double **dct_coeffs, int **quant_coeffs, double block_variance) {
    double **matrix;

    if (ctx->adaptive) {
        matrix = adjust_matrix_for_block(ctx, block_variance, 1);
    } else {
        matrix = ctx->quant_matrix;
    }

    for (int i = 0; i < ctx->block_size; ++i) {
        for (int j = 0; j < ctx->block_size; ++j) {
            quant_coeffs[i][j] = (int) round(dct_coeffs[i][j] / matrix[i][j]);
        }
    }

    if (ctx->adaptive) {
        free_array(matrix, ctx->block_size);
    }
}

void dequantize(QuantContext *ctx, int **quant_coeffs, double **dct_coeffs, double block_variance) {
    double **matrix;

    if (ctx->adaptive) {
        matrix = adjust_matrix_for_block(ctx, block_variance, 0);
    } else {
        matrix = ctx->dequant_matrix;
    }

    for (int i = 0; i < ctx->block_size; ++i) {
        for (int j = 0; j < ctx->block_size; ++j) {
            dct_coeffs[i][j] = quant_coeffs[i][j] * (ctx->adaptive ? 1.0 / matrix[i][j] : matrix[i][j]);
        }
    }

    if (ctx->adaptive) {
        free_array(matrix, ctx->block_size);
    }
}

double calculate_block_variance(double **block, int block_size) {
    double sum = 0.0;
    double sum_sq = 0.0;
    int count = block_size * block_size;

    for (int i = 0; i < block_size; ++i) {
        for (int j = 0; j < block_size; ++j) {
            sum += block[i][j];
            sum_sq += block[i][j] * block[i][j];
        }
    }

    double mean = sum / count;
    double variance = (sum_sq / count) - (mean * mean);

    return variance;
}

double **adjust_matrix_for_block(QuantContext *ctx, double variance, int is_quantize) {
    double **matrix = alloc_array(ctx->block_size, ctx->block_size);
    double **source;
    if (is_quantize) {
        source = ctx->quant_matrix;
    } else {
        source = ctx->dequant_matrix;
    }

    // Determine scaling factor based on variance
    // High variance (detail) = less quantization (smaller values)
    // Low variance (flat areas) = more quantization (larger values)
    double scale;

    // Normalizing matrix
    double norm_variance = fmin(1.0, fmax(0.1, variance / 1000.0));

    if (is_quantize) {
        // For quantization: high variance -> lower scaling (preserve details)
        scale = 2.0 - norm_variance; // 1.0 <-> 1.9
    } else {
        // For dequantization: inverse relationship
        scale = 1.0 / (2.0 - norm_variance);
    }

    for (int i = 0; i < ctx->block_size; ++i) {
        for (int j = 0; j < ctx->block_size; ++j) {
            if (i == 0 && j == 0) {
                matrix[i][j] = source[i][j];
            } else {
                matrix[i][j] = source[i][j] * scale;

                if (is_quantize && matrix[i][j] < 1.0) {
                    matrix[i][j] = 1.0;
                }
            }
        }
    }

    return matrix;
}

