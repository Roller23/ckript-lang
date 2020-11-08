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
  Utils utils;
  CkriptVM VM;
  std::string line = "";
  bool running = true;
  while (running) {
    std::cout << "> ";
    std::getline(std::cin, line);
    if (line.size() == 0) continue;
    if (line == "exit") break;
    TokenList tokens = Lexer().tokenize(line);
    Node AST = Parser(tokens, Token::TokenType::NONE, "", utils).parse(NULL);
    Evaluator evaluator = Evaluator(AST, VM, utils, true);
    evaluator.start();
    while (true) {
      std::cout << "> ";
      std::getline(std::cin, line);
      if (line.size() == 0) continue;
      if (line == "exit") {
        running = false;
        break;
      }
      TokenList tokens = Lexer().tokenize(line);
      Node AST = Parser(tokens, Token::TokenType::NONE, "", utils).parse(NULL);
      evaluator.AST = AST;
      evaluator.start();
    }
  }
}