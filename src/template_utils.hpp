#ifndef _UTILS_TEMPLATE_UTILS_HPP__
#define _UTILS_TEMPLATE_UTILS_HPP__

#include <functional>
#include <vector>

namespace utils::tpl {

template <typename T> struct get_first_arg_type;

template <typename Ret, typename Arg1, typename... Args>
struct get_first_arg_type<std::function<Ret(Arg1, Args...)>> {
  using type = Arg1;
};

template <typename T> struct get_vector_element_type;

template <typename T> struct get_vector_element_type<std::vector<T>> {
  using type = T;
};

} // namespace utils::tpl

#endif