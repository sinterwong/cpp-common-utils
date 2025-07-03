#ifndef __UTILS_EXCEPTION_HPP_
#define __UTILS_EXCEPTION_HPP_

#include <stdexcept>
#include <variant>

namespace common_utils::exception {
#include <type_traits> // Required for std::is_same_v

// Helper type trait to check if a type T is one of Types...
template <typename T, typename... Types> struct is_one_of;

template <typename T, typename First, typename... Rest>
struct is_one_of<T, First, Rest...> {
  static constexpr bool value =
      std::is_same_v<T, First> || is_one_of<T, Rest...>::value;
};

template <typename T> struct is_one_of<T> {
  static constexpr bool value = false;
};

template <typename T, typename... Ts>
const T &get_or_throw(const std::variant<Ts...> &v) {
  if constexpr (is_one_of<T, Ts...>::value) {
    if (const auto ptr = std::get_if<T>(&v)) {
      return *ptr;
    }
    // If T is in Ts... but the variant currently holds a different type.
    // This part of the message might be slightly redundant if T wasn't held,
    // but it's clearer than just "Requested type not found".
    throw std::runtime_error(
        "Variant does not currently hold the requested type: " +
        std::string(typeid(T).name()));
  } else {
    // If T is not a possible type for this variant.
    throw std::runtime_error("Requested type " + std::string(typeid(T).name()) +
                             " is not a valid alternative for this variant");
  }
}

class InvalidValueException : public std::runtime_error {
public:
  explicit InvalidValueException(const std::string &message)
      : std::runtime_error("Invalid value: " + message) {}
};

class OutOfRangeException : public std::out_of_range {
public:
  explicit OutOfRangeException(const std::string &message)
      : std::out_of_range("Out of range: " + message) {}
};

class NullPointerException : public std::logic_error {
public:
  explicit NullPointerException(const std::string &message)
      : std::logic_error("Null pointer: " + message) {}
};

class FileOperationException : public std::runtime_error {
public:
  explicit FileOperationException(const std::string &message)
      : std::runtime_error("File operation error: " + message) {}
};

class NetworkException : public std::runtime_error {
public:
  explicit NetworkException(const std::string &message)
      : std::runtime_error("Network error: " + message) {}
};

class ExecutionException : public std::runtime_error {
public:
  explicit ExecutionException(const std::string &message)
      : std::runtime_error("Execution error: " + message) {}
};
} // namespace common_utils::exception

#endif