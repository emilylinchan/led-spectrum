#include "../inc/network/udp_sender.hpp"
#include <iostream>
#include <algorithm>
#include <cstring>

UdpSender::UdpSender()
{
    // Initialise Winsock
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cerr << "[UdpSender] WSAStartup failed: " << WSAGetLastError() << "\n";
        return;
    }

    // Create a UDP socket
    m_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_sock == INVALID_SOCKET) {
        std::cerr << "[UdpSender] socket() failed: " << WSAGetLastError() << "\n";
        WSACleanup();
        return;
    }

    // Fill in the destination address once
    std::memset(&m_dest, 0, sizeof(m_dest));
    m_dest.sin_family = AF_INET;
    m_dest.sin_port   = htons(ESP32_PORT);

    if (inet_pton(AF_INET, ESP32_IP, &m_dest.sin_addr) != 1) {
        std::cerr << "[UdpSender] inet_pton failed for IP: " << ESP32_IP << "\n";
        closesocket(m_sock);
        WSACleanup();
        return;
    }

    m_ready = true;
    std::cout << "[UdpSender] Ready → " << ESP32_IP << ":" << ESP32_PORT << "\n";
}

UdpSender::~UdpSender()
{
    if (m_sock != INVALID_SOCKET) {
        closesocket(m_sock);
    }
    WSACleanup();
}

void UdpSender::Send(const float* barValues, int count, bool oscilloscope, float r, float g, float b)
{
    if (!m_ready || !barValues) return;

    // Payload: [mode][R][G][B][h0..h31] = 4 + N_LED_COLS = 36 bytes
    static constexpr int HEADER = 4;
    const int n = std::min(count, N_LED_COLS);
    uint8_t payload[HEADER + N_LED_COLS] = {};

    // Convert float values (0.0–1.0) to uint8_t (0–255)
    auto toByte = [](float v) -> uint8_t {
        if (v < 0.0f) v = 0.0f;
        if (v > 1.0f) v = 1.0f;
        return static_cast<uint8_t>(v * 255.0f);
    };

    // Pack the payload and convert bar values to bytes
    payload[0] = oscilloscope ? 1 : 0; 
    payload[1] = toByte(r);           
    payload[2] = toByte(g);
    payload[3] = toByte(b);

    for (int i = 0; i < n; ++i) {
        payload[HEADER + i] = toByte(barValues[i]);
    }

    // Fire and forget (return value ignored, a dropped frame is just replaced by the next one)
    sendto(
        m_sock,
        reinterpret_cast<const char*>(payload),
        HEADER + N_LED_COLS,
        0,
        reinterpret_cast<const sockaddr*>(&m_dest),
        sizeof(m_dest)
    );
}