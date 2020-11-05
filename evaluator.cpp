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
  RpnStack res_stack;
  for (auto &token : rpn_stack) {
    if (token.type == RpnElement::OPERATOR) {
      if (token.op.type == Operator::BASIC) {
        if (token.op.type == Token::OP_PLUS) {
          RpnElement y = res_stack.back();
          res_stack.pop_back();
          RpnElement x = res_stack.back();
          res_stack.pop_back();
          RpnElement result = perform_addition(x, y);
          res_stack.push_back(result);
        }
      } else if (token.op.type == Operator::FUNC) {
        // to do
      } else if (token.op.type == Operator::INDEX) {
        // to do
      }
    } else {
      res_stack.push_back(token);
    }
  }
  return {};
}

void Evaluator::declare_variable(Node &declaration) {

}

RpnElement Evaluator::node_to_element(Node &node) {
  assert(node.expr.is_paren() == false);
  if (node.expr.is_operation()) {
    if (node.expr.type == Expression::FUNC_CALL) {
      return {Operator(node.expr.func_call)};
    }
    if (node.expr.type == Expression::INDEX) {
      return {Operator(node.expr.index)};
    }
    return {Operator(node.expr.op)};
  }
  Value val;
  if (node.expr.type == Expression::BOOL_EXPR) {
    val.type = Value::BOOLEAN;
    val.boolean_value = node.expr.bool_literal;
    return val;
  }
  if (node.expr.type == Expression::STR_EXPR) {
    val.type = Value::STRING;
    val.string_value = node.expr.string_literal;
    return val;
  }
  if (node.expr.type == Expression::FLOAT_EXPR) {
    val.type = Value::FLOAT;
    val.float_value = node.expr.float_literal;
    return val;
  }
  if (node.expr.type == Expression::NUM_EXPR) {
    val.type = Value::NUMBER;
    val.number_value = node.expr.number_literal;
    return val;
  }
  if (node.expr.type == Expression::IDENTIFIER_EXPR) {
    val.type = Value::REFERENCE;
    val.reference = get_reference_by_name(node.expr.id_name); // will break
    return val;
  }
  throw "\n\nUndentified expression type!\n\n";
}

Variable *Evaluator::get_reference_by_name(const std::string &name) {
  return nullptr;
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