#pragma once

#include "../audio/engine.hpp"
#include "windows.h"

#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <cmath>
#include <chrono>

class RenderEqualizer {
public:
    void Display(); // general volume level
    void DisplayBuffer(); // print float values of buffer

    void EnableVisualizer(std::vector<double>& freq, std::mutex& magMutex, int sampleRate);

private:
    CONSOLE_SCREEN_BUFFER_INFO csbi;        
    int termWidth;
    int termHeight;

    constexpr int N_BARS;
    constexpr int SAMPLE_RATE

    std::vector<std::string> levels = 
    {   "#", 
        "##",
        "###",
        "####", 
        "#####", 
        "######", 
        "#######", 
        "########", 
        "#########", 
        "##########"
    };
};