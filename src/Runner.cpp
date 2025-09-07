#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <atomic>
#include "B1553Monitor.hpp"
#include "MessageParser.hpp"
#include "UdpPublisher.hpp"
#include "JsonFormatter.hpp"
#include "Config.hpp"
#include "ExtractionEngine.hpp"
#include "CsvLogger.hpp"
#include <unordered_set>
#include <fstream>

int main(int argc, char* argv[]) {
    std::string configPath = "config.json"; // legacy default name if present
    bool userProvidedConfig = false;
    int overridePort = -1;
    for (int i=1;i<argc;++i) {
        std::string a = argv[i];
        if ((a == "-c" || a == "--config") && i+1 < argc) {
            configPath = argv[++i];
            userProvidedConfig = true;
        } else if ((a == "-p" || a == "--port") && i+1 < argc) {
            try { overridePort = std::stoi(argv[++i]); } catch(...) { std::cerr << "Invalid port after " << a << "\n"; return 1; }
        } else if (a == "--help" || a == "-h") {
            std::cout << "Usage: DDCStreamerApp [-c config.json] [-p port]\n"
                         "  -c, --config   Path to configuration JSON.\n"
                         "                  If omitted, first existing is chosen from:\n"
                         "                  config.nested.sample.json, config.sample.json, config.json\n"
                         "  -p, --port     Override UDP port (ignores udp_port in config)\n";
            return 0;
        }
    }

    if (!userProvidedConfig) {
        // Pick the first existing candidate; prefer nested sample.
        const char* candidates[] = {"config.nested.sample.json", "config.sample.json", "config.json"};
        bool picked = false;
        for (auto c : candidates) {
            std::ifstream f(c);
            if (f.good()) { configPath = c; picked = true; break; }
        }
        if (!picked) {
            std::cerr << "No configuration file found (looked for config.nested.sample.json, config.sample.json, config.json)." << std::endl;
            return 1;
        } else {
            std::cout << "No -c provided; using default config: " << configPath << std::endl;
        }
    }

    std::string err;
    auto cfgOpt = ddc::ConfigLoader::loadFromFile(configPath, err);
    if(!cfgOpt){ std::cerr << "Config load error: " << err << std::endl; return 1; }
    // Make a mutable copy so we can override selected values from CLI
    ddc::AppConfig cfg = *cfgOpt;
    if (overridePort > 0) {
        if (overridePort > 65535) { std::cerr << "Port out of range" << std::endl; return 1; }
        cfg.udpPort = static_cast<uint16_t>(overridePort);
    }

    ddc::B1553Monitor monitor;
    ddc::MessageParser parser;
    ddc::UdpPublisher udp;
    ddc::ExtractionEngine engine(cfg);
    ddc::CsvLogger csv;
    if(!cfg.csvPath.empty()) {
        // Collect full ordered field list for stable CSV header
        std::vector<std::string> allFields;
        for (auto &stream : cfg.streams) {
            for (auto &f : stream.fields) allFields.push_back(f.name);
        }
        csv.setColumns(allFields);
        csv.open(cfg.csvPath);
    }

    if(!monitor.open(cfg.device, cfg.channelMask)) { std::cerr << "Failed to open device" << std::endl; return 1; }
    if (cfg.simulation) {
        // Derive RT/SA sets from config fields
        std::vector<uint16_t> rts, sas;
        {
            std::unordered_set<uint16_t> rtSet, saSet;
            for (auto &stream : cfg.streams) {
                for (auto &f : stream.fields) { rtSet.insert(f.rt); saSet.insert(f.subAddress); }
            }
            rts.assign(rtSet.begin(), rtSet.end());
            sas.assign(saSet.begin(), saSet.end());
        }
    bool randomPattern = (cfg.simPattern != "increment");
    monitor.enableSimulation(cfg.simRateHz, rts, sas, 32, randomPattern);
        // Pattern flag (reflection not direct; add setter if needed) -> quick hack via dynamic cast not available; adjust header? For brevity not modifying further.
    }
    if(!udp.open(cfg.udpHost, cfg.udpPort)) { std::cerr << "Failed to open UDP" << std::endl; return 1; }

    std::cout << "Streaming from " << cfg.device << " to " << cfg.udpHost << ":" << cfg.udpPort
              << " using config " << configPath;
    if (overridePort > 0) std::cout << " (port overridden)";
    std::cout << std::endl;

    std::atomic<bool> running{true};
    std::atomic<uint64_t> seq{0};

    monitor.start([&](const ddc::Raw1553Message& raw){
        auto p = parser.parse(raw);
        if(!p) return;
        auto extracted = engine.process(*p);
        if (!cfg.batchMessages) {
            for (auto& ev : extracted) {
                nlohmann::json j;
                // Include both microseconds and seconds
                j["timestamp_us"] = ev.timestamp;
                j["timestamp"] = static_cast<double>(ev.timestamp)/1e6;
                // Add ISO8601 datetime similar to python utilities
                try {
                    auto now = std::chrono::system_clock::now();
                    std::time_t t = std::chrono::system_clock::to_time_t(now);
                    auto tm = *std::gmtime(&t);
                    std::ostringstream oss;
                    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
                    j["datetime"] = oss.str();
                } catch(...) {}
                j["seq"] = seq.fetch_add(1, std::memory_order_relaxed);
                // Nested path support
                if (ev.name.find('.') != std::string::npos) {
                    // reuse same helper logic (simple reimplementation)
                    size_t start = 0; nlohmann::json* cur = &j;
                    std::string path = ev.name;
                    while (true) {
                        size_t dot = path.find('.', start);
                        std::string key = (dot == std::string::npos) ? path.substr(start) : path.substr(start, dot - start);
                        if (dot == std::string::npos) { (*cur)[key] = ev.numericValue; break; }
                        cur = &((*cur)[key]);
                        start = dot + 1;
                    }
                } else {
                    j[ev.name] = ev.numericValue;
                }
                std::string payload = j.dump();
                udp.send(payload);
            }
        }
        if (!extracted.empty() && !cfg.csvPath.empty()) csv.writeValues(extracted);
    });

    // If batch mode, run a rate-controlled loop
    std::thread batchThread;
    if (cfg.batchMessages) {
        batchThread = std::thread([&]{
            using namespace std::chrono;
            auto interval = duration<double>(1.0 / (cfg.outputRateHz > 0 ? cfg.outputRateHz : 50.0));
            while (running.load()) {
                auto snap = engine.buildJsonSnapshot();
                if (!snap.empty()) {
                    snap["seq"] = seq.fetch_add(1, std::memory_order_relaxed);
                    std::string payload = snap.dump();
                    udp.send(payload);
                }
                std::this_thread::sleep_for(interval);
            }
        });
    }

    std::cout << "Press Enter to stop..." << std::endl; std::string line; std::getline(std::cin, line);
    running = false;
    monitor.stop();
    if (batchThread.joinable()) batchThread.join();
    udp.close();
    csv.flush();
    return 0;
}
