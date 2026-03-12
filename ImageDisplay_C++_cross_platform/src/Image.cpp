//*****************************************************************************
// Image.cpp : Implements image read/write operations
// Cross-platform version (no Windows dependencies)
//
// .rgb file format: all R values, then all G values, then all B values
// (planar format: RRRR...GGGG...BBBB...)
//
// Internal Data format: interleaved RGB per pixel
// (interleaved format: RGB.RGB.RGB...)
//*****************************************************************************

#include "Image.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//-----------------------------------------------------------------------------
// Default Constructor
//-----------------------------------------------------------------------------
MyImage::MyImage()
{
    Data      = NULL;
    Width     = -1;
    Height    = -1;
    ImagePath[0] = 0;
}

//-----------------------------------------------------------------------------
// Destructor
//-----------------------------------------------------------------------------
MyImage::~MyImage()
{
    if (Data)
    {
        delete[] Data;
        Data = NULL;
    }
}

//-----------------------------------------------------------------------------
// Copy Constructor
//-----------------------------------------------------------------------------
MyImage::MyImage(MyImage* otherImage)
{
    Width  = otherImage->Width;
    Height = otherImage->Height;
    strncpy(ImagePath, otherImage->ImagePath, MAX_PATH_LEN - 1);

    int size = Width * Height * 3;
    Data = new char[size];
    for (int i = 0; i < size; i++)
        Data[i] = otherImage->Data[i];
}

//-----------------------------------------------------------------------------
// Assignment Operator
//-----------------------------------------------------------------------------
MyImage& MyImage::operator=(const MyImage& otherImage)
{
    if (this == &otherImage) return *this;

    if (Data) delete[] Data;

    Width  = otherImage.Width;
    Height = otherImage.Height;
    strncpy(ImagePath, otherImage.ImagePath, MAX_PATH_LEN - 1);

    int size = Width * Height * 3;
    Data = new char[size];
    for (int i = 0; i < size; i++)
        Data[i] = otherImage.Data[i];

    return *this;
}

//-----------------------------------------------------------------------------
// ReadImage
// Reads a .rgb file (planar format) into interleaved RGB Data array
//-----------------------------------------------------------------------------
bool MyImage::ReadImage()
{
    // Validate state
    if (ImagePath[0] == 0 || Width < 0 || Height < 0)
    {
        fprintf(stderr, "Error: Image path or dimensions not set.\n");
        return false;
    }

    FILE* IN_FILE = fopen(ImagePath, "rb");
    if (IN_FILE == NULL)
    {
        fprintf(stderr, "Error: Cannot open file for reading: %s\n", ImagePath);
        return false;
    }

    int numPixels = Width * Height;

    // Read planar channels separately
    // .rgb file layout: RRRR...GGGG...BBBB...
    char* Rbuf = new char[numPixels];
    char* Gbuf = new char[numPixels];
    char* Bbuf = new char[numPixels];

    fread(Rbuf, sizeof(char), numPixels, IN_FILE);
    fread(Gbuf, sizeof(char), numPixels, IN_FILE);
    fread(Bbuf, sizeof(char), numPixels, IN_FILE);

    fclose(IN_FILE);

    // Allocate interleaved Data buffer
    if (Data) delete[] Data;
    Data = new char[numPixels * 3];

    // Convert planar → interleaved: R,G,B,R,G,B,...
    for (int i = 0; i < numPixels; i++)
    {
        Data[3 * i]     = Rbuf[i];
        Data[3 * i + 1] = Gbuf[i];
        Data[3 * i + 2] = Bbuf[i];
    }

    delete[] Rbuf;
    delete[] Gbuf;
    delete[] Bbuf;

    return true;
}

//-----------------------------------------------------------------------------
// WriteImage
// Writes interleaved RGB Data array back to a .rgb file (planar format)
//-----------------------------------------------------------------------------
bool MyImage::WriteImage()
{
    if (ImagePath[0] == 0 || Width < 0 || Height < 0)
    {
        fprintf(stderr, "Error: Image path or dimensions not set.\n");
        return false;
    }

    FILE* OUT_FILE = fopen(ImagePath, "wb");
    if (OUT_FILE == NULL)
    {
        fprintf(stderr, "Error: Cannot open file for writing: %s\n", ImagePath);
        return false;
    }

    int numPixels = Width * Height;

    char* Rbuf = new char[numPixels];
    char* Gbuf = new char[numPixels];
    char* Bbuf = new char[numPixels];

    // Convert interleaved → planar
    for (int i = 0; i < numPixels; i++)
    {
        Rbuf[i] = Data[3 * i];
        Gbuf[i] = Data[3 * i + 1];
        Bbuf[i] = Data[3 * i + 2];
    }

    // Write planar format: RRRR...GGGG...BBBB...
    fwrite(Rbuf, sizeof(char), numPixels, OUT_FILE);
    fwrite(Gbuf, sizeof(char), numPixels, OUT_FILE);
    fwrite(Bbuf, sizeof(char), numPixels, OUT_FILE);

    delete[] Rbuf;
    delete[] Gbuf;
    delete[] Bbuf;

    fclose(OUT_FILE);

    return true;
}