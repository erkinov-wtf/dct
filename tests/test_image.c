/**
 * test_image.c - Test file for Image Processing implementation
 * Part of Adaptive DCT Image Compressor
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <image.h>

// Utility function to create a simple test pattern image
Image* create_test_pattern(int width, int height) {
    Image* img = image_create(width, height, 3, 8, FORMAT_RGB);
    if (!img) {
        return NULL;
    }

    // Create a test pattern: gradient + vertical bars
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // Horizontal gradient for R channel
            unsigned char r = (unsigned char)(255 * x / width);

            // Vertical gradient for G channel
            unsigned char g = (unsigned char)(255 * y / height);

            // Vertical bars for B channel
            unsigned char b = (x % 32 < 16) ? 255 : 0;

            image_set_pixel(img, x, y, 0, r);
            image_set_pixel(img, x, y, 1, g);
            image_set_pixel(img, x, y, 2, b);
        }
    }

    return img;
}

// Utility function to calculate image PSNR
double calculate_psnr(Image* original, Image* processed) {
    if (!original || !processed ||
        original->width != processed->width ||
        original->height != processed->height) {
        return -1.0;
    }

    double mse = 0.0;
    int samples = 0;

    for (int y = 0; y < original->height; y++) {
        for (int x = 0; x < original->width; x++) {
            for (int c = 0; c < original->channels; c++) {
                int orig = image_get_pixel(original, x, y, c);
                int proc = image_get_pixel(processed, x, y, c);
                int diff = orig - proc;
                mse += diff * diff;
                samples++;
            }
        }
    }

    if (samples == 0) return -1.0;
    mse /= samples;

    if (mse == 0) return INFINITY;
    return 10 * log10(255.0 * 255.0 / mse);
}

// Print a small section of image for verification
void print_image_section(Image* img, int start_x, int start_y, int width, int height, const char* title) {
    if (!img) return;

    // Adjust dimensions if they exceed image bounds
    if (start_x + width > img->width) width = img->width - start_x;
    if (start_y + height > img->height) height = img->height - start_y;

    printf("%s (%d x %d section starting at [%d, %d]):\n",
           title, width, height, start_x, start_y);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (img->channels == 1) {
                printf("%3d ", image_get_pixel(img, start_x + x, start_y + y, 0));
            } else {
                printf("(%3d,%3d,%3d) ",
                       image_get_pixel(img, start_x + x, start_y + y, 0),
                       image_get_pixel(img, start_x + x, start_y + y, 1),
                       image_get_pixel(img, start_x + x, start_y + y, 2));
            }
        }
        printf("\n");
    }
    printf("\n");
}

// Test image creation functionality
void test_image_creation(void) {
    printf("=== Testing Image Creation ===\n");

    int width = 64, height = 48;

    // Test RGB image creation
    printf("Creating RGB image (%d x %d)...\n", width, height);
    Image* rgb_img = image_create(width, height, 3, 8, FORMAT_RGB);

    if (!rgb_img) {
        printf("FAILED: Could not create RGB image\n\n");
        return;
    }

    printf("SUCCESS: RGB image created\n");
    printf("Image properties:\n");
    printf("- Width: %d\n", rgb_img->width);
    printf("- Height: %d\n", rgb_img->height);
    printf("- Channels: %d\n", rgb_img->channels);
    printf("- Bit depth: %d\n", rgb_img->bit_depth);
    printf("- Format: %s\n\n", rgb_img->format == FORMAT_RGB ? "RGB" : "Other");

    // Test YCbCr image creation
    printf("Creating YCbCr image (%d x %d)...\n", width, height);
    Image* ycbcr_img = image_create(width, height, 3, 8, FORMAT_YCBCR);

    if (!ycbcr_img) {
        printf("FAILED: Could not create YCbCr image\n\n");
        image_free(rgb_img);
        return;
    }

    printf("SUCCESS: YCbCr image created\n");
    printf("Image properties:\n");
    printf("- Width: %d\n", ycbcr_img->width);
    printf("- Height: %d\n", ycbcr_img->height);
    printf("- Channels: %d\n", ycbcr_img->channels);
    printf("- Bit depth: %d\n", ycbcr_img->bit_depth);
    printf("- Format: %s\n", ycbcr_img->format == FORMAT_YCBCR ? "YCbCr" : "Other");
    printf("- Y channel: %p\n", (void*)ycbcr_img->y_channel);
    printf("- Cb channel: %p\n", (void*)ycbcr_img->cb_channel);
    printf("- Cr channel: %p\n\n", (void*)ycbcr_img->cr_channel);

    // Test grayscale image creation
    printf("Creating Grayscale image (%d x %d)...\n", width, height);
    Image* gray_img = image_create(width, height, 1, 8, FORMAT_GRAYSCALE);

    if (!gray_img) {
        printf("FAILED: Could not create Grayscale image\n\n");
        image_free(rgb_img);
        image_free(ycbcr_img);
        return;
    }

    printf("SUCCESS: Grayscale image created\n");
    printf("Image properties:\n");
    printf("- Width: %d\n", gray_img->width);
    printf("- Height: %d\n", gray_img->height);
    printf("- Channels: %d\n", gray_img->channels);
    printf("- Bit depth: %d\n", gray_img->bit_depth);
    printf("- Format: %s\n\n", gray_img->format == FORMAT_GRAYSCALE ? "Grayscale" : "Other");

    // Test invalid parameters
    printf("Testing invalid parameters...\n");
    Image* invalid_img = image_create(0, -10, 5, 12, 99);

    if (!invalid_img) {
        printf("SUCCESS: Invalid parameters correctly rejected\n\n");
    } else {
        printf("FAILED: Invalid parameters were accepted\n\n");
        image_free(invalid_img);
    }

    // Clean up
    image_free(rgb_img);
    image_free(ycbcr_img);
    image_free(gray_img);

    printf("Image creation test completed\n\n");
}

// Test pixel access functions
void test_pixel_access(void) {
    printf("=== Testing Pixel Access Functions ===\n");

    int width = 8, height = 8;
    Image* test_img = image_create(width, height, 3, 8, FORMAT_RGB);

    if (!test_img) {
        printf("FAILED: Could not create test image\n\n");
        return;
    }

    // Set specific pixel values
    printf("Setting pixel values...\n");
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            unsigned char r = (x * 32) % 256;
            unsigned char g = (y * 32) % 256;
            unsigned char b = ((x+y) * 16) % 256;

            image_set_pixel(test_img, x, y, 0, r);
            image_set_pixel(test_img, x, y, 1, g);
            image_set_pixel(test_img, x, y, 2, b);
        }
    }

    // Get and verify pixel values
    printf("Verifying pixel values...\n");
    int errors = 0;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            unsigned char expected_r = (x * 32) % 256;
            unsigned char expected_g = (y * 32) % 256;
            unsigned char expected_b = ((x+y) * 16) % 256;

            unsigned char actual_r = image_get_pixel(test_img, x, y, 0);
            unsigned char actual_g = image_get_pixel(test_img, x, y, 1);
            unsigned char actual_b = image_get_pixel(test_img, x, y, 2);

            if (expected_r != actual_r || expected_g != actual_g || expected_b != actual_b) {
                errors++;
                printf("Error at [%d, %d]: expected (%d, %d, %d), got (%d, %d, %d)\n",
                       x, y, expected_r, expected_g, expected_b, actual_r, actual_g, actual_b);
                if (errors >= 5) {
                    printf("Too many errors, stopping check...\n");
                    break;
                }
            }
        }
        if (errors >= 5) break;
    }

    if (errors == 0) {
        printf("SUCCESS: All pixel values verified correctly\n");
    } else {
        printf("FAILED: %d pixel values incorrect\n", errors);
    }

    // Test out-of-bounds access
    printf("\nTesting out-of-bounds access...\n");
    unsigned char out_of_bounds_value = image_get_pixel(test_img, width + 10, height + 10, 0);
    printf("Out-of-bounds get returned: %d (expected 0)\n", out_of_bounds_value);

    printf("Setting out-of-bounds pixel...\n");
    image_set_pixel(test_img, width + 10, height + 10, 0, 255);
    printf("If no crash occurred, out-of-bounds check works correctly.\n\n");

    // Display image content for verification
    print_image_section(test_img, 0, 0, width, height, "Test image content");

    // Clean up
    image_free(test_img);

    printf("Pixel access test completed\n\n");
}

// Test color conversion between RGB and YCbCr
void test_color_conversion(void) {
    printf("=== Testing Color Conversion ===\n");

    int width = 16, height = 16;

    // Create test pattern
    printf("Creating test pattern...\n");
    Image* rgb_img = create_test_pattern(width, height);
    if (!rgb_img) {
        printf("FAILED: Could not create test pattern\n\n");
        return;
    }

    printf("Test pattern created with color gradients\n");
    print_image_section(rgb_img, 0, 0, 4, 4, "RGB test pattern (top-left corner)");

    // Convert RGB to YCbCr
    printf("Converting RGB to YCbCr...\n");
    Image* ycbcr_img = image_rgb_to_ycbcr(rgb_img);
    if (!ycbcr_img) {
        printf("FAILED: RGB to YCbCr conversion failed\n\n");
        image_free(rgb_img);
        return;
    }

    printf("RGB to YCbCr conversion successful\n");

    // Check a few specific values
    printf("YCbCr values at [0,0]: Y=%d, Cb=%d, Cr=%d\n",
           image_get_y(ycbcr_img, 0, 0),
           image_get_cb(ycbcr_img, 0, 0),
           image_get_cr(ycbcr_img, 0, 0));

    printf("YCbCr values at [%d,%d]: Y=%d, Cb=%d, Cr=%d\n",
           width-1, height-1,
           image_get_y(ycbcr_img, width-1, height-1),
           image_get_cb(ycbcr_img, width-1, height-1),
           image_get_cr(ycbcr_img, width-1, height-1));

    // Convert back to RGB
    printf("Converting YCbCr back to RGB...\n");
    Image* converted_rgb = image_ycbcr_to_rgb(ycbcr_img);
    if (!converted_rgb) {
        printf("FAILED: YCbCr to RGB conversion failed\n\n");
        image_free(rgb_img);
        image_free(ycbcr_img);
        return;
    }

    printf("YCbCr to RGB conversion successful\n");
    print_image_section(converted_rgb, 0, 0, 4, 4, "Converted RGB (top-left corner)");

    // Calculate PSNR between original and converted
    double psnr = calculate_psnr(rgb_img, converted_rgb);
    printf("PSNR between original and converted: %.2f dB\n", psnr);

    if (psnr > 40.0) {
        printf("SUCCESS: High PSNR indicates good conversion quality\n");
    } else if (psnr > 30.0) {
        printf("WARNING: Acceptable PSNR but some quality loss\n");
    } else {
        printf("FAILED: Low PSNR indicates significant quality loss\n");
    }

    // Count exact matches
    int total_pixels = width * height * 3;
    int exact_matches = 0;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            for (int c = 0; c < 3; c++) {
                if (image_get_pixel(rgb_img, x, y, c) == image_get_pixel(converted_rgb, x, y, c)) {
                    exact_matches++;
                }
            }
        }
    }

    printf("Exact pixel value matches: %d out of %d (%.2f%%)\n\n",
           exact_matches, total_pixels, (float)exact_matches / total_pixels * 100);

    // Clean up
    image_free(rgb_img);
    image_free(ycbcr_img);
    image_free(converted_rgb);

    printf("Color conversion test completed\n\n");
}

// Test chroma subsampling
void test_chroma_subsampling(void) {
    printf("=== Testing Chroma Subsampling ===\n");

    int width = 32, height = 32;

    // Create test pattern
    printf("Creating test pattern...\n");
    Image* rgb_img = create_test_pattern(width, height);
    if (!rgb_img) {
        printf("FAILED: Could not create test pattern\n\n");
        return;
    }

    // Convert to YCbCr
    printf("Converting to YCbCr...\n");
    Image* ycbcr_img = image_rgb_to_ycbcr(rgb_img);
    if (!ycbcr_img) {
        printf("FAILED: RGB to YCbCr conversion failed\n\n");
        image_free(rgb_img);
        return;
    }

    printf("Image before subsampling:\n");
    printf("- Cb dimensions: %d x %d\n", ycbcr_img->cb_width, ycbcr_img->cb_height);
    printf("- Cr dimensions: %d x %d\n", ycbcr_img->cr_width, ycbcr_img->cr_height);
    printf("- Subsampling mode: %d\n\n", ycbcr_img->subsampling);

    // Create baseline RGB for comparison
    printf("Creating baseline RGB image...\n");
    Image* baseline_rgb = image_ycbcr_to_rgb(ycbcr_img);
    if (!baseline_rgb) {
        printf("FAILED: Baseline YCbCr to RGB conversion failed\n\n");
        image_free(rgb_img);
        image_free(ycbcr_img);
        return;
    }

    // Try 4:2:2 subsampling with error checking
    printf("Applying 4:2:2 subsampling...\n");
    int result = image_apply_subsampling(ycbcr_img, SUBSAMPLE_422);
    if (result != 0) {
        printf("FAILED: Could not apply 4:2:2 subsampling (error code %d)\n\n", result);
        image_free(rgb_img);
        image_free(ycbcr_img);
        image_free(baseline_rgb);
        return;
    }

    printf("4:2:2 subsampling applied successfully\n");
    printf("Image after 4:2:2 subsampling:\n");
    printf("- Cb dimensions: %d x %d\n", ycbcr_img->cb_width, ycbcr_img->cb_height);
    printf("- Cr dimensions: %d x %d\n", ycbcr_img->cr_width, ycbcr_img->cr_height);
    printf("- Subsampling mode: %d\n\n", ycbcr_img->subsampling);

    // Convert 4:2:2 to RGB
    printf("Converting 4:2:2 YCbCr to RGB...\n");
    Image* rgb_422 = image_ycbcr_to_rgb(ycbcr_img);
    if (!rgb_422) {
        printf("FAILED: 4:2:2 YCbCr to RGB conversion failed\n\n");
        image_free(rgb_img);
        image_free(ycbcr_img);
        image_free(baseline_rgb);
        return;
    }

    printf("4:2:2 YCbCr to RGB conversion successful\n");

    // Calculate PSNR for 4:2:2
    double psnr_422 = calculate_psnr(baseline_rgb, rgb_422);
    printf("PSNR for 4:2:2 subsampling: %.2f dB\n\n", psnr_422);

    // Apply 4:2:0 subsampling
    printf("Applying 4:2:0 subsampling...\n");
    if (image_apply_subsampling(ycbcr_img, SUBSAMPLE_420) != 0) {
        printf("FAILED: Could not apply 4:2:0 subsampling\n\n");
        image_free(rgb_img);
        image_free(ycbcr_img);
        image_free(baseline_rgb);
        image_free(rgb_422);
        return;
    }

    printf("4:2:0 subsampling applied\n");
    printf("Image after 4:2:0 subsampling:\n");
    printf("- Cb dimensions: %d x %d\n", ycbcr_img->cb_width, ycbcr_img->cb_height);
    printf("- Cr dimensions: %d x %d\n", ycbcr_img->cr_width, ycbcr_img->cr_height);
    printf("- Subsampling mode: %d\n", ycbcr_img->subsampling);

    // Convert 4:2:0 to RGB
    printf("Converting 4:2:0 YCbCr to RGB...\n");
    Image* rgb_420 = image_ycbcr_to_rgb(ycbcr_img);
    if (!rgb_420) {
        printf("FAILED: 4:2:0 YCbCr to RGB conversion failed\n\n");
        image_free(rgb_img);
        image_free(ycbcr_img);
        image_free(baseline_rgb);
        image_free(rgb_422);
        return;
    }

    printf("4:2:0 YCbCr to RGB conversion successful\n");

    // Calculate PSNR for 4:2:0
    double psnr_420 = calculate_psnr(baseline_rgb, rgb_420);
    printf("PSNR for 4:2:0 subsampling: %.2f dB\n\n", psnr_420);

    // Return to 4:4:4 (no subsampling)
    printf("Returning to 4:4:4 (no subsampling)...\n");
    if (image_apply_subsampling(ycbcr_img, SUBSAMPLE_444) != 0) {
        printf("FAILED: Could not apply 4:4:4 subsampling\n\n");
        image_free(rgb_img);
        image_free(ycbcr_img);
        image_free(baseline_rgb);
        image_free(rgb_422);
        image_free(rgb_420);
        return;
    }

    printf("4:4:4 subsampling applied\n");
    printf("Image after returning to 4:4:4:\n");
    printf("- Cb dimensions: %d x %d\n", ycbcr_img->cb_width, ycbcr_img->cb_height);
    printf("- Cr dimensions: %d x %d\n", ycbcr_img->cr_width, ycbcr_img->cr_height);
    printf("- Subsampling mode: %d\n", ycbcr_img->subsampling);

    // Convert 4:4:4 to RGB
    printf("Converting restored 4:4:4 YCbCr to RGB...\n");
    Image* rgb_444 = image_ycbcr_to_rgb(ycbcr_img);
    if (!rgb_444) {
        printf("FAILED: Restored 4:4:4 YCbCr to RGB conversion failed\n\n");
        image_free(rgb_img);
        image_free(ycbcr_img);
        image_free(baseline_rgb);
        image_free(rgb_422);
        image_free(rgb_420);
        return;
    }

    printf("Restored 4:4:4 YCbCr to RGB conversion successful\n");

    // Calculate PSNR for restored 4:4:4
    double psnr_444 = calculate_psnr(baseline_rgb, rgb_444);
    printf("PSNR for restored 4:4:4: %.2f dB\n\n", psnr_444);

    // Summary
    printf("Subsampling PSNR Summary:\n");
    printf("- 4:2:2 subsampling: %.2f dB\n", psnr_422);
    printf("- 4:2:0 subsampling: %.2f dB\n", psnr_420);
    printf("- Restored 4:4:4: %.2f dB\n\n", psnr_444);

    // Calculate compression ratios
    int original_size = width * height * 3;
    int size_422 = width * height + (width/2) * height * 2;
    int size_420 = width * height + (width/2) * (height/2) * 2;

    printf("Compression ratios:\n");
    printf("- 4:2:2: %.2f:1 (%.1f%% of original)\n",
           (float)original_size / size_422, (float)size_422 / original_size * 100);
    printf("- 4:2:0: %.2f:1 (%.1f%% of original)\n\n",
           (float)original_size / size_420, (float)size_420 / original_size * 100);

    // Clean up
    image_free(rgb_img);
    image_free(ycbcr_img);
    image_free(baseline_rgb);
    image_free(rgb_422);
    image_free(rgb_420);
    image_free(rgb_444);

    printf("Subsampling test completed\n\n");
}

// Test image crop functionality
void test_image_crop(void) {
    printf("=== Testing Image Cropping ===\n");

    int width = 64, height = 64;

    // Create test pattern
    printf("Creating test pattern...\n");
    Image* img = create_test_pattern(width, height);
    if (!img) {
        printf("FAILED: Could not create test pattern\n\n");
        return;
    }

    printf("Test pattern created with dimensions %d x %d\n", img->width, img->height);

    // Test various crop regions
    int crop_tests[][4] = {
            // x, y, width, height
            {10, 10, 32, 32},   // Center crop
            {0, 0, 16, 16},     // Top-left crop
            {48, 48, 16, 16},   // Bottom-right crop
            {16, 0, 32, 64},    // Full height slice
            {0, 16, 64, 32}     // Full width slice
    };

    int num_tests = sizeof(crop_tests) / sizeof(crop_tests[0]);

    for (int i = 0; i < num_tests; i++) {
        int crop_x = crop_tests[i][0];
        int crop_y = crop_tests[i][1];
        int crop_width = crop_tests[i][2];
        int crop_height = crop_tests[i][3];

        printf("\nTest crop %d: [%d, %d] to [%d, %d] (size: %d x %d)...\n",
               i+1, crop_x, crop_y, crop_x + crop_width - 1, crop_y + crop_height - 1,
               crop_width, crop_height);

        Image* cropped = image_crop(img, crop_x, crop_y, crop_width, crop_height);

        if (!cropped) {
            printf("FAILED: Crop operation failed\n");
            continue;
        }

        printf("Cropped image created with dimensions %d x %d\n", cropped->width, cropped->height);

        // Verify crop dimensions
        if (cropped->width != crop_width || cropped->height != crop_height) {
            printf("FAILED: Cropped image has wrong dimensions\n");
        } else {
            printf("Cropped image dimensions verified\n");
        }

        // Verify pixel values (first few pixels)
        int errors = 0;
        for (int y = 0; y < fmin(4, cropped->height); y++) {
            for (int x = 0; x < fmin(4, cropped->width); x++) {
                for (int c = 0; c < 3; c++) {
                    unsigned char src_val = image_get_pixel(img, crop_x + x, crop_y + y, c);
                    unsigned char crop_val = image_get_pixel(cropped, x, y, c);

                    if (src_val != crop_val) {
                        errors++;
                        if (errors <= 5) {
                            printf("Error at [%d, %d], channel %d: source=%d, cropped=%d\n",
                                   x, y, c, src_val, crop_val);
                        }
                    }
                }
            }
        }

        if (errors == 0) {
            printf("Cropped pixel values verified for sample area\n");
        } else {
            printf("FAILED: %d errors in cropped pixel values\n", errors);
        }

        // Clean up cropped image
        image_free(cropped);
    }

    // Test invalid crop parameters
    printf("\nTesting invalid crop parameters...\n");

    Image* invalid_crop = image_crop(img, -10, 10, 20, 20);
    if (!invalid_crop) {
        printf("SUCCESS: Negative x position correctly rejected\n");
    } else {
        printf("FAILED: Negative x position was accepted\n");
        image_free(invalid_crop);
    }

    invalid_crop = image_crop(img, 10, 10, width + 10, height);
    if (!invalid_crop) {
        printf("SUCCESS: Width beyond image bounds correctly rejected\n");
    } else {
        printf("FAILED: Width beyond image bounds was accepted\n");
        image_free(invalid_crop);
    }

    invalid_crop = image_crop(img, 10, 10, 0, 20);
    if (!invalid_crop) {
        printf("SUCCESS: Zero width correctly rejected\n");
    } else {
        printf("FAILED: Zero width was accepted\n");
        image_free(invalid_crop);
    }

    // Clean up
    image_free(img);

    printf("\nCropping test completed\n\n");
}

// Test image resize functionality
void test_image_resize(void) {
    printf("=== Testing Image Resizing ===\n");

    int width = 64, height = 64;

    // Create test pattern
    printf("Creating test pattern...\n");
    Image* img = create_test_pattern(width, height);
    if (!img) {
        printf("FAILED: Could not create test pattern\n\n");
        return;
    }

    printf("Test pattern created with dimensions %d x %d\n", img->width, img->height);

    // Test various resize operations
    int resize_tests[][2] = {
            // new_width, new_height
            {32, 32},   // Downsize by 50%
            {128, 128}, // Upsize by 200%
            {64, 32},   // Half height
            {32, 64},   // Half width
            {100, 75}   // Arbitrary dimensions
    };

    int num_tests = sizeof(resize_tests) / sizeof(resize_tests[0]);

    for (int i = 0; i < num_tests; i++) {
        int new_width = resize_tests[i][0];
        int new_height = resize_tests[i][1];

        printf("\nTest resize %d: %d x %d to %d x %d (%.1f%% of original area)...\n",
               i+1, width, height, new_width, new_height,
               (float)(new_width * new_height) / (width * height) * 100);

        Image* resized = image_resize(img, new_width, new_height);

        if (!resized) {
            printf("FAILED: Resize operation failed\n");
            continue;
        }

        printf("Resized image created with dimensions %d x %d\n", resized->width, resized->height);

        // Verify resize dimensions
        if (resized->width != new_width || resized->height != new_height) {
            printf("FAILED: Resized image has wrong dimensions\n");
        } else {
            printf("Resized image dimensions verified\n");
        }

        // For downsizing, verify corner pixels (approximately)
        if (new_width < width && new_height < height) {
            int corners[4][2] = {{0,0}, {new_width-1, 0}, {0, new_height-1}, {new_width-1, new_height-1}};
            float scale_x = (float)width / new_width;
            float scale_y = (float)height / new_height;

            printf("Checking corner pixels:\n");
            for (int c = 0; c < 4; c++) {
                int x = corners[c][0];
                int y = corners[c][1];
                int orig_x = (int)(x * scale_x);
                int orig_y = (int)(y * scale_y);

                printf("Corner [%d, %d] -> maps to original around [%d, %d]\n",
                       x, y, orig_x, orig_y);

                printf("  Resized: (%d, %d, %d), Original: (%d, %d, %d)\n",
                       image_get_pixel(resized, x, y, 0),
                       image_get_pixel(resized, x, y, 1),
                       image_get_pixel(resized, x, y, 2),
                       image_get_pixel(img, orig_x, orig_y, 0),
                       image_get_pixel(img, orig_x, orig_y, 1),
                       image_get_pixel(img, orig_x, orig_y, 2));
            }
        }

        // For upsizing, verify
        if (new_width > width || new_height > height) {
            printf("Checking middle pixel for interpolation quality...\n");
            int mid_x = new_width / 2;
            int mid_y = new_height / 2;
            int orig_mid_x = width / 2;
            int orig_mid_y = height / 2;

            printf("Middle resized [%d, %d]: (%d, %d, %d), Original [%d, %d]: (%d, %d, %d)\n",
                   mid_x, mid_y,
                   image_get_pixel(resized, mid_x, mid_y, 0),
                   image_get_pixel(resized, mid_x, mid_y, 1),
                   image_get_pixel(resized, mid_x, mid_y, 2),
                   orig_mid_x, orig_mid_y,
                   image_get_pixel(img, orig_mid_x, orig_mid_y, 0),
                   image_get_pixel(img, orig_mid_x, orig_mid_y, 1),
                   image_get_pixel(img, orig_mid_x, orig_mid_y, 2));
        }

        // Clean up resized image
        image_free(resized);
    }

    // Test with YCbCr image
    printf("\nTesting YCbCr image resizing...\n");
    Image* ycbcr = image_rgb_to_ycbcr(img);
    if (!ycbcr) {
        printf("FAILED: Could not convert to YCbCr\n");
        image_free(img);
        return;
    }

    // Apply subsampling to test if it's preserved
    printf("Applying 4:2:0 subsampling...\n");
    if (image_apply_subsampling(ycbcr, SUBSAMPLE_420) != 0) {
        printf("FAILED: Could not apply subsampling\n");
        image_free(img);
        image_free(ycbcr);
        return;
    }

    // Resize YCbCr image
    int ycbcr_new_width = 48;
    int ycbcr_new_height = 48;
    printf("Resizing YCbCr image from %d x %d to %d x %d...\n",
           ycbcr->width, ycbcr->height, ycbcr_new_width, ycbcr_new_height);

    Image* resized_ycbcr = image_resize(ycbcr, ycbcr_new_width, ycbcr_new_height);
    if (!resized_ycbcr) {
        printf("FAILED: YCbCr resize operation failed\n");
        image_free(img);
        image_free(ycbcr);
        return;
    }

    printf("Resized YCbCr image created\n");
    printf("Checking if subsampling was preserved: mode=%d\n", resized_ycbcr->subsampling);

    if (resized_ycbcr->subsampling == ycbcr->subsampling) {
        printf("SUCCESS: Subsampling mode was preserved\n");
    } else {
        printf("FAILED: Subsampling mode was changed\n");
    }

    // Test invalid resize parameters
    printf("\nTesting invalid resize parameters...\n");

    Image* invalid_resize = image_resize(img, 0, 50);
    if (!invalid_resize) {
        printf("SUCCESS: Zero width correctly rejected\n");
    } else {
        printf("FAILED: Zero width was accepted\n");
        image_free(invalid_resize);
    }

    invalid_resize = image_resize(img, 50, -30);
    if (!invalid_resize) {
        printf("SUCCESS: Negative height correctly rejected\n");
    } else {
        printf("FAILED: Negative height was accepted\n");
        image_free(invalid_resize);
    }

    // Clean up
    image_free(img);
    image_free(ycbcr);
    image_free(resized_ycbcr);

    printf("\nResizing test completed\n\n");
}

// Test BMP file I/O
void test_bmp_io(void) {
    printf("=== Testing BMP I/O ===\n");

    int width = 64, height = 64;
    const char* test_filename = "test_output.bmp";

    // Create test pattern
    printf("Creating test pattern...\n");
    Image* original = create_test_pattern(width, height);
    if (!original) {
        printf("FAILED: Could not create test pattern\n\n");
        return;
    }

    // Save to BMP file
    printf("Saving image to %s...\n", test_filename);
    int save_result = image_save_bmp(original, test_filename);

    if (save_result != 0) {
        printf("FAILED: Could not save BMP file\n\n");
        image_free(original);
        return;
    }

    printf("Image saved successfully\n");

    // Load BMP file
    printf("Loading image from %s...\n", test_filename);
    Image* loaded = image_load_bmp(test_filename);

    if (!loaded) {
        printf("FAILED: Could not load BMP file\n\n");
        image_free(original);
        return;
    }

    printf("Image loaded successfully with dimensions %d x %d\n", loaded->width, loaded->height);

    // Compare dimensions
    if (loaded->width != original->width || loaded->height != original->height) {
        printf("FAILED: Loaded image dimensions (%d x %d) don't match original (%d x %d)\n",
               loaded->width, loaded->height, original->width, original->height);
        image_free(original);
        image_free(loaded);
        return;
    }

    // Compare pixels
    int errors = 0;
    int max_errors_to_report = 5;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            for (int c = 0; c < 3; c++) {
                unsigned char orig_val = image_get_pixel(original, x, y, c);
                unsigned char loaded_val = image_get_pixel(loaded, x, y, c);

                // Allow small differences due to RGB/BGR conversion or rounding
                if (abs(orig_val - loaded_val) > 1) {
                    errors++;
                    if (errors <= max_errors_to_report) {
                        printf("Pixel difference at [%d, %d], channel %d: original=%d, loaded=%d\n",
                               x, y, c, orig_val, loaded_val);
                    }
                }
            }
        }
    }

    if (errors == 0) {
        printf("SUCCESS: All pixel values match between original and loaded image\n");
    } else {
        printf("WARNING: %d pixels differ between original and loaded image\n", errors);
        printf("Note: Small differences are expected due to RGB/BGR conversion\n");
    }

    // Calculate PSNR
    double psnr = calculate_psnr(original, loaded);
    printf("PSNR between original and loaded: %.2f dB\n", psnr);

    if (psnr > 40.0) {
        printf("Image quality: Excellent\n");
    } else if (psnr > 30.0) {
        printf("Image quality: Good\n");
    } else {
        printf("Image quality: Poor\n");
    }

    // Save different formats
    printf("\nTesting BMP save with different formats...\n");

    // Convert to YCbCr and save
    Image* ycbcr = image_rgb_to_ycbcr(original);
    if (ycbcr) {
        printf("Saving YCbCr image as BMP...\n");
        const char* ycbcr_filename = "test_ycbcr.bmp";
        if (image_save_bmp(ycbcr, ycbcr_filename) == 0) {
            printf("SUCCESS: YCbCr image saved as BMP\n");
        } else {
            printf("FAILED: Could not save YCbCr as BMP\n");
        }
        image_free(ycbcr);
    }

    // Make a grayscale image and save
    Image* grayscale = image_create(width, height, 1, 8, FORMAT_GRAYSCALE);
    if (grayscale) {
        // Copy just the red channel to make a quick grayscale image
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                unsigned char val = image_get_pixel(original, x, y, 0);
                image_set_pixel(grayscale, x, y, 0, val);
            }
        }

        printf("Saving grayscale image as BMP...\n");
        const char* gray_filename = "test_gray.bmp";
        if (image_save_bmp(grayscale, gray_filename) == 0) {
            printf("SUCCESS: Grayscale image saved as BMP\n");
        } else {
            printf("FAILED: Could not save grayscale as BMP\n");
        }
        image_free(grayscale);
    }

    // Clean up
    image_free(original);
    image_free(loaded);

    printf("\nBMP I/O test completed\n\n");
}

// Test PNG loading (with the placeholder implementation)
void test_png_loading(void) {
    printf("=== Testing PNG Loading ===\n");

    const char* test_filename = "test_image.png";

    printf("Attempting to load %s...\n", test_filename);
    Image* loaded = image_load_png(test_filename);

    if (!loaded) {
        printf("Expected behavior: PNG loading not implemented in the current version\n");
        printf("Note: This is just to verify the placeholder function works correctly\n");
    } else {
        printf("Unexpected result: PNG loading succeeded despite placeholder implementation\n");
        image_free(loaded);
    }

    printf("\nPNG loading test completed\n\n");
}

int main(void) {
    printf("======================================\n");
    printf("     Image Processing Tests\n");
    printf("======================================\n\n");

    test_image_creation();
    test_pixel_access();
    test_color_conversion();
    test_chroma_subsampling();
    test_image_crop();
    test_image_resize();
    test_bmp_io();
    test_png_loading();

    printf("All tests completed!\n");
    return 0;
}

