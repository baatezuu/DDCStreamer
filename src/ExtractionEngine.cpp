#include "ExtractionEngine.hpp"
#include <nlohmann/json.hpp>
#include <cmath>
#include <sstream>
#include <string>
#include <cstring>
#include <chrono>
#include <ctime>
#include <iomanip>

namespace ddc {

ExtractionEngine::ExtractionEngine(const AppConfig& cfg) : m_cfg(cfg) {
    for (const auto& stream : cfg.streams) {
        for (const auto& f : stream.fields) {
            MsgKey k{f.rt, f.subAddress, f.transmit};
            m_lookup[k].push_back(f);
        }
    }
}

std::vector<ExtractedValue> ExtractionEngine::process(const ParsedMessage& msg) {
    MsgKey k{msg.rt, msg.sa, msg.transmit};
    std::vector<ExtractedValue> out;
    auto it = m_lookup.find(k);
    if (it == m_lookup.end()) return out;

    for (const auto& spec : it->second) {
        // Bounds check
        int available = static_cast<int>(msg.data.size());
        if (spec.startWord < 1 || spec.endWord < 1 || spec.startWord > available || spec.endWord > available)
            continue;
        uint64_t timestamp = msg.timestamp;
        double value = 0.0;
        if (spec.singleBit) {
            int wIndex = spec.startWord - 1;
            uint16_t word = msg.data[wIndex];
            if (spec.bit >=0 && spec.bit < 16) {
                value = ((word >> spec.bit) & 0x1);
            }
        } else if (spec.startWord == spec.endWord) {
            uint16_t word = msg.data[spec.startWord - 1];
            if (spec.type == "signed") {
                int16_t s = static_cast<int16_t>(word);
                value = static_cast<double>(s);
            } else {
                value = word;
            }
        } else { // multi-word combine, first word is MSW per user spec
            // Combine sequential 16-bit words big-endian style (word order: W1 W2 -> (W1<<16)|W2)
            uint64_t accum = 0;
            int n = spec.endWord - spec.startWord + 1;
            for (int i=0;i<n;i++) {
                uint16_t w = msg.data[spec.startWord - 1 + i];
                accum = (accum << 16) | w;
            }
            if (spec.type == "ieee754" && ( ( (spec.endWord - spec.startWord +1) *16)==32 )) {
                uint32_t raw = static_cast<uint32_t>(accum & 0xFFFFFFFFu);
                float f; std::memcpy(&f, &raw, sizeof(f));
                value = static_cast<double>(f);
            } else if (spec.type == "signed") {
                // Signed combine (assume total bits <= 48). Sign bit at MSW highest bit.
                int totalBits = n * 16;
                uint64_t signMask = 1ull << (totalBits - 1);
                int64_t signedVal = (accum & signMask) ? (static_cast<int64_t>(accum) - (1ll << totalBits)) : static_cast<int64_t>(accum);
                value = static_cast<double>(signedVal);
            } else {
                value = static_cast<double>(accum);
            }
        }
        value *= spec.lsbScale; // apply scaling
        // Type conversions (basic)
        if (spec.type == "float") {
            // Interpret raw combined integer as signed? For now treat value already scaled.
        }
        ExtractedValue ev{spec.name, value, spec.type, timestamp};
        out.push_back(ev);
        std::lock_guard<std::mutex> lk(m_mtx);
        m_latest[spec.name] = ev;
    }
    return out;
}

// Helper: set nested path a.b.c = value
static void setNestedValue(nlohmann::json& root, const std::string& path, double value) {
    size_t start = 0; nlohmann::json* cur = &root;
    while (true) {
        size_t dot = path.find('.', start);
        std::string key = (dot == std::string::npos) ? path.substr(start) : path.substr(start, dot - start);
        if (dot == std::string::npos) {
            (*cur)[key] = value;
            break;
        } else {
            cur = &((*cur)[key]);
        }
        start = dot + 1;
    }
}

nlohmann::json ExtractionEngine::buildJsonSnapshot() {
    nlohmann::json j;
    uint64_t latestTs = 0;
    {
        std::lock_guard<std::mutex> lk(m_mtx);
        for (auto& kv : m_latest) {
            const auto& name = kv.first;
            const auto& ev = kv.second;
            if (ev.timestamp > latestTs) latestTs = ev.timestamp;
            if (name.find('.') != std::string::npos) {
                setNestedValue(j, name, ev.numericValue);
            } else {
                j[name] = ev.numericValue;
            }
        }
    }
    // Add timestamps: microseconds and seconds float
    j["timestamp_us"] = latestTs;
    j["timestamp"] = static_cast<double>(latestTs) / 1e6; // seconds float
    // Add ISO 8601 datetime (wall clock) similar to python scripts for compatibility
    try {
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        auto tm = *std::gmtime(&t); // use UTC
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
        j["datetime"] = oss.str();
    } catch(...) {
        // ignore failures silently
    }
    return j;
}

} // namespace ddc
