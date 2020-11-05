#include "evaluator.hpp"
#include "utils.hpp"
#include "error-handler.hpp"

#include <iostream>
#include <cassert>

typedef Statement::StmtType StmtType;
typedef Utils::VarType VarType;

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
    if (statement.stmt.expressions.size() != 1) return;
    Value result = evaluate_expression(statement.stmt.expressions.at(0));
    return;
  }
  if (statement.stmt.type == StmtType::DECL) {
    if (statement.stmt.declaration.size() != 1) return;
    declare_variable(statement.stmt.declaration.at(0));
  }
}

Value Evaluator::evaluate_expression(NodeList &expression_tree) {
  std::cout << "Evaluating an expression\n";
  RpnStack rpn_stack;
  flatten_tree(rpn_stack, expression_tree);
  RpnStack res_stack;
  for (auto &token : rpn_stack) {
    if (token.type == RpnElement::OPERATOR) {
      if (token.op.op_type == Operator::BASIC) {
        if (utils.op_binary(token.op.type)) {
          if (res_stack.size() < 2) {
            ErrorHandler::throw_syntax_error("Cannot perform this operation\n");
          }
          RpnElement y = res_stack.back();
          res_stack.pop_back();
          RpnElement x = res_stack.back();
          res_stack.pop_back();
          RpnElement result;
          if (token.op.type == Token::OP_PLUS) {
            result = perform_addition(x, y);
          } else if (token.op.type == Token::OP_MINUS) {
            result = perform_subtraction(x, y);
          } else if (token.op.type == Token::OP_MUL) {
            result = perform_multiplication(x, y);
          } else if (token.op.type == Token::OP_DIV) {
            result = perform_division(x, y);
          }
          res_stack.push_back(result);
        } else if (utils.op_unary(token.op.type)) {

        }
      } else if (token.op.op_type == Operator::FUNC) {
        // to do
      } else if (token.op.op_type == Operator::INDEX) {
        // to do
      }
    } else {
      res_stack.push_back(token);
    }
  }
  std::cout << "Expression result = " << stringify(res_stack.at(0).value) << "\n";
  return {};
}

std::string Evaluator::stringify(Value &val) {
  if (val.type == VarType::STR) {
    return val.string_value;
  }
  if (val.type == VarType::BOOL) {
    return val.boolean_value ? "true" : "false";
  }
  if (val.type == VarType::FLOAT) {
    return std::to_string(val.float_value);
  }
  if (val.type == VarType::INT) {
    return std::to_string(val.number_value);
  }
  if (val.type == VarType::REF) {
    // to do
  }
  return "";
}

double Evaluator::to_double(Value &val) {
  if (val.type == VarType::FLOAT) {
    return val.float_value;
  }
  if (val.type == VarType::INT) {
    return (double)val.number_value;
  }
  ErrorHandler::throw_runtime_error("Cannot convert " + stringify(val) + " to double");
  return 0;
}

RpnElement Evaluator::perform_addition(RpnElement &x, RpnElement &y) {
  Value val;
  if (x.value.type == VarType::STR || y.value.type == VarType::STR) {
    std::string str1 = stringify(x.value);
    std::string str2 = stringify(y.value);
    std::cout << "concating " << str1 << " to " << str2 << "\n";
    val.type = VarType::STR;
    val.string_value = str1 + str2;
    std::cout << "result = " + val.string_value << "\n";
    return {val};
  }
  if (x.value.type == VarType::INT && y.value.type == VarType::INT) {
    std::cout << "adding " << x.value.number_value << " to " << y.value.number_value << "\n";
    val.type = VarType::INT;
    val.number_value = x.value.number_value + y.value.number_value;
    std::cout << "result = " << val.number_value << "\n";
    return {val};
  }
  if (x.value.type == VarType::FLOAT || y.value.type == VarType::FLOAT) {
    double f1 = to_double(x.value);
    double f2 = to_double(y.value);
    val.type = VarType::FLOAT;
    val.float_value = f1 + f2;
    std::cout << "result = " << val.float_value << "\n";
    return {val};
  }
  std::string msg = "Cannot perform addition on " + stringify(x.value) + " and " + stringify(y.value);
  ErrorHandler::throw_runtime_error(msg);
  return {};
}

RpnElement Evaluator::perform_subtraction(RpnElement &x, RpnElement &y) {
  Value val;
  if (x.value.type == VarType::INT && y.value.type == VarType::INT) {
    std::cout << "subtracting " << x.value.number_value << " from " << y.value.number_value << "\n";
    val.type = VarType::INT;
    val.number_value = x.value.number_value - y.value.number_value;
    std::cout << "result = " << val.number_value << "\n";
    return {val};
  }
  if (x.value.type == VarType::FLOAT || y.value.type == VarType::FLOAT) {
    double f1 = to_double(x.value);
    double f2 = to_double(y.value);
    std::cout << "subtracting " << f1 << " from " << f2 << "\n";
    val.type = VarType::FLOAT;
    val.float_value = f1 - f2;
    std::cout << "result = " << val.float_value << "\n";
    return {val};
  }
  std::string msg = "Cannot perform subtraction on " + stringify(x.value) + " and " + stringify(y.value);
  ErrorHandler::throw_runtime_error(msg);
  return {};
}

RpnElement Evaluator::perform_multiplication(RpnElement &x, RpnElement &y) {
  Value val;
  if (x.value.type == VarType::INT && y.value.type == VarType::INT) {
    std::cout << "multiplicating " << x.value.number_value << " by " << y.value.number_value << "\n";
    val.type = VarType::INT;
    val.number_value = x.value.number_value * y.value.number_value;
    std::cout << "result = " << val.number_value << "\n";
    return {val};
  }
  if (x.value.type == VarType::FLOAT || y.value.type == VarType::FLOAT) {
    double f1 = to_double(x.value);
    double f2 = to_double(y.value);
    std::cout << "multiplicating " << f1 << " by " << f2 << "\n";
    val.type = VarType::FLOAT;
    val.float_value = f1 * f2;
    std::cout << "result = " << val.float_value << "\n";
    return {val};
  }
  std::string msg = "Cannot perform multiplication on " + stringify(x.value) + " and " + stringify(y.value);
  ErrorHandler::throw_runtime_error(msg);
  return {};
}

RpnElement Evaluator::perform_division(RpnElement &x, RpnElement &y) {
  Value val;
  if (x.value.type == VarType::INT && y.value.type == VarType::INT) {
    if (y.value.number_value == 0) {
      ErrorHandler::throw_runtime_error("Cannot divide by zero");
    }
    std::cout << "dividing " << x.value.number_value << " by " << y.value.number_value << "\n";
    val.type = VarType::INT;
    val.number_value = x.value.number_value / y.value.number_value;
    std::cout << "result = " << val.number_value << "\n";
    return {val};
  }
  if (x.value.type == VarType::FLOAT || y.value.type == VarType::FLOAT) {
    double f1 = to_double(x.value);
    double f2 = to_double(y.value);
    if (f2 == 0.0f) {
      ErrorHandler::throw_runtime_error("Cannot divide by zero");
    }
    std::cout << "dividing " << f1 << " by " << f2 << "\n";
    val.type = VarType::FLOAT;
    val.float_value = f1 / f2;
    std::cout << "result = " << val.float_value << "\n";
    return {val};
  }
  std::string msg = "Cannot perform subtraction on " + stringify(x.value) + " and " + stringify(y.value);
  ErrorHandler::throw_runtime_error(msg);
  return {};
}

void Evaluator::declare_variable(Node &declaration) {
  Declaration &decl = declaration.decl;
  std::cout << "Declaring " + decl.var_type + " " + decl.id + "\n";
  Value var_val = evaluate_expression(decl.var_expr);
  Utils::VarType var_type = utils.var_lut.at(decl.var_type);
  if (var_type != var_val.type);
  std::cout << "Assigning " + stringify(var_val) + " to " + decl.id + "\n";
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
    val.type = VarType::BOOL;
    val.boolean_value = node.expr.bool_literal;
    return {val};
  }
  if (node.expr.type == Expression::STR_EXPR) {
    val.type = VarType::STR;
    val.string_value = node.expr.string_literal;
    return {val};
  }
  if (node.expr.type == Expression::FLOAT_EXPR) {
    val.type = VarType::FLOAT;
    val.float_value = node.expr.float_literal;
    return {val};
  }
  if (node.expr.type == Expression::NUM_EXPR) {
    val.type = VarType::INT;
    val.number_value = node.expr.number_literal;
    val.is_neg = node.expr.is_negative;
    return {val};
  }
  if (node.expr.type == Expression::IDENTIFIER_EXPR) {
    val.type = VarType::REF;
    val.reference = get_reference_by_name(node.expr.id_name); // will break
    return {val};
  }
  ErrorHandler::throw_runtime_error("Undentified expression type!\n");
  return {};
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
    if (node.expr.type != Expression::RPN) {
      res.push_back(node_to_element(node));
    }
  }
  expression_tree.clear();
}