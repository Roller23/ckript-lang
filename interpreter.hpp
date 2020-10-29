#ifndef __INTERPRETER_
#define __INTERPRETER_

#include "lexer.hpp"

class Interpreter {
  public:
    void process_file(const std::string &filename);
    void process_stream(void); // to do
};

#endif // __INTERPRETER_