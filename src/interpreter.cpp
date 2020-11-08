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

void Interpreter::process_stream() {
  Lexer lexer;
  lexer.verbose = false;
  Utils utils;
  CkriptVM VM;
  std::string line = "";
  while (true) {
    std::cout << "> ";
    std::getline(std::cin, line);
    if (line.size() == 0) continue;
    if (line == "exit") break;
    TokenList tokens = lexer.tokenize(line);
    Parser parser(tokens, Token::TokenType::NONE, "", utils);
    Node AST = parser.parse(NULL);
    Evaluator evaluator(AST, VM, utils);
    evaluator.start();
  }
}