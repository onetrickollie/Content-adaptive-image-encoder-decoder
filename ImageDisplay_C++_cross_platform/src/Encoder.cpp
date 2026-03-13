//**
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
//   - Valid block sizes: {2, 4, 8, 16, 32}
//   - Result: large blocks in smooth regions, small blocks in detail regions
//
// Variance threshold: 500.0 (empirically determined, see README)
//*

#include "Encoder.h"
#include "DCT.h"
#include <cmath>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <algorithm>
#include <map>

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
//-----------------------------------------------------------------------------
void Encoder::encode()
{
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
        for (int y = 0; y < height; y += MAX_BLOCK_SIZE)
            for (int x = 0; x < width; x += MAX_BLOCK_SIZE)
                buildQuadtree(x, y, MAX_BLOCK_SIZE);
    }

    std::cout << "Encoded " << blocks.size() << " blocks with Q=" << Q << std::endl;

    // Print block size breakdown for Mode 2
    if (M == 2)
        printBlockStats();
}

//-----------------------------------------------------------------------------
// printBlockStats
// Prints how many blocks of each size were produced by the quadtree
// Useful for documenting and defending the algorithm
//-----------------------------------------------------------------------------
void Encoder::printBlockStats()
{
    std::map<int, int> sizeCounts;
    for (const BlockData& bd : blocks)
        sizeCounts[bd.N]++;

    std::cout << "Block size distribution:" << std::endl;
    for (auto& pair : sizeCounts)
        std::cout << "  " << pair.first << "x" << pair.first
                  << " : " << pair.second << " blocks" << std::endl;
}

//-----------------------------------------------------------------------------
// computeVariance
// Computes variance of pixel values in an NxN region for one channel
// Variance = E[x^2] - E[x]^2
//-----------------------------------------------------------------------------
double Encoder::computeVariance(int startX, int startY, int N, int channel)
{
    char* data = image->getImageData();
    double sum   = 0.0;
    double sumSq = 0.0;
    int count    = 0;

    for (int row = 0; row < N; row++)
    {
        for (int col = 0; col < N; col++)
        {
            int px = startX + col;
            int py = startY + row;

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
// If max variance across channels >= VARIANCE_THRESHOLD AND N > MIN_BLOCK_SIZE:
//   Split into 4 quadrants of size N/2 and recurse
// Else:
//   Encode this block at size N
//
// Valid block sizes produced: {2, 4, 8, 16, 32}
//-----------------------------------------------------------------------------
void Encoder::buildQuadtree(int startX, int startY, int N)
{
    // Compute max variance across all 3 channels
    double maxVariance = 0.0;
    for (int ch = 0; ch < 3; ch++)
    {
        double v = computeVariance(startX, startY, N, ch);
        if (v > maxVariance) maxVariance = v;
    }

    // Split if high variance and not yet at minimum block size
    if (maxVariance >= VARIANCE_THRESHOLD && N > MIN_BLOCK_SIZE)
    {
        int half = N / 2;
        buildQuadtree(startX,        startY,        half); // top-left
        buildQuadtree(startX + half, startY,        half); // top-right
        buildQuadtree(startX,        startY + half, half); // bottom-left
        buildQuadtree(startX + half, startY + half, half); // bottom-right
    }
    else
    {
        blocks.push_back(encodeBlock(startX, startY, N));
    }
}

//-----------------------------------------------------------------------------
// encodeBlock
// Encodes a single NxN block at pixel position (startX, startY)
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

        computeDCT(block, N);

        bd.coeffs[ch].resize(N, std::vector<int>(N));
        for (int row = 0; row < N; row++)
            for (int col = 0; col < N; col++)
                bd.coeffs[ch][row][col] = (int)round(block[row][col] / quantStep);
    }

    return bd;
}

//-----------------------------------------------------------------------------
// saveDCTFile
// File format:
//   [int] width, height, Q, M, numBlocks
//   Per block: [int] startX, startY, N
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
// Searches Q from 0 upward until compressed bpp <= target B
//-----------------------------------------------------------------------------
void Encoder::computeQFromBPP()
{
    std::cout << "Searching for Q to achieve " << B << " bpp..." << std::endl;

    int bestQ = 1;

    for (int testQ = 0; testQ <= 30; testQ++)
    {
        Q = testQ;

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

        saveDCTFile("/tmp/test_bpp.DCT");
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