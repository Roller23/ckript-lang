#if !defined(__ERROR_HANDLER_)
#define __ERROR_HANDLER_

#include <string>

class ErrorHandler {
  public:
    static void thow_syntax_error(const std::string &cause, int line = -1);
    static void throw_file_error(const std::string &cause);
};

#endif // __ERROR_HANDLER_