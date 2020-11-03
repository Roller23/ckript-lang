#include "evaluator.hpp"

#include <iostream>

typedef Statement::StmtType StmtType;

void Evaluator::start() {
  std::cout << "\n------ START -------\n";
  for (auto &statement : AST.children) {
    execute_statement(statement);
  }
}

void Evaluator::execute_statement(Node &statement) {
  if (statement.stmt.type == StmtType::NONE) return;
  std::cout << "Executing a statement\n";
  // // statement.stmt.print();
  // std::cout << "TYPE " << statement.stmt.type << "\n";
  // for (auto &nested_stmt : statement.stmt.statements) {
  //   execute_statement(nested_stmt);
  //   for (auto &child : nested_stmt.children) {
  //     execute_statement(child);
  //   }
  // }
  if (statement.stmt.type == StmtType::EXPR) {
    evaluate_expression(statement);
  }
}

void Evaluator::evaluate_expression(Node &expression) {
  std::cout << "Evaluating an expression\n";
}

void Evaluator::declare_variable(Node &declaration) {

}