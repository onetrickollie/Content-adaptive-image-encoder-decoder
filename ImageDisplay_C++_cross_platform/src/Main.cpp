#include <wx/wx.h>
#include <wx/dcbuffer.h>

#include <iostream>
#include <string>
#include <cstdlib>

#include "Image.h"

// These will be uncommented as we build each phase:
// #include "Encoder.h"
// #include "Decoder.h"

using namespace std;

/**
 * Main.cpp
 * InputImage - path to a .rgb image file
 * M  - 1 for standard 8x8 DCT, 2 for adaptive NxN DCT
 * Q  - quantization step where Q is non-negative int, or -1 for auto compute
 * B  - target bits per pixel (float > 0.0), ir -1.0 if Q is given
 * Note: Either Q or B must be -1
 * Keyboard functions:
 * B/b  - toggle block boundary display on/off
 */

/** Declarations*/

/**
 * Class that implements wxApp
 */
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

    wxImage       wxImg;           // wxWidgets image object for display
    wxScrolledWindow* scrolledWindow;

    bool showBlockBoundaries;      // toggled by B key press
    int  width;
    int  height;
};

//-----------------------------------------------------------------------------
// Global parameters parsed from command line
//-----------------------------------------------------------------------------
static string g_imagePath = "";
static int    g_M         = 1;      // Mode: 1 = 8x8, 2 = adaptive NxN
static int    g_Q         = -1;     // Quantization step, or -1
static float  g_B         = -1.0f; // Target bits/pixel, or -1.0

//-----------------------------------------------------------------------------
// MyApp::OnInit
// Parses command line arguments, runs encode/decode pipeline, launches window
//-----------------------------------------------------------------------------
bool MyApp::OnInit()
{
    wxInitAllImageHandlers();

    // Expect exactly 4 arguments after the program name
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

    // Validate Q and B — exactly one must be -1
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

    // Print parsed parameters for verification
    cout << "Image Path : " << g_imagePath << endl;
    cout << "Mode  (M)  : " << g_M << (g_M == 1 ? " (8x8 fixed blocks)" : " (adaptive NxN blocks)") << endl;
    cout << "Quant (Q)  : " << g_Q << (g_Q == -1 ? " (auto-compute from B)" : "") << endl;
    cout << "BPP   (B)  : " << g_B << (g_B == -1.0f ? " (controlled by Q)" : " bpp target") << endl;

    //-------------------------------------------------------------------------
    // Load the input image
    //-------------------------------------------------------------------------
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

    //-------------------------------------------------------------------------
    // TODO Phase 2: Run encoder pipeline here
    // Encoder encoder(inputImage, g_M, g_Q, g_B);
    // encoder.encode();
    // encoder.saveDCTFile();
    //
    // TODO Phase 2: Run decoder pipeline here
    // Decoder decoder(encoder);
    // MyImage* outputImage = decoder.decode();
    //
    // For now, display the original unmodified image
    //-------------------------------------------------------------------------
    MyImage* displayImage = inputImage;

    // Launch the display window
    MyFrame* frame = new MyFrame("myProgram - DCT Image Codec", displayImage);
    frame->Show(true);

    return true;
}

//-----------------------------------------------------------------------------
// MyFrame Constructor
// Sets up the scrollable window and binds events
//-----------------------------------------------------------------------------
MyFrame::MyFrame(const wxString& title, MyImage* displayImage)
    : wxFrame(NULL, wxID_ANY, title),
      showBlockBoundaries(false)
{
    width  = displayImage->getWidth();
    height = displayImage->getHeight();

    // Convert our raw RGB data into a wxImage for rendering
    // wxImage expects malloc'd memory — it will own and free it
    unsigned char* wxData =
        (unsigned char*)malloc(width * height * 3 * sizeof(unsigned char));

    char* srcData = displayImage->getImageData();
    for (int i = 0; i < width * height * 3; i++)
        wxData[i] = (unsigned char)srcData[i];

    // SetData takes ownership of wxData (do not free manually)
    wxImg.Create(width, height, wxData, false);

    // Scrollable window setup
    scrolledWindow = new wxScrolledWindow(this, wxID_ANY);
    scrolledWindow->SetScrollbars(10, 10, width, height);
    scrolledWindow->SetVirtualSize(width, height);

    // Bind paint and keyboard events
    scrolledWindow->Bind(wxEVT_PAINT,   &MyFrame::OnPaint,   this);
    scrolledWindow->Bind(wxEVT_KEY_DOWN, &MyFrame::OnKeyDown, this);

    // Make scrolled window focusable so it receives key events
    scrolledWindow->SetFocus();

    SetClientSize(width, height);
    SetBackgroundColour(*wxBLACK);
}

//-----------------------------------------------------------------------------
// MyFrame::OnPaint
// Draws the reconstructed image. In Phase 4, also draws block boundaries.
//-----------------------------------------------------------------------------
void MyFrame::OnPaint(wxPaintEvent& event)
{
    wxBufferedPaintDC dc(scrolledWindow);
    scrolledWindow->DoPrepareDC(dc);

    // Draw the image
    wxBitmap bmp(wxImg);
    dc.DrawBitmap(bmp, 0, 0, false);

    // TODO Phase 4: Draw block boundaries when showBlockBoundaries is true
    if (showBlockBoundaries)
    {
        // Block boundary drawing will be implemented in Phase 4
        // For now just a placeholder message in the title bar
        SetTitle("myProgram - Block Boundaries: ON");
    }
    else
    {
        SetTitle("myProgram - Block Boundaries: OFF");
    }
}

//-----------------------------------------------------------------------------
// MyFrame::OnKeyDown
// Handles the B/b keyboard toggle for block boundary display
//-----------------------------------------------------------------------------
void MyFrame::OnKeyDown(wxKeyEvent& event)
{
    int keyCode = event.GetKeyCode();

    if (keyCode == 'B' || keyCode == 'b')
    {
        showBlockBoundaries = !showBlockBoundaries;
        cout << "Block boundaries: " << (showBlockBoundaries ? "ON" : "OFF") << endl;

        // Trigger a repaint
        scrolledWindow->Refresh();
    }

    event.Skip(); // Pass unhandled keys along
}

wxIMPLEMENT_APP(MyApp);