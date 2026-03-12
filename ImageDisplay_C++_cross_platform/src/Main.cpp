//**
// Main.cpp : Entry point for myProgram
//
// Usage: ./myProgram InputImage M Q B
//
//   InputImage  - path to a .rgb image file
//   M           - 1 for standard 8x8 DCT, 2 for adaptive NxN DCT
//   Q           - quantization step (non-negative int), or -1 to auto-compute
//   B           - target bits per pixel (float > 0.0), or -1.0 if Q is given
//
// NOTE: Either Q or B must be -1, they cannot both be positive.
//
// Keyboard:
//   B / b       - toggle block boundary display on/off
//*

#include <wx/wx.h>
#include <wx/dcbuffer.h>

#include <iostream>
#include <string>
#include <cstdlib>

#include "Image.h"
#include "Encoder.h"
#include "Decoder.h"

using namespace std;

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class MyApp : public wxApp {
public:
    bool OnInit() override;
};

class MyFrame : public wxFrame {
public:
    MyFrame(const wxString& title, MyImage* displayImage);

private:
    void OnPaint(wxPaintEvent& event);
    void OnKeyDown(wxKeyEvent& event);

    wxImage           wxImg;
    wxScrolledWindow* scrolledWindow;

    bool showBlockBoundaries;
    int  width;
    int  height;
};

//-----------------------------------------------------------------------------
// Global parameters parsed from command line
//-----------------------------------------------------------------------------
static string g_imagePath = "";
static int    g_M         = 1;
static int    g_Q         = -1;
static float  g_B         = -1.0f;

//-----------------------------------------------------------------------------
// MyApp::OnInit
//-----------------------------------------------------------------------------
bool MyApp::OnInit()
{
    wxInitAllImageHandlers();

    if (wxApp::argc != 5)
    {
        cerr << "Usage: ./myProgram InputImage M Q B" << endl;
        cerr << "  M : 1 (8x8 DCT) or 2 (adaptive NxN DCT)" << endl;
        cerr << "  Q : quantization step (>=0), or -1 to auto-compute from B" << endl;
        cerr << "  B : target bits/pixel (>0.0), or -1.0 if Q is given" << endl;
        return false;
    }

    // Parse arguments
    g_imagePath = wxApp::argv[1].ToStdString();
    g_M         = atoi(wxApp::argv[2].ToStdString().c_str());
    g_Q         = atoi(wxApp::argv[3].ToStdString().c_str());
    g_B         = atof(wxApp::argv[4].ToStdString().c_str());

    // Validate M
    if (g_M != 1 && g_M != 2)
    {
        cerr << "Error: M must be 1 or 2." << endl;
        return false;
    }

    // Validate Q and B
    if (g_Q != -1 && g_B != -1.0f)
    {
        cerr << "Error: Q and B cannot both be positive. One must be -1." << endl;
        return false;
    }
    if (g_Q == -1 && g_B <= 0.0f)
    {
        cerr << "Error: If Q is -1, B must be a positive float." << endl;
        return false;
    }
    if (g_B == -1.0f && g_Q < 0)
    {
        cerr << "Error: If B is -1, Q must be a non-negative integer." << endl;
        return false;
    }

    cout << "Image Path : " << g_imagePath << endl;
    cout << "Mode  (M)  : " << g_M << (g_M == 1 ? " (8x8 fixed blocks)" : " (adaptive NxN blocks)") << endl;
    cout << "Quant (Q)  : " << g_Q << (g_Q == -1 ? " (auto-compute from B)" : "") << endl;
    cout << "BPP   (B)  : " << g_B << (g_B == -1.0f ? " (controlled by Q)" : " bpp target") << endl;

    // Load input image
    MyImage* inputImage = new MyImage();
    inputImage->setWidth(IMAGE_WIDTH);
    inputImage->setHeight(IMAGE_HEIGHT);
    inputImage->setImagePath(g_imagePath.c_str());

    if (!inputImage->ReadImage())
    {
        cerr << "Error: Failed to read image from: " << g_imagePath << endl;
        return false;
    }
    cout << "Image loaded successfully." << endl;

    // Build output DCT filename from input path
    string dctPath = g_imagePath;
    size_t dotPos = dctPath.find_last_of('.');
    if (dotPos != string::npos)
        dctPath = dctPath.substr(0, dotPos) + ".DCT";
    else
        dctPath += ".DCT";

    // Run encoder
    Encoder encoder(inputImage, g_M, g_Q, g_B);
    encoder.encode();
    encoder.saveDCTFile(dctPath);

    // Run decoder
    Decoder decoder(encoder);
    MyImage* outputImage = decoder.decode();

    // Display reconstructed image
    MyFrame* frame = new MyFrame("myProgram - DCT Image Codec", outputImage);
    frame->Show(true);

    return true;
}

//-----------------------------------------------------------------------------
// MyFrame Constructor
//-----------------------------------------------------------------------------
MyFrame::MyFrame(const wxString& title, MyImage* displayImage)
    : wxFrame(NULL, wxID_ANY, title),
      showBlockBoundaries(false)
{
    width  = displayImage->getWidth();
    height = displayImage->getHeight();

    unsigned char* wxData =
        (unsigned char*)malloc(width * height * 3 * sizeof(unsigned char));

    char* srcData = displayImage->getImageData();
    for (int i = 0; i < width * height * 3; i++)
        wxData[i] = (unsigned char)srcData[i];

    wxImg.Create(width, height, wxData, false);

    scrolledWindow = new wxScrolledWindow(this, wxID_ANY);
    scrolledWindow->SetScrollbars(10, 10, width, height);
    scrolledWindow->SetVirtualSize(width, height);

    scrolledWindow->Bind(wxEVT_PAINT,    &MyFrame::OnPaint,   this);
    scrolledWindow->Bind(wxEVT_KEY_DOWN, &MyFrame::OnKeyDown, this);
    scrolledWindow->SetFocus();

    SetClientSize(width, height);
    SetBackgroundColour(*wxBLACK);
}

//-----------------------------------------------------------------------------
// MyFrame::OnPaint
//-----------------------------------------------------------------------------
void MyFrame::OnPaint(wxPaintEvent& event)
{
    wxBufferedPaintDC dc(scrolledWindow);
    scrolledWindow->DoPrepareDC(dc);

    wxBitmap bmp(wxImg);
    dc.DrawBitmap(bmp, 0, 0, false);

    if (showBlockBoundaries)
        SetTitle("myProgram - Block Boundaries: ON");
    else
        SetTitle("myProgram - Block Boundaries: OFF");
}

//-----------------------------------------------------------------------------
// MyFrame::OnKeyDown
//-----------------------------------------------------------------------------
void MyFrame::OnKeyDown(wxKeyEvent& event)
{
    int keyCode = event.GetKeyCode();

    if (keyCode == 'B' || keyCode == 'b')
    {
        showBlockBoundaries = !showBlockBoundaries;
        cout << "Block boundaries: " << (showBlockBoundaries ? "ON" : "OFF") << endl;
        scrolledWindow->Refresh();
    }

    event.Skip();
}

wxIMPLEMENT_APP(MyApp);