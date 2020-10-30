#ifndef __INTERPRETER_
#define __INTERPRETER_

#include <string>

class Interpreter {
  public:
    void process_file(const std::string &filename);
    void run_thread();
    void throw_error(const std::string &cause, bool terminate);
};

#endif // __INTERPRETER_