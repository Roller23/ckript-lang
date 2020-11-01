#include "interpreter.hpp"
#include <string>
#include <fstream>
#include <iostream>

int main(int argc, char *argv[]) {
  // if (argc == 1) {
  //   // to do - create a shell
  //   return 0;
  // }
  Interpreter python;
  // std::string filename = argv[1];
  python.process_file("test.ck");
  std::cout << std::endl;
  return 0;
}