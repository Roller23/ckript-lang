#include <iostream>
#include "src/interpreter.hpp"

int main(int argc, char *argv[]) {
  if (argc == 1) {
    std::cout << "No input files\n";
    return 1;
  }
  Interpreter().process_file(argv[1], argc - 1, argv + 1);
  return 0;
}