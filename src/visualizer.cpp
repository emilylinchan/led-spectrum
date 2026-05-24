/*
    Terminal Equalizer - A real-time command line audio visualizer
    Copyright (C) 2026 Majock Bim
    Copyright (C) 2026 Joe R.

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

#include "../inc/ui/visualizer.hpp"
#include <cstring>

#define AsciiRgb(k, r, g, b) "\033[" #k ";2;" #r ";" #g ";" #b "m"

template <typename _divide_number_t, typename _divide_number1_t>
inline _divide_number_t __cdecl SafeDivide(_In_ _divide_number_t a, _In_ _divide_number1_t b) noexcept 
{
    if (b == 0) {return(1.00f);} return(_divide_number_t(_divide_number1_t(a)/b));
}

std::string RenderEqualizer::getBraille(unsigned char mask) {
    std::string s;
    s += (char)0xE2;
    s += (char)(0xA0 | (mask >> 6));
    s += (char)(0x80 | (mask & 0x3F));
    return s;
}

void RenderEqualizer::Display() {
    int level;
    float temp;
    float vol;
    while(AudioEngine::Get().IsRunning()) {
        vol = AudioEngine::Get().GenVolLevel();
        temp = vol * 9.0;
        level = (int)temp;

        if(vol == 0.0) std::cout << "\r" << " " << "          " << std::flush;
        else std::cout << "\r" << levels[level] << "          " << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void RenderEqualizer::DisplayBuffer() {
    std::vector<double> CurrentBuffer =  AudioEngine::Get().GetCurrentBuffer(); 
    std::cout << "Size of buffer: " << CurrentBuffer.size() << "\n";

    for(int i = 0; i < (int)CurrentBuffer.size(); ++i) {
        std::cout << i << ". " << CurrentBuffer[i] << "\n";
    }
}

bool __cdecl IsCorrectRow(int row) {
    int index = 0;
    const int Rows[] = {1, 3, 6, 4, 8, 19, 13, 16, 45, 82, 17, 36};
    while (index < (int)(sizeof(Rows) / sizeof(const int))) {
        if (row == Rows[index]) return true;
        index++;
    }
    return false;
}

void RenderEqualizer::EnableVisualizer(std::vector<double>& freq, std::vector<double>& wave, std::mutex& magMutex, int sampleRate, JsonFileReader& jsonFileReader) {
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    termWidth  = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    termHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

    numBins = termWidth * 2;
    N_BARS = termWidth / 3;
    if (numBins < 1) numBins = 1;
    if (N_BARS < 1) N_BARS = 1;

    unsigned int frameCount = 0;
    std::string frame;
    frame.reserve(termWidth * termHeight * 10);
    
    int initialMaxBins = std::max(numBins, N_BARS);
    barValues.assign(initialMaxBins, 0.0f);
    peakValues.assign(initialMaxBins, 0.0f);
    peakDecay.assign(initialMaxBins, 0.0f);
    waveValues.assign(initialMaxBins, 0.5f);
    
    // Winamp dynamics
    std::vector<float> peakVelocity(initialMaxBins, 0.0f);
    std::vector<int> peakHold(initialMaxBins, 0);

    const struct ThemeModeManager themeModeManager[] = {
        {"Default Mode", ThemeMode::Mode0},
        {"Mode 1", ThemeMode::Mode1},
        {"Mode 2", ThemeMode::Mode2},
        {"Mode 3", ThemeMode::Mode3},
        {"Mode 4", ThemeMode::Mode4},
        {"Pink Mode", ThemeMode::Mode5},
        {"Gradient Mode", ThemeMode::Mode6}
    };
    size_t sizeOfTMM = sizeof(themeModeManager) / sizeof(const struct ThemeModeManager);
    enum ThemeMode themeMode = ThemeMode::Mode0;
    
    std::cout << "\033[?1049h\033[H\033[2J\033[?25l" << std::flush;

    bool themeChanged = true;
    static bool lastMState = false;

    // AGC / Rolling Normalization - Balanced 'Goldilocks' Tuning
    float rollingMax = 70.0f;
    const float noiseFloor = 50.0f; // High enough to be clean, low enough for bass body

    while (AudioEngine::Get().IsRunning()) {
        // Theme Switching
        size_t index0 = 0;
        size_t themesArraySize = jsonFileReader.themes.size();
        while (index0 < themesArraySize) {
            if (GetAsyncKeyState(jsonFileReader.themes.at(index0).key) & 0x8000) {
                std::memcpy(&jsonFileReader.currentTheme, &jsonFileReader.themes.at(index0), sizeof(struct Theme));
                strcpy_s((char*)jsonFileReader.currentTheme.themeName, sizeof(jsonFileReader.currentTheme.themeName), jsonFileReader.themes.at(index0).themeName);
                strcpy_s((char*)jsonFileReader.currentTheme.themeId, sizeof(jsonFileReader.currentTheme.themeId), jsonFileReader.themes.at(index0).themeId);
                strcpy_s((char*)jsonFileReader.currentTheme.themeMode, sizeof(jsonFileReader.currentTheme.themeMode), jsonFileReader.themes.at(index0).themeMode);
                themeChanged = true;
                break; 
            }
            index0++;
        }

        if (themeChanged) {
            size_t tmIndex = 0;
            while (tmIndex < sizeOfTMM) {
                if (strcmp(themeModeManager[tmIndex].themeModeName, jsonFileReader.currentTheme.themeMode) == 0) {
                    themeMode = themeModeManager[tmIndex].themeMode;
                    break;
                }
                tmIndex++;
            }
            themeChanged = false;
            N_BARS = (themeMode == ThemeMode::Mode5 || themeMode == ThemeMode::Mode6) ? (termWidth / 3) : ((termWidth - 1) / 2);
            if (N_BARS < 1) N_BARS = 1;
            int resizeMaxBins = std::max(numBins, N_BARS);
            barValues.assign(resizeMaxBins, 0.0f);
            peakValues.assign(resizeMaxBins, 0.0f);
            peakVelocity.assign(resizeMaxBins, 0.0f);
            peakHold.assign(resizeMaxBins, 0);
            waveValues.assign(resizeMaxBins, 0.5f);
            std::cout << "\033[2J" << std::flush;
        }

        bool currentMState = (GetAsyncKeyState('M') & 0x8000) != 0;
        if (currentMState && !lastMState) {
            oscilloscopeMode = !oscilloscopeMode;
            std::cout << "\033[2J" << std::flush;
        }
        lastMState = currentMState;

        if (frameCount % 30 == 0) {
            GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
            int newWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;
            int newHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
            if (newWidth != termWidth || newHeight != termHeight) {
                termWidth = newWidth; termHeight = newHeight;
                numBins = termWidth * 2;
                N_BARS = (themeMode == ThemeMode::Mode5 || themeMode == ThemeMode::Mode6) ? (termWidth / 3) : ((termWidth - 1) / 2);
                int resizeMaxBins = std::max(numBins, N_BARS);
                barValues.assign(resizeMaxBins, 0.0f);
                peakValues.assign(resizeMaxBins, 0.0f);
                peakVelocity.assign(resizeMaxBins, 0.0f);
                peakHold.assign(resizeMaxBins, 0);
                waveValues.assign(resizeMaxBins, 0.5f);
                frame.reserve(termWidth * termHeight * 10);
                std::cout << "\033[2J" << std::flush;
            }
        }
        frameCount++;

        float masterVol = AudioEngine::Get().GenVolLevel();
        float frameMax = 0.0f;
        bool hasData = false;

        {
            std::lock_guard<std::mutex> lock(magMutex);
            if (!freq.empty()) {
                hasData = true;
                if (!oscilloscopeMode) {
                    for (int i = 0; i < N_BARS; i++) {
                        float fLow = 20.0f * std::pow(20000.0f / 20.0f, (float)i / N_BARS);
                        float fHigh = 20.0f * std::pow(20000.0f / 20.0f, (float)(i + 1) / N_BARS);
                        double binStart = (double)fLow * (double)(freq.size() - 1) / (sampleRate / 2.0);
                        double binEnd = (double)fHigh * (double)(freq.size() - 1) / (sampleRate / 2.0);
                        int iStart = std::max(0, std::min((int)std::floor(binStart), (int)freq.size() - 1));
                        int iEnd = std::max(0, std::min((int)std::ceil(binEnd), (int)freq.size() - 1));
                        
                        double pVal = -100.0; 
                        for (int j = iStart; j <= iEnd; j++) {
                            if (freq[j] > pVal) pVal = freq[j];
                        }

                        // Gentler Tilt (40% boost at high end) to keep bass prominent
                        float tilt = 1.0f + (float)i / (float)N_BARS * 0.4f; 
                        pVal *= tilt;

                        if (pVal > frameMax) frameMax = (float)pVal;

                        // AGC Scaling
                        float target = (float)((pVal - noiseFloor) / (rollingMax - noiseFloor));
                        target = std::max(0.0f, std::min(1.0f, target * masterVol));
                        
                        // Balanced Gamma Curve
                        target = std::pow(target, 1.75f);

                        // Balanced Falloff (0.035f) - Smooth but responsive
                        if (target > barValues[i]) {
                            barValues[i] = target;
                        } else {
                            barValues[i] -= 0.035f; 
                            if (barValues[i] < 0.0f) barValues[i] = 0.0f;
                        }

                        // Peak Physics
                        if (target >= peakValues[i]) {
                            peakValues[i] = target;
                            peakVelocity[i] = 0.0f;
                            peakHold[i] = 12; 
                        } else {
                            if (peakHold[i] > 0) {
                                peakHold[i]--;
                            } else {
                                peakVelocity[i] += 0.0015f; 
                                peakValues[i] -= peakVelocity[i];
                            }
                        }
                        if (peakValues[i] < barValues[i]) {
                            peakValues[i] = barValues[i];
                            peakVelocity[i] = 0.00f;
                        }
                    }

                    if (frameMax > rollingMax) {
                        rollingMax = rollingMax * 0.9f + frameMax * 0.1f;
                    } else {
                        rollingMax = rollingMax * 0.999f + frameMax * 0.001f;
                    }
                    if (rollingMax < 70.0f) rollingMax = 70.0f;

                } else if (!wave.empty()) {
                    int offset = 0;
                    for (int j = 0; j < (int)wave.size() - 1 && j < 500; j++) {
                        if (wave[j] < 0 && wave[j+1] >= 0) { offset = j; break; }
                    }
                    int remainingSamples = (int)wave.size() - offset;
                    for (int i = 0; i < numBins; i++) {
                        int idx = offset + (int)(i * (float)remainingSamples / numBins);
                        if (idx >= (int)wave.size()) idx = (int)wave.size() - 1;
                        float target = (float)wave[idx] * masterVol;
                        target = std::max(-1.0f, std::min(1.0f, target * 0.9f));
                        float targetCentered = (target + 1.0f) * 0.5f;
                        waveValues[i] = waveValues[i] * 0.4f + targetCentered * 0.6f;
                    }
                }
            }
        }

        if (!hasData) { std::this_thread::sleep_for(std::chrono::milliseconds(10)); continue; }

        frame.clear();
        int renderHeight = termHeight - 1;
        if (renderHeight < 1) renderHeight = 1;

        char themeColor[64];
        sprintf_s(themeColor, sizeof(themeColor), "\033[38;2;%d;%d;%dm", (int)(jsonFileReader.currentTheme.colorRed * 255), (int)(jsonFileReader.currentTheme.colorGreen * 255), (int)(jsonFileReader.currentTheme.colorBlue * 255));

        if (oscilloscopeMode) {
            for (int row = 0; row < renderHeight; row++) {
                int blockRow = renderHeight - 1 - row;
                int dotBase = blockRow * 4;
                for (int col = 0; col < termWidth; col++) {
                    int binL = col * 2; int binR = col * 2 + 1;
                    unsigned char mask = 0;
                    auto drawSegment = [&](int bIdx, int mCol) {
                        float val = waveValues[bIdx];
                        float prevVal = (bIdx > 0) ? waveValues[bIdx - 1] : val;
                        int h1 = (int)(prevVal * (renderHeight * 4 - 1));
                        int h2 = (int)(val * (renderHeight * 4 - 1));
                        int start = std::min(h1, h2); int end = std::max(h1, h2);
                        for (int y = start; y <= end; y++) {
                            if (y >= dotBase && y < dotBase + 4) {
                                int d = y - dotBase;
                                if (mCol == 0) {
                                    if (d == 0) mask |= 0x40; else if (d == 1) mask |= 0x04; else if (d == 2) mask |= 0x02; else if (d == 3) mask |= 0x01;
                                } else {
                                    if (d == 0) mask |= 0x80; else if (d == 1) mask |= 0x20; else if (d == 2) mask |= 0x10; else if (d == 3) mask |= 0x08;
                                }
                            }
                        }
                    };
                    drawSegment(binL, 0); if (binR < numBins) drawSegment(binR, 1);
                    if (mask > 0) { frame += themeColor; frame += getBraille(mask); frame += "\033[0m"; }
                    else frame += " ";
                }
                if (row < renderHeight - 1) frame += '\n';
            }
        } else if (themeMode == ThemeMode::Mode5 || themeMode == ThemeMode::Mode6) {
            for (int row = 0; row < renderHeight; row++) {
                int blockRow = renderHeight - 1 - row;
                char currentRowColor[64];
                if (themeMode == ThemeMode::Mode6) {
                    float ratio = (float)blockRow / (float)(renderHeight - 1);
                    int r = (int)(255 * ratio); int g = (int)(255 * (1.0f - ratio));
                    sprintf_s(currentRowColor, sizeof(currentRowColor), "\033[38;2;%d;%d;%dm", r, g, 0);
                } else strcpy_s(currentRowColor, sizeof(currentRowColor), themeColor);

                for (int col = 0; col < termWidth; col++) {
                    int barIdx = col / 3; int colInBar = col % 3;
                    if (barIdx < N_BARS && colInBar < 2) {
                        float val = barValues[barIdx];
                        int h = (int)(val * (renderHeight - 1));
                        float phVal = peakValues[barIdx];
                        int ph = (int)(phVal * (renderHeight - 1));
                        
                        if (blockRow <= h) { 
                            frame += currentRowColor; frame += "█"; frame += "\033[0m"; 
                        } else if (blockRow == ph && ph > 0) { 
                            if (themeMode == ThemeMode::Mode6) frame += "\033[38;2;128;128;128m";
                            else frame += currentRowColor;
                            frame += "─"; frame += "\033[0m";
                        } else {
                            frame += " ";
                        }
                    } else frame += " ";
                }
                if (row < renderHeight - 1) frame += '\n';
            }
        } else {
            const char* UTF8Codes[] = { "─", "│", "─", "│", "┌", "┐", "└", "┘", "├", "┤", "┬", "┴", "┼", "♪", "♫", "♬", "♩", "⣿", "⣶", "⣤", "⣀", "⡇", "#" };
            size_t sizeofCodes = sizeof(UTF8Codes) / sizeof(UTF8Codes[0]);
            for (int row = renderHeight; row > 0; row--) {
                for (int i = 0; i < N_BARS; i++) {
                    int barHeight = (int)(barValues[i] * renderHeight);
                    float rowA = float(row) / float(renderHeight);
                    float rowB = -rowA + 1.0f; float rowC = -fabs(rowA - 0.5f) + 1.0f;
                    float effect = (themeMode == ThemeMode::Mode4) ? barValues[i] * 2.5f : 1.0f;
                    int color[3] = { (int)((255 * rowA * effect) * jsonFileReader.currentTheme.colorRed), (int)((255 * rowB * effect) * jsonFileReader.currentTheme.colorGreen), (int)((255 * rowC * effect) * jsonFileReader.currentTheme.colorBlue) };
                    int barTop = 0, barBottom = 0;
                    if (themeMode == ThemeMode::Mode1) { barTop = (renderHeight - barHeight) / 2; barBottom = renderHeight - barTop; }
                    else if (themeMode == ThemeMode::Mode2) { barTop = barHeight; barBottom = barHeight; }
                    else if (themeMode == ThemeMode::Mode3) { int bh1 = renderHeight - barHeight; barTop = (renderHeight - bh1) / 2; barBottom = renderHeight - barTop; }
                    char character[2] = {jsonFileReader.currentTheme.customCharacter, '\0'};
                    char temp[128];
                    if (themeMode == ThemeMode::Mode0) {
                        if (row <= barHeight) sprintf_s(temp, sizeof(temp), "\033[38;2;%d;%d;%dm%s\033[0m", color[0], color[1], color[2], (jsonFileReader.currentTheme.useRandomCharacters) ? UTF8Codes[rand() % sizeofCodes] : character);
                        else sprintf_s(temp, sizeof(temp), " ");
                    } else if (themeMode == ThemeMode::Mode1 || themeMode == ThemeMode::Mode2 || themeMode == ThemeMode::Mode3) {
                        if (row <= barBottom && row >= barTop) sprintf_s(temp, sizeof(temp), "\033[38;2;%d;%d;%dm%s\033[0m", color[0], color[1], color[2], (jsonFileReader.currentTheme.useRandomCharacters) ? UTF8Codes[rand() % sizeofCodes] : character);
                        else sprintf_s(temp, sizeof(temp), " ");
                    } else if (themeMode == ThemeMode::Mode4) {
                        sprintf_s(temp, sizeof(temp), "\033[38;2;%d;%d;%dm%s\033[0m", color[0], color[1], color[2], (jsonFileReader.currentTheme.useRandomCharacters) ? UTF8Codes[rand() % sizeofCodes] : character);
                    }
                    frame += temp; frame += " ";
                }
                if (row > 1) frame += '\n';
            }
        }
        std::cout << "\033[H" << frame << "\033[J" << std::flush;        
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); 
    }
    std::cout << "\033[2J\033[H\033[?1049l\033[?25h" << std::flush;
}
