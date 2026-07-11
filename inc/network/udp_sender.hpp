#pragma once

// Excludes the legacy winsock.h (which conflicts with winsock2.h below)
// Must be included before any header that pulls in <windows.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstdint>

static constexpr const char* ESP32_IP   = "192.168.8.103"; // Fixed IP of the ESP32 on the LAN (set in spectrum_esp32.ino)
                                                           // Use loopback address "127.0.0.1" for UDP sniffer testing 
static constexpr uint16_t    ESP32_PORT = 4210;            
static constexpr int         N_LED_COLS = 32;

class UdpSender {
    public:
        UdpSender();
        ~UdpSender();

        UdpSender(const UdpSender&)            = delete; // No copy constructor
        UdpSender& operator=(const UdpSender&) = delete; // No copy assignment operator

        bool IsReady() const { return m_ready; }
        void Send(const float* barValues, int count, bool oscilloscope, float r, float g, float b);

    private:
        // Winsock socket and destination address
        SOCKET       m_sock  = INVALID_SOCKET;
        sockaddr_in  m_dest  = {};
        bool         m_ready = false;
    };