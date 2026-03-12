//**
// Encoder.cpp : Splits image into blocks, applies DCT and quantization
//
// Pipeline per block per channel:
//   1. Extract NxN block of pixel values
//   2. Apply 2D DCT → 64 frequency coefficients
//   3. Quantize: divide each coefficient by 2^Q and round to int
//
// The quantized coefficients are stored in BlockData structs
// and later saved to a .DCT file.
//*

#include "Encoder.h"
#include "DCT.h"
#include <cmath>
#include <iostream>
#include <fstream>
#include <cstdlib>

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
    // If Q is -1, we need to auto-compute it from target BPP
    if (Q == -1)
    {
        computeQFromBPP();
        std::cout << "Auto-computed Q = " << Q << " for target B = " << B << " bpp" << std::endl;
    }

    blocks.clear();

    // For Mode 1: fixed 8x8 blocks
    int N = (M == 1) ? 8 : 8; // Mode 2 will use variable N in Phase 4

    for (int y = 0; y < height; y += N)
    {
        for (int x = 0; x < width; x += N)
        {
            // Handle edge blocks that go out of bounds
            int blockN = N;
            if (x + N > width)  blockN = width  - x;
            if (y + N > height) blockN = height - y;

            // Only process square blocks for now
            // Edge strips are handled by using the smaller dimension
            int minDim = std::min(blockN, (y + N > height ? height - y : N));
            if (minDim < 1) continue;

            blocks.push_back(encodeBlock(x, y, N));
        }
    }

    std::cout << "Encoded " << blocks.size() << " blocks with Q=" << Q << std::endl;
}

//-----------------------------------------------------------------------------
// encodeBlock
// Encodes a single NxN block at pixel position (startX, startY)
// For each of the 3 channels (R, G, B):
//   - Extract pixel values into a 2D block
//   - Apply DCT
//   - Quantize by dividing by 2^Q
//-----------------------------------------------------------------------------
BlockData Encoder::encodeBlock(int startX, int startY, int N)
{
    BlockData bd;
    bd.startX = startX;
    bd.startY = startY;
    bd.N      = N;
    bd.coeffs.resize(3); // 3 channels: R, G, B

    double quantStep = pow(2.0, Q); // 2^Q
    char* data = image->getImageData();

    for (int ch = 0; ch < 3; ch++)
    {
        // Build NxN block of pixel values for this channel
        std::vector<std::vector<double>> block(N, std::vector<double>(N, 0.0));

        for (int row = 0; row < N; row++)
        {
            for (int col = 0; col < N; col++)
            {
                int px = startX + col;
                int py = startY + row;

                // Clamp to image boundaries for edge blocks
                if (px >= width)  px = width  - 1;
                if (py >= height) py = height - 1;

                int idx = (py * width + px) * 3 + ch;
                // Cast to unsigned char first to get 0-255 range
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
//   [int] width
//   [int] height
//   [int] Q
//   [int] M
//   [int] numBlocks
//   For each block:
//     [int] startX, startY, N
//     For each channel (3):
//       For each row (N):
//         For each col (N): [int] quantized coefficient
//-----------------------------------------------------------------------------
void Encoder::saveDCTFile(const std::string& path)
{
    std::ofstream f(path, std::ios::binary);
    if (!f.is_open())
    {
        std::cerr << "Error: Cannot open DCT file for writing: " << path << std::endl;
        return;
    }

    // Write header
    f.write((char*)&width,  sizeof(int));
    f.write((char*)&height, sizeof(int));
    f.write((char*)&Q,      sizeof(int));
    f.write((char*)&M,      sizeof(int));

    int numBlocks = (int)blocks.size();
    f.write((char*)&numBlocks, sizeof(int));

    // Write each block
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
// Binary searches for a Q value that produces a compressed file
// whose size / (width * height) is approximately equal to target B bpp
//
// Strategy:
//   - Try Q values from 0 to 30
//   - For each Q, encode, save .DCT, zip it, measure bpp
//   - Find the smallest Q where bpp <= B
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
        int N = (M == 1) ? 8 : 8;
        for (int y = 0; y < height; y += N)
            for (int x = 0; x < width; x += N)
                blocks.push_back(encodeBlock(x, y, N));

        // Save temp DCT file
        saveDCTFile("/tmp/test_bpp.DCT");

        // Zip it and measure compressed size
        system("zip -q /tmp/test_bpp.DCT.zip /tmp/test_bpp.DCT");
        
        // Get zip file size
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

    // Clean up temp files
    system("rm -f /tmp/test_bpp.DCT /tmp/test_bpp.DCT.zip");
}