#include "src/interpreter.hpp"

int main(int argc, char *argv[]) {
  Interpreter interpreter;
  if (argc == 1) {
    interpreter.process_stream();
    return 0;
  }
  interpreter.process_file(argv[1], argc - 1, argv + 1);
  return 0;
}