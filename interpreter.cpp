#include "interpreter.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "ckript-vm.hpp"
#include "token.hpp"

#include <string>
#include <iostream>
#include <cstdlib>

void Interpreter::process_file(const std::string &filename) {
  Lexer lexer;
  CkriptVM VM;
  lexer.verbose = true;
  TokenList tokens = lexer.process_file(filename);
  Parser parser(tokens, Token::TokenType::NONE);
  Node AST = parser.parse(NULL);
  std::cout << "AST:\n";
  AST.print();
}