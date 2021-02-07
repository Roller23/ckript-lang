#include "interpreter.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "CVM.hpp"
#include "token.hpp"
#include "evaluator.hpp"
#include "utils.hpp"

#include <string>
#include <iostream>
#include <memory>

void Interpreter::process_file(const std::string &filename, int argc, char *argv[]) {
  Lexer lexer;
  Utils utils;
  TokenList tokens = lexer.process_file(filename);
  Parser parser(tokens, Token::TokenType::NONE, "", utils);
  Node AST = parser.parse(NULL);
  CVM VM;
  Evaluator evaluator(AST, VM, utils);
  evaluator.stack.reserve(100);
  // pass the "arguments" array
  evaluator.stack["argv"] = std::make_shared<Variable>();
  auto &var = evaluator.stack["argv"];
  var->type = Utils::ARR;
  var->val.array_type = "str";
  var->val.type = Utils::ARR;
  var->val.array_values.resize(argc);
  for (int i = 0; i < argc; i++) {
    var->val.array_values[i].type = Utils::STR;
    var->val.array_values[i].string_value = argv[i];
  }
  evaluator.start();
}

void Interpreter::process_stream() {
  Utils utils;
  CVM VM;
  std::string line = "";
  bool running = true;
  std::cout << "Ckript shell\n";
  std::cout << "Made by https://github.com/Roller23\n";
  std::cout << "Type 'exit' to exit\n";
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
      // evaluator.AST = AST;
      evaluator.start();
    }
  }
}