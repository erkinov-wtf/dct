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

// PNG file header and chunk structures
#define PNG_SIGNATURE_SIZE 8
const unsigned char PNG_SIGNATURE[PNG_SIGNATURE_SIZE] = {137, 80, 78, 71, 13, 10, 26, 10};

#pragma pack(push, 1)
typedef struct {
    unsigned int length;      // Length of the data field
    unsigned char type[4];    // Chunk type
    // Data follows here (variable length)
    // CRC follows after data (4 bytes)
} PNGChunkHeader;

typedef struct {
    unsigned char width[4];           // Width (big-endian)
    unsigned char height[4];          // Height (big-endian)
    unsigned char bit_depth;          // Bit depth (1, 2, 4, 8, or 16)
    unsigned char color_type;         // Color type (0, 2, 3, 4, or 6)
    unsigned char compression_method; // Compression method (0)
    unsigned char filter_method;      // Filter method (0)
    unsigned char interlace_method;   // Interlace method (0 or 1)
} PNGIHDRData;
#pragma pack(pop)

// PNG color types
#define PNG_COLOR_TYPE_GRAYSCALE 0
#define PNG_COLOR_TYPE_RGB 2
#define PNG_COLOR_TYPE_PALETTE 3
#define PNG_COLOR_TYPE_GRAYSCALE_ALPHA 4
#define PNG_COLOR_TYPE_RGBA 6

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

// Utility functions for PNG processing
static unsigned int read_uint32_be(const unsigned char *buffer) {
    return (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];
}

static unsigned int calculate_crc32(const unsigned char *data, size_t length) {
    // CRC-32 implementation for PNG chunks
    static unsigned int crc_table[256];
    static int crc_table_computed = 0;
    unsigned int crc = 0xffffffff;

    // Generate CRC table if it hasn't been computed yet
    if (!crc_table_computed) {
        for (int i = 0; i < 256; i++) {
            unsigned int c = i;
            for (int j = 0; j < 8; j++) {
                c = (c & 1) ? (0xEDB88320 ^ (c >> 1)) : (c >> 1);
            }
            crc_table[i] = c;
        }
        crc_table_computed = 1;
    }

    // Calculate CRC for the data
    for (size_t i = 0; i < length; i++) {
        crc = crc_table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    }

    return crc ^ 0xffffffff;
}

// Decompression function - simplified for example
// In a complete implementation, you would use zlib or implement your own deflate algorithm
static int decompress_data(const unsigned char *compressed, size_t compressed_size,
                           unsigned char *decompressed, size_t decompressed_size) {
    // This is a placeholder for zlib decompression
    // In a real implementation, you would:
    // 1. Initialize a zlib stream
    // 2. Process the compressed data
    // 3. Write to the decompressed buffer

    fprintf(stderr, "Zlib decompression not implemented yet\n");
    return -1;

    // Sample code for using zlib would be:
    /*
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = compressed_size;
    strm.next_in = compressed;
    strm.avail_out = decompressed_size;
    strm.next_out = decompressed;

    inflateInit(&strm);
    inflate(&strm, Z_FINISH);
    inflateEnd(&strm);

    return decompressed_size - strm.avail_out;
    */
}

// PNG filter reconstruction functions
static void png_unfilter_scanline(unsigned char filter_type, unsigned char *line,
                                  const unsigned char *prev_line,
                                  int bytes_per_pixel, int width) {
    int i;

    // Apply the appropriate unfilter based on filter_type
    switch (filter_type) {
        case 0: // None
            // No filtering, data already in place
            break;

        case 1: // Sub
            // Each byte is predicted by the byte to its left
            for (i = bytes_per_pixel; i < width; i++) {
                line[i] += line[i - bytes_per_pixel];
            }
            break;

        case 2: // Up
            // Each byte is predicted by the byte above it
            if (prev_line) {
                for (i = 0; i < width; i++) {
                    line[i] += prev_line[i];
                }
            }
            break;

        case 3: // Average
            // Each byte is predicted by the average of the byte to its left and the byte above it
            if (prev_line) {
                for (i = 0; i < bytes_per_pixel; i++) {
                    line[i] += prev_line[i] / 2;
                }
                for (i = bytes_per_pixel; i < width; i++) {
                    line[i] += (line[i - bytes_per_pixel] + prev_line[i]) / 2;
                }
            } else {
                for (i = bytes_per_pixel; i < width; i++) {
                    line[i] += line[i - bytes_per_pixel] / 2;
                }
            }
            break;

        case 4: // Paeth
            // Each byte is predicted using a linear function of the byte to its left, the byte above it, and the byte above and to its left
            if (prev_line) {
                for (i = 0; i < bytes_per_pixel; i++) {
                    line[i] += prev_line[i];
                }
                for (i = bytes_per_pixel; i < width; i++) {
                    int a = line[i - bytes_per_pixel];  // Left
                    int b = prev_line[i];              // Above
                    int c = prev_line[i - bytes_per_pixel]; // Above-left
                    int p = a + b - c;                 // Initial estimate
                    int pa = abs(p - a);              // Distance to a
                    int pb = abs(p - b);              // Distance to b
                    int pc = abs(p - c);              // Distance to c

                    // Find nearest of a, b, c
                    if (pa <= pb && pa <= pc) {
                        line[i] += a;
                    } else if (pb <= pc) {
                        line[i] += b;
                    } else {
                        line[i] += c;
                    }
                }
            } else {
                for (i = bytes_per_pixel; i < width; i++) {
                    line[i] += line[i - bytes_per_pixel];
                }
            }
            break;

        default:
            fprintf(stderr, "Unknown filter type: %d\n", filter_type);
    }
}

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
 * Load image from PNG file
 * Custom implementation without using libpng
 */
Image* image_load_png(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Failed to open file: %s\n", filename);
        return NULL;
    }

    // Check PNG signature
    unsigned char signature[PNG_SIGNATURE_SIZE];
    if (fread(signature, 1, PNG_SIGNATURE_SIZE, file) != PNG_SIGNATURE_SIZE ||
        memcmp(signature, PNG_SIGNATURE, PNG_SIGNATURE_SIZE) != 0) {
        fprintf(stderr, "Invalid PNG signature\n");
        fclose(file);
        return NULL;
    }

    // Variables to store PNG metadata
    int width = 0, height = 0;
    int bit_depth = 0;
    int color_type = 0;
    int bytes_per_pixel = 0;
    int has_alpha = 0;

    // Buffer for chunk reading
    unsigned char chunk_header[8]; // Length (4) + Type (4)
    PNGIHDRData ihdr_data;
    unsigned char *compressed_data = NULL;
    size_t compressed_size = 0;
    unsigned char *decompressed_data = NULL;
    unsigned int crc;

    // Read chunks until we find IHDR
    if (fread(chunk_header, 1, 8, file) != 8) {
        fprintf(stderr, "Failed to read PNG chunk header\n");
        fclose(file);
        return NULL;
    }

    unsigned int chunk_length = read_uint32_be(chunk_header);

    // First chunk must be IHDR
    if (chunk_length != 13 || memcmp(chunk_header + 4, "IHDR", 4) != 0) {
        fprintf(stderr, "First PNG chunk must be IHDR\n");
        fclose(file);
        return NULL;
    }

    // Read IHDR data
    if (fread(&ihdr_data, 1, 13, file) != 13) {
        fprintf(stderr, "Failed to read IHDR chunk data\n");
        fclose(file);
        return NULL;
    }

    // Skip CRC
    fseek(file, 4, SEEK_CUR);

    // Parse IHDR data
    width = read_uint32_be(ihdr_data.width);
    height = read_uint32_be(ihdr_data.height);
    bit_depth = ihdr_data.bit_depth;
    color_type = ihdr_data.color_type;

    if (ihdr_data.compression_method != 0 || ihdr_data.filter_method != 0 ||
        (ihdr_data.interlace_method != 0 && ihdr_data.interlace_method != 1)) {
        fprintf(stderr, "Unsupported PNG features\n");
        fclose(file);
        return NULL;
    }

    // Determine channels and bytes per pixel
    switch (color_type) {
        case PNG_COLOR_TYPE_GRAYSCALE:
            bytes_per_pixel = (bit_depth + 7) / 8;
            has_alpha = 0;
            break;
        case PNG_COLOR_TYPE_RGB:
            bytes_per_pixel = 3 * ((bit_depth + 7) / 8);
            has_alpha = 0;
            break;
        case PNG_COLOR_TYPE_GRAYSCALE_ALPHA:
            bytes_per_pixel = 2 * ((bit_depth + 7) / 8);
            has_alpha = 1;
            break;
        case PNG_COLOR_TYPE_RGBA:
            bytes_per_pixel = 4 * ((bit_depth + 7) / 8);
            has_alpha = 1;
            break;
        default:
            fprintf(stderr, "Unsupported PNG color type: %d\n", color_type);
            fclose(file);
            return NULL;
    }

    // Read all IDAT chunks and concatenate them
    while (1) {
        // Read chunk header
        if (fread(chunk_header, 1, 8, file) != 8) {
            break; // End of file
        }

        chunk_length = read_uint32_be(chunk_header);

        if (memcmp(chunk_header + 4, "IDAT", 4) == 0) {
            // Allocate or resize the compressed data buffer
            unsigned char *new_buffer = realloc(compressed_data,
                                                compressed_size + chunk_length);
            if (!new_buffer) {
                fprintf(stderr, "Memory allocation failed for PNG data\n");
                free(compressed_data);
                fclose(file);
                return NULL;
            }
            compressed_data = new_buffer;

            // Read chunk data
            if (fread(compressed_data + compressed_size, 1, chunk_length, file) != chunk_length) {
                fprintf(stderr, "Failed to read PNG IDAT chunk\n");
                free(compressed_data);
                fclose(file);
                return NULL;
            }

            compressed_size += chunk_length;

            // Skip CRC
            fseek(file, 4, SEEK_CUR);
        }
        else if (memcmp(chunk_header + 4, "IEND", 4) == 0) {
            // End of PNG file
            break;
        }
        else {
            // Skip unknown chunk
            fseek(file, chunk_length + 4, SEEK_CUR); // Skip data and CRC
        }
    }

    fclose(file);

    if (!compressed_data || compressed_size == 0) {
        fprintf(stderr, "No image data found in PNG file\n");
        free(compressed_data);
        return NULL;
    }

    // Create our image structure
    int channels = color_type == PNG_COLOR_TYPE_GRAYSCALE ? 1 :
                   (color_type == PNG_COLOR_TYPE_GRAYSCALE_ALPHA ? 2 :
                    (color_type == PNG_COLOR_TYPE_RGB ? 3 : 4));

    Image *img = image_create(width, height, has_alpha ? channels : 3,
                              bit_depth > 8 ? 16 : 8, FORMAT_RGB);
    if (!img) {
        free(compressed_data);
        return NULL;
    }

    // Calculate decompressed data size (including filter bytes)
    size_t bytes_per_scanline = 1 + width * bytes_per_pixel; // +1 for filter type byte
    size_t decompressed_size = height * bytes_per_scanline;

    // Allocate memory for decompressed data
    decompressed_data = malloc(decompressed_size);
    if (!decompressed_data) {
        fprintf(stderr, "Memory allocation failed for decompressed PNG data\n");
        free(compressed_data);
        image_free(img);
        return NULL;
    }

    // Decompress the image data
    // Note: This is where you would implement/call your own zlib decompression
    if (decompress_data(compressed_data, compressed_size,
                        decompressed_data, decompressed_size) < 0) {
        fprintf(stderr, "Failed to decompress PNG data\n");
        free(compressed_data);
        free(decompressed_data);
        image_free(img);
        return NULL;
    }

    free(compressed_data); // No longer needed

    // Process scanlines (unfilter and convert to RGB)
    unsigned char *prev_line = NULL;
    unsigned char *curr_line = malloc(bytes_per_scanline - 1); // excluding filter byte

    if (!curr_line) {
        fprintf(stderr, "Memory allocation failed for PNG scanline\n");
        free(decompressed_data);
        image_free(img);
        return NULL;
    }

    for (int y = 0; y < height; y++) {
        unsigned char *scanline = decompressed_data + y * bytes_per_scanline;
        unsigned char filter_type = scanline[0];

        // Copy scanline data (excluding filter byte)
        memcpy(curr_line, scanline + 1, bytes_per_scanline - 1);

        // Apply unfilter
        png_unfilter_scanline(filter_type, curr_line, prev_line,
                              bytes_per_pixel, bytes_per_scanline - 1);

        // Now convert pixel data to our image format
        for (int x = 0; x < width; x++) {
            // Position in the scanline
            int pos = x * bytes_per_pixel;

            if (color_type == PNG_COLOR_TYPE_GRAYSCALE) {
                // Grayscale -> RGB
                unsigned char gray = curr_line[pos];
                image_set_pixel(img, x, y, 0, gray); // R
                image_set_pixel(img, x, y, 1, gray); // G
                image_set_pixel(img, x, y, 2, gray); // B
            }
            else if (color_type == PNG_COLOR_TYPE_RGB) {
                // RGB
                image_set_pixel(img, x, y, 0, curr_line[pos]);     // R
                image_set_pixel(img, x, y, 1, curr_line[pos + 1]); // G
                image_set_pixel(img, x, y, 2, curr_line[pos + 2]); // B
            }
            else if (color_type == PNG_COLOR_TYPE_RGBA) {
                // RGBA -> RGB (discard alpha)
                image_set_pixel(img, x, y, 0, curr_line[pos]);     // R
                image_set_pixel(img, x, y, 1, curr_line[pos + 1]); // G
                image_set_pixel(img, x, y, 2, curr_line[pos + 2]); // B
                // Alpha ignored
            }
            else if (color_type == PNG_COLOR_TYPE_GRAYSCALE_ALPHA) {
                // Grayscale with alpha -> RGB (discard alpha)
                unsigned char gray = curr_line[pos];
                image_set_pixel(img, x, y, 0, gray); // R
                image_set_pixel(img, x, y, 1, gray); // G
                image_set_pixel(img, x, y, 2, gray); // B
                // Alpha ignored
            }
        }

        // Current line becomes previous line for the next iteration
        prev_line = curr_line;
    }

    free(curr_line);
    free(decompressed_data);

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
Image* image_ycbcr_to_rgb(Image *ycbcr) {
    if (!ycbcr || ycbcr->format != FORMAT_YCBCR || ycbcr->channels != 3) {
        fprintf(stderr, "Invalid input for YCbCr to RGB conversion\n");
        return NULL;
    }

    Image *rgb = image_create(ycbcr->width, ycbcr->height, 3, ycbcr->bit_depth, FORMAT_RGB);
    if (!rgb) {
        return NULL;
    }

    for (int y = 0; y < ycbcr->height; ++y) {
        for (int x = 0; x < ycbcr->width; ++x) {
            // Get YCbCr values considering subsampling
            unsigned char y_val = image_get_y(ycbcr, x, y);
            unsigned char cb_val = image_get_cb(ycbcr, x, y);
            unsigned char cr_val = image_get_cr(ycbcr, x, y);

            // Convert to RGB
            unsigned char r = Y_CB_CR_TO_R(y_val, cb_val, cr_val);
            unsigned char g = Y_CB_CR_TO_G(y_val, cb_val, cr_val);
            unsigned char b = Y_CB_CR_TO_B(y_val, cb_val, cr_val);

            // Store RGB values
            image_set_pixel(rgb, x, y, 0, r);
            image_set_pixel(rgb, x, y, 1, g);
            image_set_pixel(rgb, x, y, 2, b);
        }
    }

    return rgb;
}

/*
 * Apply chroma subsampling to YCbCr image
 */
int image_apply_subsampling(Image *ycbcr, SubsamplingMode mode) {
    if (!ycbcr || ycbcr->format != FORMAT_YCBCR) {
        fprintf(stderr, "Invalid image for subsampling\n");
        return -1;
    }

    // If already in the requested mode, do nothing
    if (ycbcr->subsampling == mode) {
        return 0;
    }

    int width = ycbcr->width;
    int height = ycbcr->height;
    int cb_width, cb_height, cr_width, cr_height;

    switch (mode) {
        case SUBSAMPLE_444:
            // No subsampling - use full resolution
            cb_width = width;
            cb_height = height;
            cr_width = width;
            cr_height = height;
            break;

        case SUBSAMPLE_422:
            // Horizontal subsampling (half width)
            cb_width = (width + 1) / 2;
            cb_height = height;
            cr_width = (width + 1) / 2;
            cr_height = height;
            break;

        case SUBSAMPLE_420:
            // Horizontal and vertical subsampling (quarter size)
            cb_width = (width + 1) / 2;
            cb_height = (height + 1) / 2;
            cr_width = (width + 1) / 2;
            cr_height = (height + 1) / 2;
            break;

        default:
            fprintf(stderr, "Unknown subsampling mode\n");
            return -1;
    }

    // Calculate new data size
    int original_size = width * height * 3;
    int y_size = width * height;
    int new_cb_size = cb_width * cb_height;
    int new_cr_size = cr_width * cr_height;
    int new_total_size = y_size + new_cb_size + new_cr_size;

    // Allocate new buffer for entire image data
    unsigned char *new_data = (unsigned char *)malloc(new_total_size);
    if (!new_data) {
        fprintf(stderr, "Memory allocation failed during subsampling\n");
        return -1;
    }

    // Copy Y channel data (remains unchanged)
    memcpy(new_data, ycbcr->y_channel, y_size);

    // Pointers to new chroma channels
    unsigned char *new_cb = new_data + y_size;
    unsigned char *new_cr = new_data + y_size + new_cb_size;

    // Perform subsampling
    switch (mode) {
        case SUBSAMPLE_444:
            // Copy the channels as is (or upsample if coming from a more compressed format)
            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    unsigned char cb = image_get_cb(ycbcr, x, y);
                    unsigned char cr = image_get_cr(ycbcr, x, y);
                    new_cb[y * cb_width + x] = cb;
                    new_cr[y * cr_width + x] = cr;
                }
            }
            break;

        case SUBSAMPLE_422:
            // Horizontal subsampling (average pairs of pixels horizontally)
            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < cb_width; ++x) {
                    int x2 = x * 2;
                    unsigned char cb_sum = 0, cr_sum = 0;
                    int count = 0;

                    // Average horizontal pair
                    for (int dx = 0; dx < 2; ++dx) {
                        int src_x = x2 + dx;
                        if (src_x < width) {
                            cb_sum += image_get_cb(ycbcr, src_x, y);
                            cr_sum += image_get_cr(ycbcr, src_x, y);
                            count++;
                        }
                    }

                    new_cb[y * cb_width + x] = count ? cb_sum / count : 128;
                    new_cr[y * cr_width + x] = count ? cr_sum / count : 128;
                }
            }
            break;

        case SUBSAMPLE_420:
            // Horizontal and vertical subsampling (average 2x2 blocks)
            for (int y = 0; y < cb_height; ++y) {
                for (int x = 0; x < cb_width; ++x) {
                    int x2 = x * 2;
                    int y2 = y * 2;
                    unsigned char cb_sum = 0, cr_sum = 0;
                    int count = 0;

                    // Average the 2x2 block
                    for (int dy = 0; dy < 2; ++dy) {
                        for (int dx = 0; dx < 2; ++dx) {
                            int src_x = x2 + dx;
                            int src_y = y2 + dy;
                            if (src_x < width && src_y < height) {
                                cb_sum += image_get_cb(ycbcr, src_x, src_y);
                                cr_sum += image_get_cr(ycbcr, src_x, src_y);
                                count++;
                            }
                        }
                    }

                    new_cb[y * cb_width + x] = count ? cb_sum / count : 128;
                    new_cr[y * cr_width + x] = count ? cr_sum / count : 128;
                }
            }
            break;
    }

    // Free old data and update with new data
    free(ycbcr->data);
    ycbcr->data = new_data;

    // Update channel pointers
    ycbcr->y_channel = new_data;
    ycbcr->cb_channel = new_data + y_size;
    ycbcr->cr_channel = new_data + y_size + new_cb_size;

    // Update dimensions
    ycbcr->cb_width = cb_width;
    ycbcr->cb_height = cb_height;
    ycbcr->cr_width = cr_width;
    ycbcr->cr_height = cr_height;
    ycbcr->subsampling = mode;

    return 0;
}

/*
 * Crop an image to the specified dimensions
 */
Image* image_crop(Image *img, int x, int y, int width, int height) {
    if (!img || width <= 0 || height <= 0 ||
        x < 0 || y < 0 || x + width > img->width || y + height > img->height) {
        fprintf(stderr, "Invalid crop parameters\n");
        return NULL;
    }

    Image *cropped = image_create(width, height, img->channels, img->bit_depth, img->format);
    if (!cropped) {
        return NULL;
    }

    // For RGB and grayscale, straightforward pixel copying
    if (img->format == FORMAT_RGB || img->format == FORMAT_GRAYSCALE) {
        for (int cy = 0; cy < height; ++cy) {
            for (int cx = 0; cx < width; ++cx) {
                for (int c = 0; c < img->channels; ++c) {
                    unsigned char value = image_get_pixel(img, x + cx, y + cy, c);
                    image_set_pixel(cropped, cx, cy, c, value);
                }
            }
        }
    }
        // For YCbCr, need to handle separate channels and possibly subsampling
    else if (img->format == FORMAT_YCBCR) {
        // Copy Y channel
        for (int cy = 0; cy < height; ++cy) {
            for (int cx = 0; cx < width; ++cx) {
                unsigned char y_val = image_get_y(img, x + cx, y + cy);
                cropped->y_channel[cy * width + cx] = y_val;
            }
        }

        // Apply subsampling to match the original
        cropped->subsampling = SUBSAMPLE_444; // Start with no subsampling

        // Copy Cb and Cr channels (before subsampling)
        for (int cy = 0; cy < height; ++cy) {
            for (int cx = 0; cx < width; ++cx) {
                unsigned char cb_val = image_get_cb(img, x + cx, y + cy);
                unsigned char cr_val = image_get_cr(img, x + cx, y + cy);
                cropped->cb_channel[cy * width + cx] = cb_val;
                cropped->cr_channel[cy * width + cx] = cr_val;
            }
        }

        // Apply the same subsampling as the original image
        if (img->subsampling != SUBSAMPLE_444) {
            image_apply_subsampling(cropped, img->subsampling);
        }
    }

    return cropped;
}

/*
 * Resize an image to new dimensions using bilinear interpolation
 */
Image* image_resize(Image *img, int new_width, int new_height) {
    if (!img || new_width <= 0 || new_height <= 0) {
        fprintf(stderr, "Invalid resize parameters\n");
        return NULL;
    }

    Image *resized = image_create(new_width, new_height, img->channels, img->bit_depth, img->format);
    if (!resized) {
        return NULL;
    }

    // Calculate scaling factors
    float x_ratio = (float)(img->width - 1) / new_width;
    float y_ratio = (float)(img->height - 1) / new_height;

    // For RGB and grayscale, perform bilinear interpolation
    if (img->format == FORMAT_RGB || img->format == FORMAT_GRAYSCALE) {
        for (int y = 0; y < new_height; ++y) {
            for (int x = 0; x < new_width; ++x) {
                float src_x = x * x_ratio;
                float src_y = y * y_ratio;

                // Calculate the four nearest pixels
                int x1 = (int)src_x;
                int y1 = (int)src_y;
                int x2 = (x1 + 1) < img->width ? (x1 + 1) : x1;
                int y2 = (y1 + 1) < img->height ? (y1 + 1) : y1;

                // Calculate interpolation weights
                float wx = src_x - x1;
                float wy = src_y - y1;
                float w1 = (1 - wx) * (1 - wy);
                float w2 = wx * (1 - wy);
                float w3 = (1 - wx) * wy;
                float w4 = wx * wy;

                // Interpolate each channel
                for (int c = 0; c < img->channels; ++c) {
                    float val = w1 * image_get_pixel(img, x1, y1, c) +
                                w2 * image_get_pixel(img, x2, y1, c) +
                                w3 * image_get_pixel(img, x1, y2, c) +
                                w4 * image_get_pixel(img, x2, y2, c);

                    image_set_pixel(resized, x, y, c, (unsigned char)round(val));
                }
            }
        }
    }
        // For YCbCr, we need to handle each channel separately
    else if (img->format == FORMAT_YCBCR) {
        // First convert to RGB for accurate interpolation
        Image *rgb = image_ycbcr_to_rgb(img);
        if (!rgb) {
            image_free(resized);
            return NULL;
        }

        // Resize the RGB image
        Image *resized_rgb = image_resize(rgb, new_width, new_height);
        image_free(rgb);

        if (!resized_rgb) {
            image_free(resized);
            return NULL;
        }

        // Convert back to YCbCr
        Image *resized_ycbcr = image_rgb_to_ycbcr(resized_rgb);
        image_free(resized_rgb);

        if (!resized_ycbcr) {
            image_free(resized);
            return NULL;
        }

        // Apply the same subsampling as the original
        if (img->subsampling != SUBSAMPLE_444) {
            image_apply_subsampling(resized_ycbcr, img->subsampling);
        }

        // Replace the initial resized image with the properly converted one
        image_free(resized);
        resized = resized_ycbcr;
    }

    return resized;
}

