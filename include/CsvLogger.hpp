#pragma once
#include <string>
#include <fstream>
#include <mutex>
#include <unordered_map>
#include <vector>
#include "ExtractionEngine.hpp"

namespace ddc {

class CsvLogger {
public:
    bool open(const std::string& path);
    // Provide full ordered list of field names (original dotted form) to fix header
    void setColumns(const std::vector<std::string>& names);
    void writeValues(const std::vector<ExtractedValue>& values);
    void flush();
private:
    std::ofstream m_ofs;
    std::mutex m_mtx;
    bool m_headerWritten{false};
    std::vector<std::string> m_columnsOriginal; // dotted names
    std::vector<std::string> m_columnsCsv;      // converted names ('.' -> '/')
    std::unordered_map<std::string, double> m_lastValues; // last seen values
};

} // namespace ddc
