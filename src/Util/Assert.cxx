#include "Assert.hxx"
#include <cstdlib>
#include <iostream>

void assume_impl(std::string_view filename, int line_num,
                 std::string_view func_name, bool condition,
                 std::string_view message) {
  if (!condition) {
    std::cerr << func_name << " @ " << filename << ":" << line_num << " - "
              << message << std::endl;
    std::abort();
  }
}

void assume_impl(std::string_view filename, int line_num,
                 std::string_view func_name, bool condition,
                 const char *(*get_message)(void)) {
  if (!condition) {
    std::cerr << func_name << " @ " << filename << ":" << line_num << " - "
              << get_message() << std::endl;
    std::abort();
  }
}
