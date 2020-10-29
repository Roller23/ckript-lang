#include "interpreter.hpp"
#include <string>
#include <fstream>
#include <iostream>

int main(int argc, char *argv[]) {
  if (argc == 1) {
    Interpreter interpreter;
    // to do - create a shell
    // interpreter.process_stream();
    return 0;
  }
  Interpreter interpreter;
  // std::string filename = argv[1];
  interpreter.process_file("hello.ck");
  return 0;
}