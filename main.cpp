#include "src/interpreter.hpp"
#include <string>
#include <fstream>
#include <iostream>

int main(int argc, char *argv[]) {
  // if (argc == 1) {
  //   // TODO - create a shell
  //   return 0;
  // }
  Interpreter interpreter;
  std::string filename = argv[1];
  interpreter.process_file(filename);
  std::cout << std::endl;
  return 0;
}