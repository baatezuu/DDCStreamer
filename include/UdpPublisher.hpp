#pragma once
#include <string>
#include <cstdint>
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mutex>

#pragma comment(lib, "ws2_32.lib")

namespace ddc {

class UdpPublisher {
public:
    UdpPublisher();
    ~UdpPublisher();

    bool open(const std::string& host, uint16_t port);
    bool send(const std::string& payload);
    void close();

private:
    SOCKET m_sock{INVALID_SOCKET};
    sockaddr_in m_addr{};
    std::mutex m_mtx;
    bool m_initialized{false};
};

} // namespace ddc
