/**
 * image.c - Implementation of Image Processing functions
 * Part of Adaptive DCT Image Compressor
 */

#include <image.h>

// BMP file header structures

#pragma pack(push, 1)
typedef struct {
    unsigned short bfType; // BMP signature
    unsigned int bfSize;   // Size of the BMP file
    unsigned short bfReserved1;
    unsigned short bfReserved2;
    unsigned int bfOffBits; // Offset to pixel data
} BMPFileHeader;

typedef struct {
    unsigned int biSize;          // Size of the header (40 bytes)
    int biWidth;                  // Image width
    int biHeight;                 // Image height
    unsigned short biPlanes;      // Number of color planes
    unsigned short biBitCount;    // Number of bits per pixel
    unsigned int biCompression;   // Compression method (0 = uncompressed)
    unsigned int biSizeImage;     // Size of the raw image data (0 = uncompressed)
    int biXPelsPerMeter;          // Horizontal resolution
    int biYPelsPerMeter;          // Vertical resolution
    unsigned int biClrUsed;       // Number of colors in the palette
    unsigned int biClrImportant;  // Number of important colors
} BMPInfoHeader;
#pragma pack(pop)

// Constants for BMP file headers
#define YCBCR_Y_R_FACTOR  0.299
#define YCBCR_Y_G_FACTOR  0.587
#define YCBCR_Y_B_FACTOR  0.114
#define YCBCR_CB_R_FACTOR -0.168736
#define YCBCR_CB_G_FACTOR -0.331264
#define YCBCR_CB_B_FACTOR  0.5
#define YCBCR_CR_R_FACTOR  0.5
#define YCBCR_CR_G_FACTOR -0.418688
#define YCBCR_CR_B_FACTOR -0.081312

// YCbCr offset values (for 8-bit)
#define YCBCR_Y_OFFSET   0
#define YCBCR_CB_OFFSET  128
#define YCBCR_CR_OFFSET  128

// RGB to YCbCr conversion macros
#define RGB_TO_Y(R, G, B) ((unsigned char)fmin(fmax(round(YCBCR_Y_R_FACTOR * (R) + YCBCR_Y_G_FACTOR * (G) + YCBCR_Y_B_FACTOR * (B) + YCBCR_Y_OFFSET), 0), 255))
#define RGB_TO_CB(R, G, B) ((unsigned char)fmin(fmax(round(YCBCR_CB_R_FACTOR * (R) + YCBCR_CB_G_FACTOR * (G) + YCBCR_CB_B_FACTOR * (B) + YCBCR_CB_OFFSET), 0), 255))
#define RGB_TO_CR(R, G, B) ((unsigned char)fmin(fmax(round(YCBCR_CR_R_FACTOR * (R) + YCBCR_CR_G_FACTOR * (G) + YCBCR_CR_B_FACTOR * (B) + YCBCR_CR_OFFSET), 0), 255))

// YCbCr to RGB conversion macros
#define Y_CB_CR_TO_R(Y, CB, CR) ((unsigned char)fmin(fmax(round(Y + 1.402 * (CR - YCBCR_CR_OFFSET)), 0), 255))
#define Y_CB_CR_TO_G(Y, CB, CR) ((unsigned char)fmin(fmax(round(Y - 0.344136 * (CB - YCBCR_CB_OFFSET) - 0.714136 * (CR - YCBCR_CR_OFFSET)), 0), 255))
#define Y_CB_CR_TO_B(Y, CB, CR) ((unsigned char)fmin(fmax(round(Y + 1.772 * (CB - YCBCR_CB_OFFSET)), 0), 255))


/*
 * Create new empty image
 */

Image *image_create(int width, int height, int channels, int bit_depth, ImageFormat format) {
    if (width <= 0 || height <= 0 || channels <= 0 ||
        (bit_depth != 8 && bit_depth != 16) ||
        (format != FORMAT_RGB && format != FORMAT_YCBCR && format != FORMAT_GRAYSCALE)) {
        fprintf(stderr, "Invalid image parameters\n");
        return NULL;
    }

    Image *img = (Image *) malloc(sizeof(Image));
    if (!img) {
        fprintf(stderr, "Memory allocation failed, when creating new image\n");
        return NULL;
    }

    img->width = width;
    img->height = height;
    img->channels = channels;
    img->bit_depth = bit_depth;
    img->format = format;

    // Calculate image size in bytes
    int bytes_per_pixel = (bit_depth + 7) / 8;
    int data_size = width * height * channels * bytes_per_pixel;

    img->data = (unsigned char *) calloc(data_size, sizeof(unsigned char));
    if (!img->data) {
        fprintf(stderr, "Memory allocation failed, when creating new image data\n");
        free(img);
        return NULL;
    }

    // Initialize YCbCr channels
    img->y_channel = NULL;
    img->cb_channel = NULL;
    img->cr_channel = NULL;
    img->cb_width = width;
    img->cb_height = height;
    img->cr_width = width;
    img->cr_height = height;
    img->subsampling = SUBSAMPLE_444;

    if (format == FORMAT_YCBCR && channels == 3) {
        img->y_channel = img->data;
        img->cb_channel = img->data + (width * height);
        img->cr_channel = img->data + (width * height * 2);
    }

    return img;
}

/*
 * Free image resources
 */
void image_free(Image *img) {
    if (!img) {
        return;
    }

    if (img->data) {
        free(img->data);
    }

    free(img);
}

/*
 * Load image from BMP file
 */

Image *image_load_bmp(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Failed to open file: %s\n", filename);
        return NULL;
    }

    // Read BMP file header
    BMPFileHeader file_header;
    BMPInfoHeader info_header;

    if (fread(&file_header, sizeof(BMPFileHeader), 1, file) != 1 ||
        fread(&info_header, sizeof(BMPInfoHeader), 1, file) != 1) {
        fprintf(stderr, "Failed to read BMP file headers\n");
        fclose(file);
        return NULL;
    }

    if (file_header.bfType != 0x4D42) {
        fprintf(stderr, "Invalid BMP file signature\n");
        fclose(file);
        return NULL;
    }

    if (info_header.biBitCount != 24) {
        fprintf(stderr, "Unsupported BMP bit depth: %d\n", info_header.biBitCount);
        fclose(file);
        return NULL;
    }

    int width = info_header.biWidth;
    int height = abs(info_header.biHeight);
    Image *img = image_create(width, height, 3, 8, FORMAT_RGB);
    if (!img) {
        fclose(file);
        return NULL;
    }

    fseek(file, file_header.bfOffBits, SEEK_SET);

    int row_size = ((width * 3 + 3) / 4) * 4;
    int padding = row_size - width * 3;

    unsigned *buffer = (unsigned *) malloc(row_size);
    if (!buffer) {
        fprintf(stderr, "Memory allocation failed, when reading BMP file\n");
        image_free(img);
        fclose(file);
        return NULL;
    }

    int is_bottom_up = info_header.biHeight > 0;

    for (int y = 0; y < height; ++y) {
        int row = is_bottom_up ? height - y - 1 : y;

        if (fread(buffer, row_size, 1, file) != 1) {
            fprintf(stderr, "Error reading BMP pixel data\n");
            free(buffer);
            image_free(img);
            fclose(file);
            return NULL;
        }

        for (int x = 0; x < width; ++x) {
            unsigned char b = buffer[x * 3 + 0];
            unsigned char g = buffer[x * 3 + 1];
            unsigned char r = buffer[x * 3 + 2];

            image_set_pixel(img, x, row, 0, r);
            image_set_pixel(img, x, row, 1, g);
            image_set_pixel(img, x, row, 2, b);
        }
    }

    free(buffer);
    fclose(file);
    return img;
}

/*
 * Save image to BMP file
 */

int image_save_bmp(Image *img, const char *filename) {
    if (!img || !filename) {
        fprintf(stderr, "Invalid image or filename\n");
        return -1;
    }

    Image *rgb_img = img;
    int should_free_rgb = 0;

    // Convert YCbCr to RGB if needed
    if (img->format == FORMAT_YCBCR) {
        rgb_img = image_ycbcr_to_rgb(img);
        if (!rgb_img) {
            return -1;
        }
        should_free_rgb = 1;
    } else if (img->format == FORMAT_GRAYSCALE) {
        // For grayscale, create an RGB image where R=G=B=gray value
        rgb_img = image_create(img->width, img->height, 3, img->bit_depth, FORMAT_RGB);
        if (!rgb_img) {
            return -1;
        }
        should_free_rgb = 1;

        for (int y = 0; y < img->height; y++) {
            for (int x = 0; x < img->width; x++) {
                unsigned char gray = image_get_pixel(img, x, y, 0);
                image_set_pixel(rgb_img, x, y, 0, gray); // R
                image_set_pixel(rgb_img, x, y, 1, gray); // G
                image_set_pixel(rgb_img, x, y, 2, gray); // B
            }
        }
    }

    FILE *file = fopen(filename, "wb");
    if (!file) {
        fprintf(stderr, "Failed to open file for writing: %s\n", filename);
        if (should_free_rgb) {
            image_free(rgb_img);
        }
        return -1;
    }

    int row_size = ((rgb_img->width * 3 + 3) / 4) * 4;
    int padding = row_size - (rgb_img->width * 3);

    int pixel_data_size = row_size * rgb_img->height;
    int file_size = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader) + pixel_data_size;

    BMPFileHeader file_header = {
            .bfType = 0x4D42,
            .bfSize = file_size,
            .bfReserved1 = 0,
            .bfReserved2 = 0,
            .bfOffBits = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader)
    };

    fwrite(&file_header, sizeof(BMPFileHeader), 1, file);

    BMPInfoHeader info_header = {
            .biSize = sizeof(BMPInfoHeader),
            .biWidth = rgb_img->width,
            .biHeight = rgb_img->height,
            .biPlanes = 1,
            .biBitCount = 24,
            .biCompression = 0,
            .biSizeImage = pixel_data_size,
            .biXPelsPerMeter = 2835, // 72 DPI
            .biYPelsPerMeter = 2835,
            .biClrUsed = 0,
            .biClrImportant = 0
    };

    fwrite(&info_header, sizeof(BMPInfoHeader), 1, file);

    unsigned char *row_buffer = (unsigned char *) malloc(row_size);
    if (!row_buffer) {
        fclose(file);
        if (should_free_rgb) {
            image_free(rgb_img);
        }
        return -1;
    }

    // Initialize padding bytes to zero
    for (int i = 0; i < padding; i++) {
        row_buffer[rgb_img->width * 3 + i] = 0;
    }

    for (int y = rgb_img->height - 1; y >= 0; y--) {
        for (int x = 0; x < rgb_img->width; x++) {
            row_buffer[x * 3 + 2] = image_get_pixel(rgb_img, x, y, 0); // R
            row_buffer[x * 3 + 1] = image_get_pixel(rgb_img, x, y, 1); // G
            row_buffer[x * 3 + 0] = image_get_pixel(rgb_img, x, y, 2); // B
        }

        fwrite(row_buffer, row_size, 1, file);
    }

    free(row_buffer);
    fclose(file);

    if (should_free_rgb) {
        image_free(rgb_img);
    }

    return 0;
}

/*
 * Get pixel value at specified coordinates
 */
unsigned  char image_get_pixel(Image *img, int x, int y, int channel) {
    if(!img || x < 0 || y < 0 || x >= img->width || y >= img->height || channel < 0 || channel >= img->channels) {
        return 0;
    }

    //int bytes_per_pixel = (img->bit_depth + 7) / 8;
    int index = (y * img->width + x) * img->channels + channel;

    return img->data[index];
}

/*
 * Set pixel value at specified coordinates
 */
void image_set_pixel(Image *img, int x, int y, int channel, unsigned char value) {
    if (!img || x < 0 || y < 0 || x >= img->width || y >= img->height || channel < 0 || channel >= img->channels) {
        return;
    }

    //int bytes_per_pixel = (img->bit_depth + 7) / 8;
    int index = (y * img->width + x) * img->channels + channel;

    img->data[index] = value;
}

/*
 * Get Y (luma) value at specified coordinates
 */
unsigned char image_get_y(Image *img, int x, int y) {
    if (!img || img->format != FORMAT_YCBCR || !img->y_channel ||
        x < 0 || y < 0 || x >= img->width || y >= img->height) {
        return 0;
    }

    return img->y_channel[y * img->width + x];
}

/*
 * Get Cb (blue difference) value at specified coordinates
 */
unsigned char image_get_cb(Image *img, int x, int y) {
    if (!img || img->format != FORMAT_YCBCR || !img->cb_channel ||
        x < 0 || y < 0 || x >= img->cb_width || y >= img->cb_height) {
        return 128;
    }

    switch (img->subsampling) {
        case SUBSAMPLE_444:
            if (x < 0 || y < 0 || x >= img->width || y >= img->height) {
                return 128;
            }
            return img->cb_channel[y * img->width + x];

        case SUBSAMPLE_422:
            if (x < 0 || y < 0 || x >= img->width || y >= img->height) {
                return 128;
            }
            return img->cb_channel[y * img->cb_width + (x / 2)];

        case SUBSAMPLE_420:
            if (x < 0 || y < 0 || x >= img->width || y >= img->height) {
                return 128;
            }
            return img->cb_channel[(y / 2) * img->cb_width + (x / 2)];

        default:
            return 128;
    }
}

/*
 * Get Cr (red difference) value at specified coordinates
 */
unsigned char image_get_cr(Image *img, int x, int y) {
    if (!img || img->format != FORMAT_YCBCR || !img->cr_channel) {
        return 128;  // Default midpoint for Cr
    }

    // Handle subsampling
    switch (img->subsampling) {
        case SUBSAMPLE_444:
            // No subsampling, 1:1 mapping
            if (x < 0 || y < 0 || x >= img->width || y >= img->height) {
                return 128;
            }
            return img->cr_channel[y * img->cr_width + x];

        case SUBSAMPLE_422:
            // Horizontal subsampling (half width)
            if (x < 0 || y < 0 || x >= img->width || y >= img->height) {
                return 128;
            }
            return img->cr_channel[y * img->cr_width + (x / 2)];

        case SUBSAMPLE_420:
            // Horizontal and vertical subsampling (quarter size)
            if (x < 0 || y < 0 || x >= img->width || y >= img->height) {
                return 128;
            }
            return img->cr_channel[(y / 2) * img->cr_width + (x / 2)];

        default:
            return 128;
    }
}

/*
 * Convert RGB image to YCbCr color space
 */
Image* image_rgb_to_ycbcr(Image *rgb) {
    if (!rgb || rgb->format != FORMAT_RGB || rgb->channels != 3) {
        fprintf(stderr, "Invalid input for RGB to YCbCr conversion\n");
        return NULL;
    }

    Image *ycbcr = image_create(rgb->width, rgb->height, 3, rgb->bit_depth, FORMAT_YCBCR);
    if (!ycbcr) {
        return NULL;
    }

    for (int y = 0; y < rgb->height; ++y) {
        for (int x = 0; x < rgb->width; ++x) {
            unsigned char r = image_get_pixel(rgb, x, y, 0);
            unsigned char g = image_get_pixel(rgb, x, y, 1);
            unsigned char b = image_get_pixel(rgb, x, y, 2);

            // Calculate YCbCr values
            unsigned char y_val = RGB_TO_Y(r, g, b);
            unsigned char cb_val = RGB_TO_CB(r, g, b);
            unsigned char cr_val = RGB_TO_CR(r, g, b);

            // Store values in separate Y, Cb, Cr channels
            ycbcr->y_channel[y * ycbcr->width + x] = y_val;
            ycbcr->cb_channel[y * ycbcr->width + x] = cb_val;
            ycbcr->cr_channel[y * ycbcr->width + x] = cr_val;
        }
    }

    return ycbcr;
}

/*
 * Convert YCbCr image to RGB color space
 */