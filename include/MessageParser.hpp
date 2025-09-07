#pragma once
#include "B1553Monitor.hpp"
#include <nlohmann/json_fwd.hpp>
#include <optional>

namespace ddc {

struct ParsedMessage {
    uint16_t rt{};
    uint16_t sa{};
    uint16_t wc{}; // word count or mode code
    bool modeCode{};
    bool transmit{}; // direction relative to RT
    uint16_t channel{};
    uint64_t timestamp{};
    std::vector<uint16_t> data;
};

class MessageParser {
public:
    std::optional<ParsedMessage> parse(const Raw1553Message& raw) const;
    nlohmann::json toJson(const ParsedMessage& msg) const;
};

} // namespace ddc
