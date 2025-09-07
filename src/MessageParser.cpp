#include "MessageParser.hpp"
#include <nlohmann/json.hpp>

namespace ddc {

std::optional<ParsedMessage> MessageParser::parse(const Raw1553Message& raw) const {
    ParsedMessage p;
    p.rt = raw.rtAddress;
    p.sa = raw.subAddress;
    p.wc = raw.wordCount;
    p.modeCode = raw.isModeCode;
    p.transmit = raw.tx;
    p.channel = raw.channel;
    p.timestamp = raw.timestamp;
    p.data = raw.dataWords;
    return p;
}

nlohmann::json MessageParser::toJson(const ParsedMessage& msg) const {
    nlohmann::json j;
    j["rt"] = msg.rt;
    j["sa"] = msg.sa;
    j["wc"] = msg.wc;
    j["modeCode"] = msg.modeCode;
    j["tx"] = msg.transmit;
    j["channel"] = msg.channel;
    j["timestamp"] = msg.timestamp;
    // Convert data words to hex strings for readability
    std::vector<std::string> dataHex; dataHex.reserve(msg.data.size());
    for (auto w : msg.data) {
        char buf[8];
        std::snprintf(buf, sizeof(buf), "0x%04X", w);
        dataHex.emplace_back(buf);
    }
    j["data"] = dataHex;
    return j;
}

} // namespace ddc
