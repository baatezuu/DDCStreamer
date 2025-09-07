#include "UdpPublisher.hpp"
#include <cstring>

namespace ddc {

UdpPublisher::UdpPublisher() {
    WSADATA wsaData{};
    if (WSAStartup(MAKEWORD(2,2), &wsaData) == 0) {
        m_initialized = true;
    }
}

UdpPublisher::~UdpPublisher() { close(); if (m_initialized) WSACleanup(); }

bool UdpPublisher::open(const std::string& host, uint16_t port) {
    std::lock_guard<std::mutex> lk(m_mtx);
    if (!m_initialized) return false;
    if (m_sock != INVALID_SOCKET) return true;

    m_sock = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_sock == INVALID_SOCKET) return false;

    std::memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin_family = AF_INET;
    m_addr.sin_port = htons(port);
    #if !defined(INET_PTON_AVAILABLE)
    #if !defined(AF_INET6)
    #define INET_PTON_AVAILABLE 0
    #endif
    #endif

    int ptonResult = -1;
    #ifdef inet_pton
        ptonResult = ::inet_pton(AF_INET, host.c_str(), &m_addr.sin_addr);
    #else
        // Fallback for some MinGW variants
        unsigned long addr = ::inet_addr(host.c_str());
        if (addr != INADDR_NONE) { m_addr.sin_addr.s_addr = addr; ptonResult = 1; } else { ptonResult = 0; }
    #endif

    if (ptonResult != 1) {
        return false; // No DNS resolve fallback in minimal MinGW build
    }
    return true;
}

bool UdpPublisher::send(const std::string& payload) {
    std::lock_guard<std::mutex> lk(m_mtx);
    if (m_sock == INVALID_SOCKET) return false;
    int sent = ::sendto(m_sock, payload.c_str(), static_cast<int>(payload.size()), 0,
                        reinterpret_cast<sockaddr*>(&m_addr), sizeof(m_addr));
    return sent == static_cast<int>(payload.size());
}

void UdpPublisher::close() {
    std::lock_guard<std::mutex> lk(m_mtx);
    if (m_sock != INVALID_SOCKET) {
        closesocket(m_sock);
        m_sock = INVALID_SOCKET;
    }
}

} // namespace ddc
