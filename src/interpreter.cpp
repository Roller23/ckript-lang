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

void Interpreter::process_file(const std::string &filename, int argc, char *argv[]) {
  Lexer lexer;
  lexer.verbose = true;
  Utils utils;
  TokenList tokens = lexer.process_file(filename);
  Parser parser(tokens, Token::TokenType::NONE, "", utils);
  Node AST = parser.parse(NULL);
  std::cout << "AST:\n";
  AST.print();
  CkriptVM VM;
  VM.load_stdlib();
  Evaluator evaluator(AST, VM, utils);
  evaluator.stack.reserve(100);
  // pass the "arguments" array
  Variable *var = new Variable;
  var->id = "arguments";
  var->type = Utils::VarType::ARR;
  var->val.array_type = "str";
  var->val.type = Utils::VarType::ARR;
  var->val.array_values.reserve(argc);
  for (int i = 0; i < argc; i++) {
    Value element;
    element.type = Utils::VarType::STR;
    element.string_value = argv[i];
    var->val.array_values.push_back(element);
  }
  evaluator.stack.push_back(var);
  evaluator.start();
}

void Interpreter::process_stream() {
  Utils utils;
  CkriptVM VM;
  VM.load_stdlib();
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