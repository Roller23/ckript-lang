#include "interpreter.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "ckript-vm.hpp"

#include <string>
#include <iostream>

void Interpreter::process_file(const std::string &filename) {
  Lexer lexer;
  Parser parser;
  CkriptVM VM;
  lexer.verbose = true;
  TokenList tokens = lexer.process_file(filename);
  if (lexer.last_error) {
    if (lexer.last_error == Lexer::FILE_ERROR) {
      std::cout << "Couldn't open " + filename + "\n";
    }
    return;
  }
  parser.parse(tokens);
}