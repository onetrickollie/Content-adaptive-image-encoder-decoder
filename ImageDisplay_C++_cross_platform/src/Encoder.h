#ifndef ENCODER_H
#define ENCODER_H

#include <string>
#include <vector>
#include "Image.h"

// Stores the quantized DCT coefficients for one block
struct BlockData {
    int startX;   // top-left x position of block in image
    int startY;   // top-left y position of block in image
    int N;        // block size (NxN)
    // quantized coefficients for each channel: [channel][row][col]
    std::vector<std::vector<std::vector<int>>> coeffs;
};

class Encoder
{
public:
    // image  : input image to encode
    // M      : 1 = fixed 8x8 blocks, 2 = adaptive NxN blocks
    // Q      : quantization step (divide by 2^Q), or -1 to auto-compute
    // B      : target bits/pixel, or -1.0 if Q is given
    Encoder(MyImage* image, int M, int Q, float B);

    // Run the full encode pipeline
    void encode();

    // Save quantized DCT coefficients to a .DCT file
    void saveDCTFile(const std::string& path);

    // Getters for decoder use
    int getQ()                                  { return Q; }
    int getWidth()                              { return width; }
    int getHeight()                             { return height; }
    const std::vector<BlockData>& getBlocks()   { return blocks; }

private:
    MyImage* image;
    int M, Q;
    float B;
    int width, height;

    // All encoded blocks (populated by encode())
    std::vector<BlockData> blocks;

    // Encode a single NxN block at position (startX, startY)
    BlockData encodeBlock(int startX, int startY, int N);

    // Auto-compute Q to hit target bits/pixel B
    void computeQFromBPP();
};

#endif // ENCODER_H