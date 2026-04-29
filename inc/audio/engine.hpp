#ifndef ENGINE_HPP
#define ENGINE_HPP

#include <windows.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <audioclient.h>
#include <mmeapi.h>
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>

class AudioEngine {
public:
    AudioEngine(const AudioEngine&) = delete; // remove copy constructor
    AudioEngine(AudioEngine&&) = delete; // remove move constructor

    // Access class statically
    static AudioEngine& Get();

    float GenVolLevel();

    std::vector<double> GetCurrentBuffer();

    // todo: get audio which is passable via an array of floats/doubles
    float FloatVolume();

    // in AudioEngine header
    UINT32 GetSampleRate() const { return pWAVEFORMATEX ? pWAVEFORMATEX->nSamplesPerSec : 44100; }
    bool IsRunning() const { return running; }
    void Stop() { running = false; }

private:
    // Initialize object
    AudioEngine();

    ~AudioEngine();

    bool running = true;
    WORD bitsPerSample = 0;
    WORD nChannels = 0;
    bool isFloat = false;

    // Initialize member variables (volume level)
    IMMDeviceEnumerator* pEnumerator = NULL;
    IMMDevice* pDevice = NULL;
    IAudioEndpointVolume* pEndpointVolume = NULL;

    // Initialize member variables (audio samples)
    IAudioClient* pAudioClient = nullptr; // manage audio client
    IAudioCaptureClient* pAudioCaptureClient = nullptr; // capture buffer
    WAVEFORMATEX* pWAVEFORMATEX = nullptr; // [4]

    // temp retreival functions 
    float InternalGenVol();
    std::vector<double> InternalBuffer();

    // temp visualizer functions
    void DisplayMaster(); // display all audio samples

    std::vector<double> buffer;

};

#endif