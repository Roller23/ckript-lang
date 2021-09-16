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
  const Node &AST = parser.parse(NULL);
  CVM VM;
  Evaluator evaluator(AST, VM, utils);
  evaluator.stack.reserve(100);
  // pass the "arguments" array
  auto &var = (evaluator.stack["argv"] = std::make_shared<Variable>());
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