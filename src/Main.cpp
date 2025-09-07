#include "B1553Monitor.hpp"
#include "MessageParser.hpp"
#include "UdpPublisher.hpp"
#include "JsonFormatter.hpp"
#include <iostream>
#include <nlohmann/json.hpp>
#ifdef DDC_ENABLE_LOGGING
#include <spdlog/spdlog.h>
#endif

using namespace ddc;

class StreamController {
public:
    bool init(const std::string& device, const std::string& host, uint16_t port) {
        if (!monitor.open(device)) return false;
        if (!udp.open(host, port)) return false;
        return true;
    }

    void run() {
        monitor.start([this](const Raw1553Message& raw){
            auto parsed = parser.parse(raw);
            if (!parsed) return;
            auto json = parser.toJson(*parsed);
            auto payload = JsonFormatter::compact(json);
            udp.send(payload);
#ifdef DDC_ENABLE_LOGGING
            spdlog::info("Sent {}", payload);
#endif
        });
    }

    void stop() { monitor.stop(); }

private:
    B1553Monitor monitor;
    MessageParser parser;
    UdpPublisher udp;
};

