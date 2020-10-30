#include "interpreter.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "ckript-vm.hpp"

#include <string>
#include <iostream>
#include <cstdlib>

void Interpreter::throw_error(const std::string &cause, bool terminate) {
  std::cout << cause << std::endl;
  if (terminate) {
    std::exit(EXIT_FAILURE);
  }
}

void Interpreter::process_file(const std::string &filename) {
  Lexer lexer;
  Parser parser;
  CkriptVM VM;
  lexer.verbose = true;
  TokenList tokens = lexer.process_file(filename);
  if (lexer.last_error != "") {
    throw_error(lexer.last_error, false);
    return;
  }
  parser.parse(tokens);
}