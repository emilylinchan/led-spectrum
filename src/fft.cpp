/*
    spectrum - A real-time command line audio visualizer
    Copyright (C) 2026 Majock Bim

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "../inc/math/fft.hpp"

FFTEngine::FFTEngine()
{
    n = 2400; // input sample (real)
    np = n / 2 + 1; // returning sample size (real + complex)

    f = fftw_alloc_real(n);
    F = fftw_alloc_complex(np);

    // Pre-calculate Hann window
    hannWindow.resize(n);
    for (int i = 0; i < n; ++i) {
        hannWindow[i] = 0.5 * (1.0 - std::cos(2.0 * M_PI * i / (n - 1)));
    }

    // build plan once 
    ForwardPlan = fftw_plan_dft_r2c_1d(n, f, F, FFTW_ESTIMATE);
}

FFTEngine::~FFTEngine()
{
    fftw_destroy_plan(ForwardPlan);
    fftw_free(f);
    fftw_free(F);
}

std::vector<double> FFTEngine::Run(std::array<double, 2400>& audioBuffer)
{
    std::vector<double> magnitudes;

    for (int i = 0; i < n; ++i)
    {
        double sample = 0.0;

        if(i < (int)audioBuffer.size())
        {
            sample = static_cast<double>(audioBuffer[i]);

            if(std::isinf(sample) || std::isnan(sample))
            {
                sample =  0.0;
            }
        }

        // Apply Hann window to reduce spectral leakage
        f[i] = sample * hannWindow[i];
    }

    // perform fft
    fftw_execute(ForwardPlan);

    magnitudes.reserve(np);

    for (int i = 0; i < np; ++i)
    {
        double real = F[i][0];
        double imag = F[i][1];

        // Magnitude
        double raw = std::sqrt(real * real + imag * imag);

        // Normalize by n/2 (for Hann window, we could normalize further but this is fine for relative scale)
        double normalized = raw / (n / 2.0);

        // Convert to dB
        double dB = 20.0 * std::log10(normalized + 1e-12);

        // shift so that -100dB is 0 and 0dB is 100
        double dB_shift = dB + 100.0;

        magnitudes.push_back(dB_shift);
    }

    return magnitudes;
}
