#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <cstdint>

namespace ddc {

struct FieldSpec {
    std::string name;            // e.g., velocity
    uint16_t rt{};               // Remote Terminal address
    uint16_t subAddress{};       // Subaddress
    bool transmit{};             // true if from RT (T), false if to RT (R). Config uses 'R'/'T'.
    int startWord{};             // 1-based word index in message
    int endWord{};               // inclusive 1-based (for multi-word)
    int bit{};                   // optional bit index (0-15) if single bit
    bool singleBit{false};
    double lsbScale{1.0};        // scaling factor; if config has lsb_exp = -7 -> scale=2^-7
    std::string type;            // raw,uint,float
};

struct StreamConfig {
    std::string name; // grouping name e.g., "transfer_alignment"
    std::vector<FieldSpec> fields;
};

struct AppConfig {
    std::vector<StreamConfig> streams;              // field groups
    uint16_t udpPort{5555};
    std::string udpHost{"127.0.0.1"};
    std::string device{"ACE0"};
    uint32_t channelMask{0x3};
    double outputRateHz{50.0};                      // optional aggregated output rate
    bool batchMessages{false};                      // if true, send grouped JSON arrays per tick
    std::string csvPath;                            // if non-empty, write CSV
    bool simulation{false};                         // run without hardware
    double simRateHz{50.0};                         // simulation message emission rate
    std::string simPattern{"random"};              // random | increment
};

class ConfigLoader {
public:
    static std::optional<AppConfig> loadFromFile(const std::string& path, std::string& err);
};

// Key for map lookups: (rt, sa, tx flag)
struct MsgKey {
    uint16_t rt; uint16_t sa; bool tx; // tx true means RT->BC (transmit), false BC->RT (receive)
    bool operator==(const MsgKey& o) const { return rt==o.rt && sa==o.sa && tx==o.tx; }
};

struct MsgKeyHash { size_t operator()(const MsgKey& k) const { return (k.rt<<11) ^ (k.sa<<1) ^ (k.tx?1:0); } };

} // namespace ddc
