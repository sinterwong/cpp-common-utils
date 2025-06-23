/**
 * @file pipeline_data.hpp
 * @author Sinter Wong (sintercver@gmail.com)
 * @brief
 * @version 0.1
 * @date 2025-05-30
 *
 * @copyright Copyright (c) 2025
 *
 */
#ifndef __UTILS_DATA_PACKET_HPP
#define __UTILS_DATA_PACKET_HPP

#include <any>
#include <cstdint>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>

namespace utils {
using DataPacketId = uint64_t;

struct DataPacket {
  DataPacketId id;
  std::map<std::string, std::any> params;

  template <typename T> T getParam(const std::string &key) const {
    auto it = params.find(key);
    if (it == params.end()) {
      throw std::runtime_error("Missing required parameter: " + key);
    }
    try {
      return std::any_cast<T>(it->second);
    } catch (const std::out_of_range &oor) {
      throw std::runtime_error("Key not found: " + key);
    } catch (const std::bad_any_cast &e) {
      throw std::runtime_error("Invalid parameter type for key '" + key +
                               "'. Expected type: " + typeid(T).name());
    }
  }

  template <typename T>
  std::optional<T> getOptionalParam(const std::string &key) const {
    auto it = params.find(key);
    if (it == params.end()) {
      return std::nullopt;
    }
    try {
      return std::any_cast<T>(it->second);
    } catch (const std::out_of_range &oor) {
      throw std::runtime_error("Key not found: " + key);
    } catch (const std::bad_any_cast &e) {
      throw std::runtime_error("Invalid parameter type for optional key '" +
                               key + "'. Expected type: " + typeid(T).name());
    }
  }

  template <typename T> void setParam(const std::string &key, T value) {
    params[key] = std::move(value);
  }
};
} // namespace utils

#endif