/*
    Terminal Equalizer - A real-time command line audio visualizer
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

#include "../inc/main.h"
#include <csignal>
#include <atomic>
#include <thread>

#if defined(_MSC_VER) && defined(__clang__)
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "libfftw3-3.lib")
#endif

#define AsciiRgb(k, r, g, b) "\033[" #k ";2;" #r ";" #g ";" #b "m"

std::atomic<bool> keepRunning(true);

void signalHandler(_In_ int signum) {
    keepRunning = false;
    AudioEngine::Get().Stop();
}

int __cdecl main(void) {
    SetConsoleOutputCP(CP_UTF8);
    fprintf(stdout, AsciiRgb(48, 0, 25, 23) AsciiRgb(38, 0, 255, 236) " * ╭─╮╭─╮╭─╴╭─╴╶┬╴╭─╮╷ ╷╭┬╮ * Authors <\033[97mMajockbim \"%s\", Joe.r Dev" AsciiRgb(38, 0, 255, 236) "> \n"
                    AsciiRgb(48, 0, 18, 25) AsciiRgb(38, 0, 180, 255)  " = ╰─╮├─╯├╴ │   │ ├┬╯│ ││││ = [\033[97mSPECTRUM" AsciiRgb(38, 0, 180, 255) "] Terminal Equalizer - Version 1.2.%08x \n"
                    AsciiRgb(48, 0, 7, 25) AsciiRgb(38, 0, 73, 255) " * ╰─╯╵  ╰─╴╰─╴ ╵ ╵╰╴╰─╯╵ ╵ * Releases: %s\n", "https://majockbim.com/", 193, "https://github.com/majockbim/spectrum/releases/");
    fprintf(stdout, AsciiRgb(48, 0, 0, 25) AsciiRgb(38, 0, 0, 255) " * This program was originally created by MajockBim and edited by Joe.r Dev. It is licensed under the MIT License. *\n\033[0m");
    
    // register signal handler for clean exit
    signal(SIGINT, signalHandler);

    SignalProcessor signal;
    FFTEngine fft;
    RenderEqualizer equalizer;

    std::mutex magMutex;
    std::vector<double> sharedMagnitudes;

    std::thread t1([&]() {
        CoInitialize(NULL);
        while (keepRunning) {
            signal.Accumulate();
            if (signal.isFull()) {
                auto pass = signal.GetFFTBuffer();
                auto magnitudes = fft.Run(pass);
                std::lock_guard<std::mutex> lock(magMutex);
                sharedMagnitudes = magnitudes;
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        }
        CoUninitialize();
    });

    std::thread t2([&]() {
        CoInitialize(NULL);
        int sampleRate = AudioEngine::Get().GetSampleRate();
        Dev::JoerAndMj::SettingsJsonFileFinder jsonFileFinder;
        Dev::JoerAndMj::SettingsJsonFileReader jsonFileReader;

        int findResult = jsonFileFinder.FindJsonFiles(&jsonFileReader);
        if (findResult == 1) {
            std::cout << " * No Json files * " << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        equalizer.EnableVisualizer(sharedMagnitudes, magMutex, sampleRate, jsonFileReader);
        CoUninitialize();
    });

    t1.join();
    t2.join();

    return 0;
}
