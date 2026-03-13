#ifndef ENCODER_H
#define ENCODER_H

#include <string>
#include <vector>
#include "Image.h"

// Variance threshold for quadtree splitting decision
// Blocks with variance >= this value will be split into smaller blocks
// Empirically determined for natural images — see README for limitations
#define VARIANCE_THRESHOLD 500.0

// Minimum and maximum block sizes for Mode 2 adaptive encoding
// Valid sizes: {2, 4, 8, 16, 32} as specified in assignment
#define MIN_BLOCK_SIZE 2
#define MAX_BLOCK_SIZE 32

// Stores the quantized DCT coefficients for one block
struct BlockData {
    int startX;   // top-left x pixel position
    int startY;   // top-left y pixel position
    int N;        // block size (NxN)
    // quantized coefficients: [channel][row][col]
    std::vector<std::vector<std::vector<int>>> coeffs;
};

class Encoder
{
public:
    Encoder(MyImage* image, int M, int Q, float B);
    void encode();
    void saveDCTFile(const std::string& path);

    int getQ()                                  { return Q; }
    int getWidth()                              { return width; }
    int getHeight()                             { return height; }
    const std::vector<BlockData>& getBlocks()   { return blocks; }

private:
    MyImage* image;
    int M, Q;
    float B;
    int width, height;

    std::vector<BlockData> blocks;

    BlockData encodeBlock(int startX, int startY, int N);
    double computeVariance(int startX, int startY, int N, int channel);
    void buildQuadtree(int startX, int startY, int N);
    void computeQFromBPP();
    void printBlockStats();
};

#endif // ENCODER_H