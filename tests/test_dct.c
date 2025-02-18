/**
 * test_dct.c - Test file for DCT implementation
 * Part of Adaptive DCT Image Compressor
 */

#include "dct.h"

/**
 * Calculate mean squared error between original and reconstructed blocks
 */
double calculate_mse(unsigned char *original, double **reconstructed, int block_size) {
    double sum_squared_error = 0.0;

    for (int i = 0; i < block_size; i++) {
        for (int j = 0; j < block_size; j++) {
            double error = original[i * block_size + j] - (reconstructed[i][j] + 128.0);
            sum_squared_error += error * error;
        }
    }

    return sum_squared_error / (block_size * block_size);
}

/**
 * Test function for DCT implementation
 */
void test_dct(void) {
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

    // Print original vs reconstructed values
    printf("\nOriginal vs Reconstructed Pixel Values:\n");
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            double original = pixel_block[i * 8 + j];
            double recon = reconstructed[i][j] + 128.0;
            printf("%3.0f vs %6.2f  ", original, recon);
        }
        printf("\n");
    }

    // Calculate and print mean squared error
    double mse = calculate_mse(pixel_block, reconstructed, 8);
    printf("\nMean Squared Error: %.6f\n", mse);

    if (mse < 0.01) {
        printf("TEST PASSED: Reconstruction is accurate (MSE < 0.01)\n");
    } else {
        printf("TEST FAILED: Reconstruction error is too high (MSE >= 0.01)\n");
    }

    // Clean up
    free_array(input_block, 8);
    free_array(dct_coeffs, 8);
    free_array(reconstructed, 8);
    dct_free(ctx);
}

int main(void) {
    test_dct();
    printf("\nDCT implementation testing completed successfully.\n");
    return 0;
}

