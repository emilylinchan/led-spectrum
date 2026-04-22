#include "../inc/ui/visualizer.hpp"

#include <iostream>

void RenderEqualizer::Display() {
    int level;
    float temp;
    float vol;
    while(true) {
        vol = AudioEngine::Get().GenVolLevel();
        temp = vol * 9.0;
        level = (int)temp;

        if(vol == 0.0) std::cout << "\r" << " " << "          " << std::flush;
        else std::cout << "\r" << levels[level] << "          " << std::flush;
    }
}

void RenderEqualizer::DisplayBuffer() {
    std::vector<double> CurrentBuffer =  AudioEngine::Get().GetCurrentBuffer(); 
    std::cout << "Size of buffer: " << CurrentBuffer.size() << "\n";

    /* Warning:
    *  Currently, buffer size is 480, and usually the program does not have enough time to
    *  print each value before the next buffer comes in, so they stay in a queue meaning the 
    *  next buffer size will be >480. This has been tested, and is just an interesting latency 'issue.'
    */
    for(int i = 0; i < CurrentBuffer.size(); ++i) {
        std::cout << i << ". " << CurrentBuffer[i] << "\n";
    }

}

void RenderEqualizer::EnableVisualizer(std::vector<double>& freq, std::mutex& magMutex, int sampleRate) 
{
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    termWidth  = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    termHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

    N_BARS = termWidth;

    unsigned int frameCount = 0;
    std::string frame;
    frame.reserve(termWidth * termHeight);
    std::vector<float> barValues(N_BARS);

    while (1) {

        if (frameCount % 30 == 0) {
            GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
            termWidth  = csbi.srWindow.Right - csbi.srWindow.Left + 1;
            termHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
            frame.reserve(termWidth * termHeight);
        }
        frameCount++;

        {
            std::lock_guard<std::mutex> lock(magMutex);
            for (int i = 0; i < N_BARS; ++i) {
                int binLow = (int)(20.0f * pow(20000.f / 20.f, (float)i / N_BARS) * 1201 / (sampleRate / 2.f));
                int binHigh = (int)(20.0f * pow(20000.f / 20.f, (float)(i+1) / N_BARS) * 1201 / (sampleRate / 2.f))
            }

        }

        frame.clear();
        for (int row = termHeight; row > 0; row--) {
            for (int i = 0; i < N_BARS; i++) {
                int barGheight = (int)(barValues[i] * termHeight);
                frame += (row <= barHeight) ? '█' : ' ';
            }
            frame += '\n';
        }

        std::cout << "\033[H" << frame << std::flush;        
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

}