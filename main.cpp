#include "interpreter.hpp"
#include <string>
#include <fstream>
#include <iostream>

int main(int argc, char *argv[]) {
  // if (argc == 1) {
  //   // to do - create a shell
  //   return 0;
  // }
  Interpreter interpreter;
  // std::string filename = argv[1];
  interpreter.process_file("test.ck");
  return 0;
}