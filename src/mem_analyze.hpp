/**
 * @file mem_analyze.hpp
 * @author Sinter Wong (sintercver@gmail.com)
 * @brief
 * @version 0.1
 * @date 2025-05-07
 *
 * @copyright Copyright (c) 2025
 *
 */
#ifndef __UTILS_MEM_ANALYZE_HPP__
#define __UTILS_MEM_ANALYZE_HPP__

#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

namespace utils {
class MemoryAnalyzer {
public:
  struct AnalysisResult {
    long rssKbBefore = -1, rssKbAfter = -1;
    long vmDataKbBefore = -1, vmDataKbAfter = -1;
    long pssKbBefore = -1, pssKbAfter = -1;
    // 也可以加入VmSize, VmHWM等

    long getRssDiffKb() const {
      if (rssKbBefore < 0 || rssKbAfter < 0)
        return 0;
      return rssKbAfter - rssKbBefore;
    }

    long getVmDataDiffKb() const {
      if (vmDataKbBefore < 0 || vmDataKbAfter < 0)
        return 0;
      return vmDataKbAfter - vmDataKbBefore;
    }

    long getPssDiffKb() const {
      if (pssKbBefore < 0 || pssKbAfter < 0)
        return 0;
      return pssKbAfter - pssKbBefore;
    }

    static std::string formatKb(long kb, bool showSign = false) {
      if (kb < 0 && !showSign)
        return "N/A";

      std::string signStr;
      if (showSign) {
        if (kb > 0)
          signStr = "+";
      }
      return signStr + std::to_string(kb) + " KB";
    }

    void print() const {
      auto printMetric = [](const std::string &name, long before, long after,
                            long diff) {
        std::cout << std::left << std::setw(12) << (name + " Before: ")
                  << (before >= 0 ? formatKb(before) : "N/A") << std::endl;
        std::cout << std::left << std::setw(12) << (name + " After: ")
                  << (after >= 0 ? formatKb(after) : "N/A") << std::endl;
        std::cout << std::left << std::setw(12) << (name + " Diff: ")
                  << (before >= 0 && after >= 0 ? formatKb(diff, true) : "N/A")
                  << std::endl;
      };

      printMetric("RSS", rssKbBefore, rssKbAfter, getRssDiffKb());
      printMetric("VmData", vmDataKbBefore, vmDataKbAfter, getVmDataDiffKb());
      printMetric("PSS", pssKbBefore, pssKbAfter, getPssDiffKb());
    }
  };

  MemoryAnalyzer() {}

private:
  // /proc/self/status 获取特定指标的通用函数
  long getMetricFromStatusFile(const std::string &metricPrefix) {
    long valueKb = 0;
    bool found = false;
    std::ifstream statusFile("/proc/self/status");
    if (!statusFile.is_open()) {
      std::cerr << "Error: Could not open /proc/self/status to get '"
                << metricPrefix << "'." << std::endl;
      return -1;
    }

    std::string line;
    while (std::getline(statusFile, line)) {
      if (line.rfind(metricPrefix, 0) == 0) { // rfind from start of line
        std::istringstream iss(line);
        std::string key;
        std::string unit; // kB
        iss >> key >> valueKb >> unit;
        if (iss.fail() ||
            (unit != "kB" &&
             !unit.empty())) { // unit might be empty if value is 0
          std::cerr << "Warning: Failed to parse line or unexpected unit for "
                    << metricPrefix << ": " << line << std::endl;
          statusFile.close();
          return -1;
        }
        found = true;
        break;
      }
    }
    statusFile.close();
    if (!found) {
      std::cerr << "Warning: " << metricPrefix
                << " line not found in /proc/self/status." << std::endl;
      return -1;
    }
    return valueKb;
  }

  // /proc/self/smaps_rollup 获取 PSS
  long getPssFromSmapsRollupKb() {
    long pssKb = 0;
    bool found = false;
    std::ifstream smapsFile("/proc/self/smaps_rollup");
    if (!smapsFile.is_open()) {
      // smaps_rollup 可能不存在于较旧的内核
      std::cerr << "Info: Could not open /proc/self/smaps_rollup. PSS (rollup) "
                   "not available."
                << std::endl;
      return -1;
    }

    std::string line;
    while (std::getline(smapsFile, line)) {
      if (line.rfind("Pss:", 0) == 0) {
        std::istringstream iss(line);
        std::string key;
        std::string unit; // kB
        iss >> key >> pssKb >> unit;
        if (iss.fail() || (unit != "kB" && !unit.empty())) {
          std::cerr << "Warning: Failed to parse Pss line or unexpected unit "
                       "from /proc/self/smaps_rollup: "
                    << line << std::endl;
          smapsFile.close();
          return -1;
        }
        found = true;
        break;
      }
    }
    smapsFile.close();
    if (!found) {
      std::cerr << "Warning: Pss line not found in /proc/self/smaps_rollup."
                << std::endl;
      return -1;
    }
    return pssKb;
  }

public:
  long getCurrentRssKb() { return getMetricFromStatusFile("VmRSS:"); }

  long getCurrentVmDataKb() { return getMetricFromStatusFile("VmData:"); }

  long getCurrentPssKb() {
    long pss = getPssFromSmapsRollupKb();
    // if (pss < 0) {
    //   // 可选: 尝试从 /proc/self/smaps 获取 (更复杂)
    //   // pss = getPssFromSmapsKb();
    // }
    return pss;
  }

  AnalysisResult analyze(const std::function<void()> &functionToProfile) {
    AnalysisResult result;

    result.rssKbBefore = getCurrentRssKb();
    result.vmDataKbBefore = getCurrentVmDataKb();
    result.pssKbBefore = getCurrentPssKb();

    functionToProfile();

    result.rssKbAfter = getCurrentRssKb();
    result.vmDataKbAfter = getCurrentVmDataKb();
    result.pssKbAfter = getCurrentPssKb();

    return result;
  }
};
} // namespace utils

#endif