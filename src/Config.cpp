#include "Config.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <cmath>

namespace ddc {

static bool parseField(const nlohmann::json& jf, FieldSpec& f, std::string& err) {
    try {
        f.name = jf.at("name").get<std::string>();
        auto msg = jf.at("message").get<std::string>(); // e.g. "17R" or "22T"
        if (msg.size() < 2) { err = "message format"; return false; }
        f.subAddress = static_cast<uint16_t>(std::stoi(msg.substr(0, msg.size()-1)));
        char dir = (char)std::toupper(msg.back());
        f.transmit = (dir == 'T');
        f.rt = jf.at("rt").get<uint16_t>();
        if (jf.contains("words")) {
            auto w = jf.at("words");
            if (w.is_array() && w.size()==2) {
                f.startWord = w[0].get<int>();
                f.endWord = w[1].get<int>();
            }
        } else {
            f.startWord = jf.at("word").get<int>();
            f.endWord = f.startWord;
        }
        if (jf.contains("bit")) { f.bit = jf.at("bit").get<int>(); f.singleBit = true; }
        // LSB scaling: prefer exponent form lsb_exp (power-of-two). Fallback to direct lsb.
        if (jf.contains("lsb_exp")) {
            int exp = jf.at("lsb_exp").get<int>();
            f.lsbScale = std::pow(2.0, exp);
        } else if (jf.contains("lsb")) {
            f.lsbScale = jf.at("lsb").get<double>();
        }
        f.type = jf.value("type", "raw");
        return true;
    } catch (const std::exception& e) { err = e.what(); return false; }
}

std::optional<AppConfig> ConfigLoader::loadFromFile(const std::string& path, std::string& err) {
    std::ifstream ifs(path);
    if(!ifs) { err = "Cannot open config file"; return std::nullopt; }
    nlohmann::json j; ifs >> j;
    AppConfig cfg;
    // Default port aligned with PlotJuggler python examples (9870)
    cfg.udpPort = j.value("udp_port", 9870);
    cfg.udpHost = j.value("udp_host", std::string("127.0.0.1"));
    cfg.device = j.value("device", std::string("ACE0"));
    cfg.channelMask = j.value("channel_mask", 0x3u);
    cfg.outputRateHz = j.value("output_rate_hz", 50.0);
    cfg.batchMessages = j.value("batch", false);
    cfg.csvPath = j.value("csv_path", std::string());
    cfg.simulation = j.value("simulation", false);
    cfg.simRateHz = j.value("sim_rate_hz", 50.0);
    cfg.simPattern = j.value("sim_pattern", std::string("random"));
    if (j.contains("streams")) {
        for (auto& js : j["streams"]) {
            StreamConfig sc; sc.name = js.value("name", std::string());
            if (js.contains("fields")) {
                for (auto& jf : js["fields"]) {
                    FieldSpec f; if (parseField(jf, f, err)) sc.fields.push_back(f); else return std::nullopt;
                }
            }
            cfg.streams.push_back(std::move(sc));
        }
    }
    return cfg;
}

} // namespace ddc
