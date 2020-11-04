#include "evaluator.hpp"

#include <iostream>
#include <cassert>

typedef Statement::StmtType StmtType;

bool Value::is_lvalue() {
  return reference != nullptr;
}

void Evaluator::start() {
  std::cout << "\n------ START -------\n";
  for (auto &statement : AST.children) {
    execute_statement(statement);
  }
}

void Evaluator::execute_statement(Node &statement) {
  if (statement.stmt.type == StmtType::NONE) return;
  std::cout << "Executing a statement\n";
  std::cout << "Type " << statement.stmt.type << "\n";
  // // statement.stmt.print();
  // std::cout << "TYPE " << statement.stmt.type << "\n";
  // for (auto &nested_stmt : statement.stmt.statements) {
  //   execute_statement(nested_stmt);
  //   for (auto &child : nested_stmt.children) {
  //     execute_statement(child);
  //   }
  // }
  if (statement.stmt.type == StmtType::EXPR) {
    if (statement.stmt.expressions.size() == 0) return;
    evaluate_expression(statement.stmt.expressions.at(0));
    return;
  }
}

Value Evaluator::evaluate_expression(NodeList &expression_tree) {
  std::cout << "Evaluating an expression\n";
  RpnStack rpn_stack;
  flatten_tree(rpn_stack, expression_tree);
  return {};
}

void Evaluator::declare_variable(Node &declaration) {

}

RpnElement Evaluator::node_to_element(Node &node) {
  assert(node.expr.is_paren() == false);
  if (node.expr.is_operation()) {
    return {Operator(node.expr.op)};
  }
  return {};
}

void Evaluator::flatten_tree(RpnStack &res, NodeList &expression_tree) {
  for (auto &node : expression_tree) {
    if (node.expr.rpn_stack.size() != 0) {
      flatten_tree(res, node.expr.rpn_stack);
      node.expr.rpn_stack.clear();
    }
    res.push_back(node_to_element(node));
  }
  expression_tree.clear();
}