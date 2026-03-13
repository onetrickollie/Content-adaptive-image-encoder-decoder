//*****************************************************************************
// Encoder.cpp : Splits image into blocks, applies DCT and quantization
//
// Mode 1: Fixed 8x8 blocks (standard JPEG-style)
// Mode 2: Adaptive NxN blocks using quadtree decomposition
//
// Quadtree algorithm:
//   - Start with MAX_BLOCK_SIZE (32x32) blocks
//   - Compute variance of the block across all channels
//   - If variance >= VARIANCE_THRESHOLD and N > MIN_BLOCK_SIZE: split into 4
//   - Recurse on each quadrant
//   - Result: large blocks in smooth regions, small blocks in detail regions
//
// Pipeline per block per channel:
//   1. Extract NxN pixel values
//   2. Apply 2D DCT
//   3. Quantize by dividing by 2^Q and rounding to int
//*****************************************************************************

#include "Encoder.h"
#include "DCT.h"
#include <cmath>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <algorithm>

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
Encoder::Encoder(MyImage* image, int M, int Q, float B)
    : image(image), M(M), Q(Q), B(B)
{
    width  = image->getWidth();
    height = image->getHeight();
}

//-----------------------------------------------------------------------------
// encode
// Main entry point — splits image into blocks and encodes each one
//-----------------------------------------------------------------------------
void Encoder::encode()
{
    // If Q is -1, auto-compute it from target BPP
    if (Q == -1)
    {
        computeQFromBPP();
        std::cout << "Auto-computed Q = " << Q << " for target B = " << B << " bpp" << std::endl;
    }

    blocks.clear();

    if (M == 1)
    {
        // Mode 1: fixed 8x8 blocks
        int N = 8;
        for (int y = 0; y < height; y += N)
            for (int x = 0; x < width; x += N)
                blocks.push_back(encodeBlock(x, y, N));
    }
    else
    {
        // Mode 2: adaptive NxN blocks using quadtree
        // Start quadtree from MAX_BLOCK_SIZE tiles
        for (int y = 0; y < height; y += MAX_BLOCK_SIZE)
            for (int x = 0; x < width; x += MAX_BLOCK_SIZE)
                buildQuadtree(x, y, MAX_BLOCK_SIZE);
    }

    std::cout << "Encoded " << blocks.size() << " blocks with Q=" << Q << std::endl;
}

//-----------------------------------------------------------------------------
// computeVariance
// Computes the variance of pixel values in an NxN region for one channel
// Variance = average of (pixel - mean)^2
// Used by quadtree to decide whether a block needs splitting
//-----------------------------------------------------------------------------
double Encoder::computeVariance(int startX, int startY, int N, int channel)
{
    char* data = image->getImageData();
    double sum = 0.0;
    double sumSq = 0.0;
    int count = 0;

    for (int row = 0; row < N; row++)
    {
        for (int col = 0; col < N; col++)
        {
            int px = startX + col;
            int py = startY + row;

            // Clamp to image boundaries
            if (px >= width)  px = width  - 1;
            if (py >= height) py = height - 1;

            double val = (double)(unsigned char)data[(py * width + px) * 3 + channel];
            sum   += val;
            sumSq += val * val;
            count++;
        }
    }

    double mean = sum / count;
    return (sumSq / count) - (mean * mean);
}

//-----------------------------------------------------------------------------
// buildQuadtree
// Recursively decides block sizes using variance as the splitting criterion
//
// Decision logic:
//   - Compute max variance across all 3 channels for this block
//   - If max variance >= VARIANCE_THRESHOLD AND N > MIN_BLOCK_SIZE:
//       split into 4 quadrants of size N/2 and recurse
//   - Otherwise: encode this block at size N
//
// This produces large blocks in smooth regions (sky, walls) and
// small blocks in detailed regions (edges, textures, faces)
//-----------------------------------------------------------------------------
void Encoder::buildQuadtree(int startX, int startY, int N)
{
    // Compute variance across all 3 channels, take the maximum
    double maxVariance = 0.0;
    for (int ch = 0; ch < 3; ch++)
    {
        double v = computeVariance(startX, startY, N, ch);
        if (v > maxVariance) maxVariance = v;
    }

    // Decide: split or keep?
    if (maxVariance >= VARIANCE_THRESHOLD && N > MIN_BLOCK_SIZE)
    {
        // Split into 4 quadrants of size N/2
        int half = N / 2;
        buildQuadtree(startX,        startY,        half); // top-left
        buildQuadtree(startX + half, startY,        half); // top-right
        buildQuadtree(startX,        startY + half, half); // bottom-left
        buildQuadtree(startX + half, startY + half, half); // bottom-right
    }
    else
    {
        // Keep this block — encode it at size N
        blocks.push_back(encodeBlock(startX, startY, N));
    }
}

//-----------------------------------------------------------------------------
// encodeBlock
// Encodes a single NxN block at pixel position (startX, startY)
// For each of the 3 channels (R, G, B):
//   1. Extract pixel values into a 2D block
//   2. Apply 2D DCT
//   3. Quantize by dividing by 2^Q and rounding
//-----------------------------------------------------------------------------
BlockData Encoder::encodeBlock(int startX, int startY, int N)
{
    BlockData bd;
    bd.startX = startX;
    bd.startY = startY;
    bd.N      = N;
    bd.coeffs.resize(3);

    double quantStep = pow(2.0, Q);
    char* data = image->getImageData();

    for (int ch = 0; ch < 3; ch++)
    {
        // Extract NxN pixel block for this channel
        std::vector<std::vector<double>> block(N, std::vector<double>(N, 0.0));

        for (int row = 0; row < N; row++)
        {
            for (int col = 0; col < N; col++)
            {
                int px = startX + col;
                int py = startY + row;

                if (px >= width)  px = width  - 1;
                if (py >= height) py = height - 1;

                int idx = (py * width + px) * 3 + ch;
                block[row][col] = (double)(unsigned char)data[idx];
            }
        }

        // Apply 2D DCT
        computeDCT(block, N);

        // Quantize: divide by 2^Q and round to nearest integer
        bd.coeffs[ch].resize(N, std::vector<int>(N));
        for (int row = 0; row < N; row++)
            for (int col = 0; col < N; col++)
                bd.coeffs[ch][row][col] = (int)round(block[row][col] / quantStep);
    }

    return bd;
}

//-----------------------------------------------------------------------------
// saveDCTFile
// Saves all quantized DCT coefficients to a binary file
//
// File format:
//   [int] width, height, Q, M, numBlocks
//   Per block:
//     [int] startX, startY, N
//     Per channel (3): Per row (N): Per col (N): [int] quantized coefficient
//-----------------------------------------------------------------------------
void Encoder::saveDCTFile(const std::string& path)
{
    std::ofstream f(path, std::ios::binary);
    if (!f.is_open())
    {
        std::cerr << "Error: Cannot open DCT file for writing: " << path << std::endl;
        return;
    }

    f.write((char*)&width,  sizeof(int));
    f.write((char*)&height, sizeof(int));
    f.write((char*)&Q,      sizeof(int));
    f.write((char*)&M,      sizeof(int));

    int numBlocks = (int)blocks.size();
    f.write((char*)&numBlocks, sizeof(int));

    for (const BlockData& bd : blocks)
    {
        f.write((char*)&bd.startX, sizeof(int));
        f.write((char*)&bd.startY, sizeof(int));
        f.write((char*)&bd.N,      sizeof(int));

        for (int ch = 0; ch < 3; ch++)
            for (int row = 0; row < bd.N; row++)
                for (int col = 0; col < bd.N; col++)
                    f.write((char*)&bd.coeffs[ch][row][col], sizeof(int));
    }

    f.close();
    std::cout << "Saved DCT file: " << path << std::endl;
}

//-----------------------------------------------------------------------------
// computeQFromBPP
// Binary searches for Q that produces compressed bpp <= target B
//-----------------------------------------------------------------------------
void Encoder::computeQFromBPP()
{
    std::cout << "Searching for Q to achieve " << B << " bpp..." << std::endl;

    int bestQ = 1;

    for (int testQ = 0; testQ <= 30; testQ++)
    {
        Q = testQ;

        // Temporarily encode with this Q
        blocks.clear();
        if (M == 1)
        {
            int N = 8;
            for (int y = 0; y < height; y += N)
                for (int x = 0; x < width; x += N)
                    blocks.push_back(encodeBlock(x, y, N));
        }
        else
        {
            for (int y = 0; y < height; y += MAX_BLOCK_SIZE)
                for (int x = 0; x < width; x += MAX_BLOCK_SIZE)
                    buildQuadtree(x, y, MAX_BLOCK_SIZE);
        }

        // Save temp DCT file
        saveDCTFile("/tmp/test_bpp.DCT");

        // Zip and measure
        system("zip -q /tmp/test_bpp.DCT.zip /tmp/test_bpp.DCT");

        std::ifstream zipFile("/tmp/test_bpp.DCT.zip", std::ios::binary | std::ios::ate);
        if (!zipFile.is_open()) continue;
        long zipSize = zipFile.tellg();
        zipFile.close();

        double bpp = (zipSize * 8.0) / (width * height);
        std::cout << "  Q=" << testQ << " → " << bpp << " bpp" << std::endl;

        bestQ = testQ;
        if (bpp <= B) break;
    }

    Q = bestQ;
    system("rm -f /tmp/test_bpp.DCT /tmp/test_bpp.DCT.zip");
}