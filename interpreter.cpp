#include "interpreter.hpp"

#include "lexer.hpp"
#include <string>
#include <fstream>
#include <iostream>

void Interpreter::process_file(const std::string &filename) {
  Lexer main_lexer;
  main_lexer.verbose = true;
  main_lexer.process_file(filename);
  if (main_lexer.last_error) {
    if (main_lexer.last_error == Lexer::FILE_ERROR) {
      std::cout << "Couldn't open " + filename + "\n";
    }
    return;
  }
}