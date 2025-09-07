#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace ddc {

class JsonFormatter {
public:
    static std::string compact(const nlohmann::json& j) { return j.dump(); }
    static std::string pretty(const nlohmann::json& j, int indent = 2) { return j.dump(indent); }
};

} // namespace ddc
