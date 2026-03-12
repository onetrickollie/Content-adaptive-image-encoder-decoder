//**
// DCT.cpp : Implements forward and inverse 2D Discrete Cosine Transform
//
// Uses the separable property:
//   2D DCT = 1D DCT applied to rows, then 1D DCT applied to columns
//   2D IDCT = 1D IDCT applied to columns, then 1D IDCT applied to rows
//
// Standard libraries used: <cmath> for cos(), sqrt(), <vector>
//*

#include "DCT.h"
#include <cmath>
#include <vector>

static const double PI = 3.14159265358979323846;

//-----------------------------------------------------------------------------
// alpha: scaling factor for DCT basis functions
// α(0) = sqrt(1/N), α(u>0) = sqrt(2/N)
//-----------------------------------------------------------------------------
static double alpha(int u, int N)
{
    if (u == 0)
        return sqrt(1.0 / N);
    else
        return sqrt(2.0 / N);
}

//-----------------------------------------------------------------------------
// compute1DDCT
// Applies the 1D DCT to a single row/column of length N (in-place)
// Formula: C(u) = α(u) · Σ x[n] · cos(π·u·(2n+1) / (2N))
//-----------------------------------------------------------------------------
static void compute1DDCT(std::vector<double>& signal, int N)
{
    std::vector<double> result(N, 0.0);

    for (int u = 0; u < N; u++)
    {
        double sum = 0.0;
        for (int n = 0; n < N; n++)
        {
            sum += signal[n] * cos(PI * u * (2.0 * n + 1.0) / (2.0 * N));
        }
        result[u] = alpha(u, N) * sum;
    }

    signal = result;
}

//-----------------------------------------------------------------------------
// compute1DIDCT
// Applies the 1D IDCT to a single row/column of length N (in-place)
// Formula: x[n] = Σ α(u) · C(u) · cos(π·u·(2n+1) / (2N))
//-----------------------------------------------------------------------------
static void compute1DIDCT(std::vector<double>& signal, int N)
{
    std::vector<double> result(N, 0.0);

    for (int n = 0; n < N; n++)
    {
        double sum = 0.0;
        for (int u = 0; u < N; u++)
        {
            sum += alpha(u, N) * signal[u] * cos(PI * u * (2.0 * n + 1.0) / (2.0 * N));
        }
        result[n] = sum;
    }

    signal = result;
}

//-----------------------------------------------------------------------------
// computeDCT
// Applies 2D DCT to an NxN block using separability:
//   Step 1: Apply 1D DCT to each row
//   Step 2: Apply 1D DCT to each column
//-----------------------------------------------------------------------------
void computeDCT(std::vector<std::vector<double>>& block, int N)
{
    // DCT each row
    for (int row = 0; row < N; row++)
    {
        compute1DDCT(block[row], N);
    }

    // DCT each column
    for (int col = 0; col < N; col++)
    {
        // Extract column into a temporary vector
        std::vector<double> column(N);
        for (int row = 0; row < N; row++)
            column[row] = block[row][col];

        compute1DDCT(column, N);

        // Write column back
        for (int row = 0; row < N; row++)
            block[row][col] = column[row];
    }
}

//-----------------------------------------------------------------------------
// computeIDCT
// Applies 2D IDCT to an NxN block using separability:
//   Step 1: Apply 1D IDCT to each column
//   Step 2: Apply 1D IDCT to each row
// Note: reverse order of DCT (columns first, then rows)
//-----------------------------------------------------------------------------
void computeIDCT(std::vector<std::vector<double>>& block, int N)
{
    // IDCT each column
    for (int col = 0; col < N; col++)
    {
        std::vector<double> column(N);
        for (int row = 0; row < N; row++)
            column[row] = block[row][col];

        compute1DIDCT(column, N);

        for (int row = 0; row < N; row++)
            block[row][col] = column[row];
    }

    // IDCT each row
    for (int row = 0; row < N; row++)
    {
        compute1DIDCT(block[row], N);
    }
}