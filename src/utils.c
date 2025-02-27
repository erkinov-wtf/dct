/**
 * util.c - Implementation of utility functions for memory allocation and management
 * Part of Adaptive DCT Image Compressor
 */
#include <utils.h>

// Allocate 2D double array
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

// Free 2D double array
void free_array(double **array, int rows) {
    for (int i = 0; i < rows; ++i) {
        free(array[i]);
    }
    free(array);
}

// Allocate 2D integer array
int **alloc_int_array(int rows, int cols) {
    int **new_array = (int **) malloc(rows * sizeof(int *));
    if (!new_array) {
        fprintf(stderr, "Memory allocation failed, when creating new 2D integer array\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < rows; ++i) {
        new_array[i] = (int *) malloc(cols * sizeof(int));
        if (!new_array[i]) {
            fprintf(stderr, "Memory allocation failed, when creating new 2D integer array\n");
            exit(EXIT_FAILURE);
        }
        memset(new_array[i], 0, cols * sizeof(int));
    }

    return new_array;
}

// Free 2D integer array
void free_int_array(int **array, int rows) {
    for (int i = 0; i < rows; ++i) {
        free(array[i]);
    }
    free(array);
}

