#include "interpreter.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "ckript-vm.hpp"
#include "token.hpp"
#include "evaluator.hpp"
#include "utils.hpp"

#include <string>
#include <iostream>
#include <cstdlib>

void Interpreter::process_file(const std::string &filename) {
  Lexer lexer;
  lexer.verbose = true;
  Utils utils;
  TokenList tokens = lexer.process_file(filename);
  Parser parser(tokens, Token::TokenType::NONE, "MAIN", utils);
  Node AST = parser.parse(NULL);
  std::cout << "AST:\n";
  AST.print();
  CkriptVM VM;
  Evaluator evaluator(AST, VM, utils);
  evaluator.start();
}