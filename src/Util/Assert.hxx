#pragma once

#include <string_view>

#define assume(condition, msg)                                                 \
  assume_impl(__FILE__, __LINE__, __func__, (condition), (msg))
#define exists(ptr, msg) assume((ptr) != nullptr, (msg))
#define unreachable(msg)                                                       \
  assume(false, (msg));                                                        \
  __builtin_unreachable()
#define unimplemented(msg) unreachable(msg)

void assume_impl(std::string_view filename, int line_num,
                 std::string_view func_name, bool, std::string_view message);
void assume_impl(std::string_view filename, int line_num,
                 std::string_view func_name, bool,
                 const char *(*get_message)(void));
