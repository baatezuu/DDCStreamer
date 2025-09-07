#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <atomic>
#include <thread>
#include <vector>

// Forward declare aceXtreme SDK types if not included yet (replace with actual headers later)
struct ACE_MESSAGE_STRUCT; // Placeholder for an actual SDK message structure

namespace ddc {

struct Raw1553Message {
    uint16_t rtAddress{};   // RT (0-31)
    bool tx{};              // true if RT->BC transmission (Transmit direction flag)
    uint16_t subAddress{};  // Subaddress (0-31)
    uint16_t wordCount{};   // Word count or mode code
    bool isModeCode{};      // Word count field interpreted as mode code
    uint16_t channel{};     // Channel number (A/B)
    uint64_t timestamp{};   // Hardware timestamp (e.g., nanoseconds or microseconds)
    std::vector<uint16_t> dataWords; // Payload data words
    uint32_t statusWord1{}; // Primary status word
    uint32_t statusWord2{}; // Secondary status (if applicable)
};

using MessageCallback = std::function<void(const Raw1553Message&)>;

class B1553Monitor {
public:
    B1553Monitor();
    ~B1553Monitor();

    // Opens and initializes the board for monitoring.
    // deviceName: e.g., "ACE0" or board identifier from DDC API enumeration.
    bool open(const std::string& deviceName, uint32_t channelMask = 0x3); // Channel A/B both by default

    // Enable internal simulation instead of hardware (must be called before start)
    void enableSimulation(double rateHz, const std::vector<uint16_t>& rts,
                          const std::vector<uint16_t>& subAddresses,
                          uint16_t wordCount = 32,
                          bool randomPattern = true);

    // Start asynchronous monitoring loop.
    bool start(MessageCallback cb);

    // Stop monitoring loop and close device.
    void stop();

private:
    void monitorLoop();

    std::atomic<bool> m_running{false};
    std::thread m_thread;
    MessageCallback m_callback;
    void* m_deviceHandle{nullptr}; // Replace with real handle type (e.g., ACE_HANDLE)
    uint32_t m_channelMask{0};
    // Simulation parameters
    bool m_simulation{false};
    double m_simIntervalSec{0.02};
    std::vector<uint16_t> m_simRTs;
    std::vector<uint16_t> m_simSAs;
    uint16_t m_simWC{16};
    bool m_simPatternRandom{true};
};

} // namespace ddc
