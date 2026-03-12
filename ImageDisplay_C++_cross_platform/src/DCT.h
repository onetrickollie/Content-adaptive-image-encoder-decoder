#ifndef DCT_H
#define DCT_H

#include <vector>

// Computes 2D DCT on an NxN block (in-place)
// Input:  block[row][col] of pixel values (0-255 range)
// Output: block[row][col] of DCT frequency coefficients
void computeDCT(std::vector<std::vector<double>>& block, int N);

// Computes 2D Inverse DCT on an NxN block (in-place)
// Input:  block[row][col] of DCT frequency coefficients
// Output: block[row][col] of reconstructed pixel values
void computeIDCT(std::vector<std::vector<double>>& block, int N);

#endif // DCT_H