//*
// Decoder.cpp : Reconstructs image from quantized DCT coefficients
//
// Pipeline per block per channel:
//   1. Inverse quantize: multiply each coefficient by 2^Q
//   2. Apply 2D IDCT → reconstructed pixel values
//   3. Clamp pixel values to [0, 255]
//   4. Write pixels back into output image
//*

#include "Decoder.h"
#include "DCT.h"
#include <cmath>
#include <iostream>
#include <vector>
#include <algorithm>

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
Decoder::Decoder(Encoder& encoder) : encoder(encoder) {}

//-----------------------------------------------------------------------------
// decode
// Reconstructs the full image from all encoded blocks
//-----------------------------------------------------------------------------
MyImage* Decoder::decode()
{
    int width  = encoder.getWidth();
    int height = encoder.getHeight();
    int Q      = encoder.getQ();

    double quantStep = pow(2.0, Q); // 2^Q

    // Allocate output image
    MyImage* outImage = new MyImage();
    outImage->setWidth(width);
    outImage->setHeight(height);

    // Allocate pixel buffer (interleaved RGB)
    char* outData = new char[width * height * 3]();
    outImage->setImageData(outData);

    const std::vector<BlockData>& blocks = encoder.getBlocks();

    for (const BlockData& bd : blocks)
    {
        int N      = bd.N;
        int startX = bd.startX;
        int startY = bd.startY;

        for (int ch = 0; ch < 3; ch++)
        {
            // Inverse quantize — multiply by 2^Q
            std::vector<std::vector<double>> block(N, std::vector<double>(N));
            for (int row = 0; row < N; row++)
                for (int col = 0; col < N; col++)
                    block[row][col] = bd.coeffs[ch][row][col] * quantStep;

            // Apply 2D IDCT
            computeIDCT(block, N);

            // Write reconstructed pixels back to output image
            for (int row = 0; row < N; row++)
            {
                for (int col = 0; col < N; col++)
                {
                    int px = startX + col;
                    int py = startY + row;

                    // Skip pixels outside image bounds (edge blocks)
                    if (px >= width || py >= height) continue;

                    // Clamp to valid pixel range [0, 255]
                    int pixelVal = (int)round(block[row][col]);
                    pixelVal = std::max(0, std::min(255, pixelVal));

                    int idx = (py * width + px) * 3 + ch;
                    outData[idx] = (char)(unsigned char)pixelVal;
                }
            }
        }
    }

    std::cout << "Decoded " << blocks.size() << " blocks." << std::endl;
    return outImage;
}