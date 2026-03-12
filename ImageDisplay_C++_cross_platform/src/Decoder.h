#ifndef DECODER_H
#define DECODER_H

#include "Image.h"
#include "Encoder.h"

class Decoder
{
public:
    Decoder(Encoder& encoder);
    MyImage* decode();

private:
    Encoder& encoder;
};

#endif // DECODER_H