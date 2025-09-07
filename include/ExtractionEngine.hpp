#pragma once
#include "Config.hpp"
#include "MessageParser.hpp"
#include <nlohmann/json_fwd.hpp>
#include <unordered_map>
#include <mutex>

namespace ddc {

struct ExtractedValue {
    std::string name;
    double numericValue{};
    std::string type; // from config
    uint64_t timestamp{}; // source message timestamp
};

class ExtractionEngine {
public:
    explicit ExtractionEngine(const AppConfig& cfg);

    // Feed a parsed message; returns list of newly extracted values (raw instantaneous)
    std::vector<ExtractedValue> process(const ParsedMessage& msg);

    // Build JSON payload depending on batch vs immediate
    nlohmann::json buildJsonSnapshot();

private:
    const AppConfig& m_cfg;
    std::unordered_map<MsgKey, std::vector<FieldSpec>, MsgKeyHash> m_lookup;

    std::mutex m_mtx;
    // Latest values by field name
    std::unordered_map<std::string, ExtractedValue> m_latest;
};

} // namespace ddc
