#ifndef DECODER_H
#define DECODER_H

#include <vector>
#include "Image.h"
#include "Encoder.h"

class Decoder
{
public:
    // Takes a completed Encoder to get all block data and parameters
    Decoder(Encoder& encoder);

    // Runs inverse quantization + IDCT on all blocks
    // Returns a newly allocated MyImage with reconstructed pixels
    MyImage* decode();

private:
    Encoder& encoder;
};

#endif // DECODER_H