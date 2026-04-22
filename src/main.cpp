#include "../inc/main.h"

int main() {
    SignalProcessor signal;
    FFTEngine fft;
    RenderEqualizer equalizer;

    std::thread t1([&]() {
        while (1) {
            signal.Accumulate();
            if (signal.isFull()) {
                auto pass = signal.GetFFTBuffer();
                auto magnitudes = fft.Run(pass);
                std::lock_guard<std::mutex> lock(magMutex);
                sharedMagnitudes = magnitudes;
            }
        }
    });

    std::thread t2([&]() {
        int sampleRate = AudioEngine::Get().GetSampleRate();
        equalizer.EnableVisualizer(sharedMagnitudes, magMutex, sampleRate);
    });

    t1.join();
    t2.join();

    return 0;
}