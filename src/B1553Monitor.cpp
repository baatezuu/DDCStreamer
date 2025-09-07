#include "B1553Monitor.hpp"
#include <chrono>
#include <thread>
#include <random>

// TODO: Replace placeholders with actual DDC aceXtreme API calls.
// This skeleton simulates message reception until integrated with hardware.

namespace ddc {

B1553Monitor::B1553Monitor() = default;
B1553Monitor::~B1553Monitor() { stop(); }

bool B1553Monitor::open(const std::string& deviceName, uint32_t channelMask) {
    m_channelMask = channelMask;
    // TODO: Use aceXtreme API: aceInitialize(), aceOpenChannel(), configure monitor mode, etc.
    m_deviceHandle = reinterpret_cast<void*>(0x1); // fake non-null
    return true;
}

bool B1553Monitor::start(MessageCallback cb) {
    if (m_running.load()) return false;
    m_callback = std::move(cb);
    m_running = true;
    m_thread = std::thread(&B1553Monitor::monitorLoop, this);
    return true;
}

void B1553Monitor::stop() {
    if (!m_running.load()) return;
    m_running = false;
    if (m_thread.joinable()) m_thread.join();
    // TODO: Close hardware handle via aceClose()
    m_deviceHandle = nullptr;
}

void B1553Monitor::monitorLoop() {
    using namespace std::chrono_literals;
    auto start = std::chrono::steady_clock::now();
    std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution<uint16_t> wordDist(0, 0xFFFF);
    static uint16_t incBase = 0;
    std::uniform_int_distribution<int> boolDist(0,1);
    size_t rtIdx = 0, saIdx = 0;
    auto simInterval = std::chrono::duration<double>(m_simIntervalSec);
    while (m_running.load()) {
        Raw1553Message msg;
        if (m_simulation) {
            if (m_simRTs.empty()) m_simRTs = {5};
            if (m_simSAs.empty()) m_simSAs = {17,22};
            msg.rtAddress = m_simRTs[rtIdx++ % m_simRTs.size()];
            msg.subAddress = m_simSAs[saIdx++ % m_simSAs.size()];
            // Previously was random (boolDist). This caused half the simulated messages
            // to have tx=true while config fields (e.g. "17R" / "22R") expect receive (tx=false).
            // That mismatch meant no extraction early on, giving the impression of no data.
            // For deterministic simulation aligned with typical monitor direction, force tx=false.
            msg.tx = false; // simulate receive (R) direction consistently
            msg.wordCount = m_simWC;
            msg.isModeCode = false;
            msg.channel = 0;
            msg.dataWords.resize(m_simWC);
            for (auto &w : msg.dataWords) {
                if (m_simPatternRandom) {
                    w = wordDist(rng);
                } else {
                    w = incBase++;
                }
            }
        } else {
            // Placeholder hardware fetch
            msg.rtAddress = 1; msg.tx=false; msg.subAddress=2; msg.wordCount=4; msg.isModeCode=false; msg.channel=0;
            msg.dataWords = {0x1111,0x2222,0x3333,0x4444};
        }
        auto now = std::chrono::steady_clock::now();
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(now - start).count();
        msg.timestamp = static_cast<uint64_t>(us);
        msg.statusWord1 = 0; msg.statusWord2 = 0;
        if (m_callback) m_callback(msg);
        if (m_simulation) {
            std::this_thread::sleep_for(simInterval);
        } else {
            std::this_thread::sleep_for(200ms);
        }
    }
}

void B1553Monitor::enableSimulation(double rateHz, const std::vector<uint16_t>& rts,
                          const std::vector<uint16_t>& subAddresses,
                          uint16_t wordCount,
                          bool randomPattern) {
    m_simulation = true;
    m_simIntervalSec = (rateHz > 0) ? 1.0 / rateHz : 0.02;
    m_simRTs = rts;
    m_simSAs = subAddresses;
    m_simWC = wordCount;
    m_simPatternRandom = randomPattern;
}

} // namespace ddc
