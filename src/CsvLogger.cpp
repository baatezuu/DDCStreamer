#include "CsvLogger.hpp"
#include <sstream>
#include <unordered_set>

namespace ddc {

bool CsvLogger::open(const std::string& path) {
    m_ofs.open(path, std::ios::out | std::ios::trunc);
    return static_cast<bool>(m_ofs);
}

void CsvLogger::setColumns(const std::vector<std::string>& names) {
    std::lock_guard<std::mutex> lk(m_mtx);
    if (m_headerWritten) return; // cannot change after header
    m_columnsOriginal = names;
    m_columnsCsv.clear();
    m_columnsCsv.reserve(names.size());
    for (auto &n : names) {
        std::string c = n;
        for (auto &ch : c) if (ch == '.') ch = '/';
        m_columnsCsv.push_back(std::move(c));
    }
}

void CsvLogger::writeValues(const std::vector<ExtractedValue>& values) {
    if (values.empty()) return;
    std::lock_guard<std::mutex> lk(m_mtx);
    // Update last-values map
    for (auto &v : values) {
        m_lastValues[v.name] = v.numericValue;
        // If a new column appears that wasn't declared, append (fallback)
        if (!m_headerWritten && std::find(m_columnsOriginal.begin(), m_columnsOriginal.end(), v.name) == m_columnsOriginal.end()) {
            m_columnsOriginal.push_back(v.name);
            std::string c = v.name; for (auto &ch : c) if (ch == '.') ch = '/';
            m_columnsCsv.push_back(std::move(c));
        }
    }
    if (!m_headerWritten) {
        if (m_columnsOriginal.empty()) { // fallback to current values ordering
            for (auto &v : values) {
                m_columnsOriginal.push_back(v.name);
                std::string c = v.name; for (auto &ch : c) if (ch == '.') ch = '/';
                m_columnsCsv.push_back(std::move(c));
            }
        }
        m_ofs << "timestamp";
        for (auto &col : m_columnsCsv) m_ofs << "," << col;
        m_ofs << "\n";
        m_headerWritten = true;
    }
    // Use timestamp of first updated value for the row
    uint64_t ts = values.front().timestamp;
    m_ofs << ts;
    for (auto &orig : m_columnsOriginal) {
        auto it = m_lastValues.find(orig);
        if (it != m_lastValues.end()) m_ofs << "," << it->second; else m_ofs << ","; // blank if unseen yet
    }
    m_ofs << "\n";
}

void CsvLogger::flush() { std::lock_guard<std::mutex> lk(m_mtx); m_ofs.flush(); }

} // namespace ddc
