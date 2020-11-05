#if !defined(__ERROR_HANDLER_)
#define __ERROR_HANDLER_

#include <string>
#include <cstdint>

class ErrorHandler {
  public:
    static void throw_syntax_error(const std::string &cause, std::uint32_t line = 0);
    static void throw_runtime_error(const std::string &cause, std::uint32_t line = 0);
    static void throw_file_error(const std::string &cause);
    static void throw_generic_error(const std::string &cause, std::uint32_t line = 0);
};

#endif // __ERROR_HANDLER_