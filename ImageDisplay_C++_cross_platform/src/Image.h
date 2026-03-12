//*****************************************************************************
// Image.h : Defines the class operations on images
// Cross-platform version (no Windows dependencies)
//*****************************************************************************

#ifndef IMAGE_DISPLAY
#define IMAGE_DISPLAY

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Fixed image dimensions — change here if needed
#define IMAGE_WIDTH  512
#define IMAGE_HEIGHT 512

// Max path length (replaces Windows _MAX_PATH)
#define MAX_PATH_LEN 512

// Class structure of Image
// Stores an RGB image as interleaved bytes: R,G,B,R,G,B,...
class MyImage
{
private:
    int   Width;                  // Width of image in pixels
    int   Height;                 // Height of image in pixels
    char  ImagePath[MAX_PATH_LEN]; // Path to the image file
    char* Data;                   // Interleaved RGB pixel data

public:
    // Constructor / Destructor
    MyImage();
    MyImage(MyImage* otherImage);   // Copy constructor
    ~MyImage();

    // Assignment operator
    MyImage& operator=(const MyImage& otherImage);

    // Setters
    void setWidth(const int w)          { Width = w; }
    void setHeight(const int h)         { Height = h; }
    void setImageData(const char* img)  { Data = (char*)img; }
    void setImagePath(const char* path) { strncpy(ImagePath, path, MAX_PATH_LEN - 1); }

    // Getters
    int   getWidth()     { return Width; }
    int   getHeight()    { return Height; }
    char* getImageData() { return Data; }
    char* getImagePath() { return ImagePath; }

    // File I/O
    bool ReadImage();   // Read .rgb file into Data
    bool WriteImage();  // Write Data back to .rgb file
};

#endif // IMAGE_DISPLAY