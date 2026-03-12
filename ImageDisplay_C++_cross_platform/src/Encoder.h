#ifndef ENCODER_H
#define ENCODER_H

#include "Image.h"
#include <string>
class Encoder
{
public:
    Encoder(MyImage* image, int M, int Q, float B);
    void encode();
    void saveDCTFile(const std::string& path);
    int  getQ() { return Q; }

private:
    MyImage* image;
    int M, Q;
    float B;
};

#endif // ENCODER_H