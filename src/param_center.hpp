/**
 * @file param_center.hpp
 * @author Sinter Wong (sintercver@gmail.com)
 * @brief
 * @version 0.1
 * @date 2025-02-18
 *
 * @copyright Copyright (c) 2025
 *
 */
#ifndef __UTILS_PARAM_CENTER_HPP__
#define __UTILS_PARAM_CENTER_HPP__

#include <algorithm>
#include <variant>
namespace utils {
template <typename P> class ParamCenter {
public:
  using Params = P;

  template <typename T> void setParams(T params) {
    params_ = std::move(params);
  }

  template <typename Func> void visitParams(Func &&func) {
    std::visit([&](auto &&params) { std::forward<Func>(func)(params); },
               params_);
  }
  template <typename T> T *getParams() { return std::get_if<T>(&params_); }

private:
  Params params_;
};
} // namespace utils

#endif