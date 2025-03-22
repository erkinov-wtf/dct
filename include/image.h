/**
 * image.h - Header file for Image Processing implementation
 * Part of Adaptive DCT Image Compressor
 */

#ifndef IMAGE_H
#define IMAGE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <utils.h>

// Image format enumeration
typedef enum {
    FORMAT_RGB,     // RGB color format
    FORMAT_YCBCR,   // YCbCr color format
    FORMAT_GRAYSCALE // Single channel grayscale
} ImageFormat;

// Color subsampling options
typedef enum {
    SUBSAMPLE_444,  // No chroma subsampling (4:4:4)
    SUBSAMPLE_422,  // Horizontal subsampling (4:2:2)
    SUBSAMPLE_420   // Horizontal and vertical subsampling (4:2:0)
} SubsamplingMode;

// Image structure
typedef struct {
    int width;              // Image width in pixels
    int height;             // Image height in pixels
    int channels;           // Number of color channels
    int bit_depth;          // Bits per pixel (8, 16, etc.)
    ImageFormat format;     // Color format
    unsigned char *data;    // Raw pixel data

    // For YCbCr with subsampling
    unsigned char *y_channel;   // Y (luma) channel
    unsigned char *cb_channel;  // Cb (blue difference) channel
    unsigned char *cr_channel;  // Cr (red difference) channel
    int cb_width;              // Width of Cb channel after subsampling
    int cb_height;             // Height of Cb channel after subsampling
    int cr_width;              // Width of Cr channel after subsampling
    int cr_height;             // Height of Cr channel after subsampling
    SubsamplingMode subsampling; // Current subsampling mode
} Image;

/**
 * Create a new empty image
 *
 * @param width Image width in pixels
 * @param height Image height in pixels
 * @param channels Number of color channels
 * @param bit_depth Bits per pixel
 * @param format Color format
 * @return Newly allocated image or NULL on failure
 */
Image* image_create(int width, int height, int channels, int bit_depth, ImageFormat format);

/**
 * Free image resources
 *
 * @param img Image to free
 */
void image_free(Image *img);

/**
 * Load image from BMP file
 *
 * @param filename Path to BMP file
 * @return Loaded image or NULL on failure
 */
Image* image_load_bmp(const char *filename);

/**
 * Load image from PNG file
 * Note: This will require linking with libpng
 *
 * @param filename Path to PNG file
 * @return Loaded image or NULL on failure
 */
Image* image_load_png(const char *filename);

/**
 * Save image to BMP file
 *
 * @param img Image to save
 * @param filename Path to output file
 * @return 0 on success, non-zero on failure
 */
int image_save_bmp(Image *img, const char *filename);

/**
 * Convert RGB image to YCbCr color space
 *
 * @param rgb Input RGB image
 * @return New image in YCbCr format or NULL on failure
 */
Image* image_rgb_to_ycbcr(Image *rgb);

/**
 * Convert YCbCr image to RGB color space
 *
 * @param ycbcr Input YCbCr image
 * @return New image in RGB format or NULL on failure
 */
Image* image_ycbcr_to_rgb(Image *ycbcr);

/**
 * Apply chroma subsampling to YCbCr image
 *
 * @param ycbcr Input YCbCr image
 * @param mode Subsampling mode (4:4:4, 4:2:2, or 4:2:0)
 * @return 0 on success, non-zero on failure
 */
int image_apply_subsampling(Image *ycbcr, SubsamplingMode mode);

/**
 * Get pixel value at specified coordinates
 *
 * @param img Image to read from
 * @param x X coordinate
 * @param y Y coordinate
 * @param channel Color channel (0 = R/Y, 1 = G/Cb, 2 = B/Cr)
 * @return Pixel value or 0 if coordinates are out of bounds
 */
unsigned char image_get_pixel(Image *img, int x, int y, int channel);

/**
 * Set pixel value at specified coordinates
 *
 * @param img Image to modify
 * @param x X coordinate
 * @param y Y coordinate
 * @param channel Color channel (0 = R/Y, 1 = G/Cb, 2 = B/Cr)
 * @param value New pixel value
 */
void image_set_pixel(Image *img, int x, int y, int channel, unsigned char value);

/**
 * Get Y (luma) value at specified coordinates
 *
 * @param img YCbCr image
 * @param x X coordinate
 * @param y Y coordinate
 * @return Y value or 0 if coordinates are out of bounds
 */
unsigned char image_get_y(Image *img, int x, int y);

/**
 * Get Cb (blue difference) value at specified coordinates
 * Handles subsampling automatically
 *
 * @param img YCbCr image
 * @param x X coordinate in original image space
 * @param y Y coordinate in original image space
 * @return Cb value or 128 if coordinates are out of bounds
 */
unsigned char image_get_cb(Image *img, int x, int y);

/**
 * Get Cr (red difference) value at specified coordinates
 * Handles subsampling automatically
 *
 * @param img YCbCr image
 * @param x X coordinate in original image space
 * @param y Y coordinate in original image space
 * @return Cr value or 128 if coordinates are out of bounds
 */
unsigned char image_get_cr(Image *img, int x, int y);

/**
 * Crop an image to the specified dimensions
 *
 * @param img Source image
 * @param x Starting X coordinate
 * @param y Starting Y coordinate
 * @param width Width of crop region
 * @param height Height of crop region
 * @return New cropped image or NULL on failure
 */
Image* image_crop(Image *img, int x, int y, int width, int height);

/**
 * Resize an image to new dimensions
 * Uses bilinear interpolation for better quality
 *
 * @param img Source image
 * @param new_width New width
 * @param new_height New height
 * @return Resized image or NULL on failure
 */
Image* image_resize(Image *img, int new_width, int new_height);

#endif /* IMAGE_H */

