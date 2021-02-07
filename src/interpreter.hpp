#if !defined(__INTERPRETER_)
#define __INTERPRETER_

#include <string>

class Interpreter {
  public:
    void process_file(const std::string &filename, int argc, char *argv[]);
};

#endif // __INTERPRETER_