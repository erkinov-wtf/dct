/**
 * util.h - Utility functions for memory allocation and management
 * Part of Adaptive DCT Image Compressor
 */

#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utils.h>

/**
 * Helper function to allocate 2D double arrays
 *
 * @param rows Number of rows
 * @param cols Number of columns
 * @return Allocated 2D array
 */
double** alloc_array(int rows, int cols);

/**
 * Helper function to free 2D double arrays
 *
 * @param array Array to free
 * @param rows Number of rows
 */
void free_array(double **array, int rows);

/**
 * Helper function to allocate 2D integer arrays
 *
 * @param rows Number of rows
 * @param cols Number of columns
 * @return Allocated 2D array
 */
int** alloc_int_array(int rows, int cols);

/**
 * Helper function to free 2D integer arrays
 *
 * @param array Array to free
 * @param rows Number of rows
 */
void free_int_array(int **array, int rows);

#endif /* UTIL_H */

