#include "src/interpreter.hpp"
#include <string>
#include <fstream>
#include <iostream>

int main(int argc, char *argv[]) {
  Interpreter interpreter;
  if (argc == 1) {
    interpreter.process_stream();
    return 0;
  }
  std::string filename = argv[1];
  interpreter.process_file(filename);
  return 0;
}