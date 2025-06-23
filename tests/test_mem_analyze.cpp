/**
 * @file test_mem_analyze.cpp
 * @author Sinter Wong (sintercver@gmail.com)
 * @brief
 * @version 0.1
 * @date 2025-05-08
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <gtest/gtest.h>

#include "utils/mem_analyze.hpp"

class MemoryAnalyzeTest : public ::testing::Test {
protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(MemoryAnalyzeTest, Normal) {
  utils::MemoryAnalyzer analyzer;
  auto result = analyzer.analyze([]() {
    std::vector<int> largeVector(1000000);
    std::fill(largeVector.begin(), largeVector.end(), 66);
  });
  result.print();
}

TEST_F(MemoryAnalyzeTest, MemLeak) {
  utils::MemoryAnalyzer analyzer;
  auto result = analyzer.analyze([]() {
    // 4byte x 20M
    size_t size = 20 * 1024 * 1024;
    int *leak = new int[size];
    std::fill(leak, leak + size, 66); // 不实际使用os可能会懒加载物理区域
  });
  result.print();
}
