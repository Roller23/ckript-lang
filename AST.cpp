#include "AST.hpp"

#include <string>
#include <iostream>

void Node::add_children(Node &node) {
  this->children.push_back(node);
}

void Node::add_children(NodeList &nodes) {
  for (auto &node : nodes) {
    this->children.push_back(node);
  }
}

void Node::print_nesting(int nest) {
  for (int i = 0; i < nest; i++) {
    std::cout << "  " << std::flush;
  }
}

void Statement::print(const std::string &_name, int nest) {
  Node::print_nesting(nest);
  std::cout << _name;
  if (this->stmt_expr.type != Expression::ExprType::NONE && this->stmt_expr.tokens.size() > 0) {
    std::cout << ", statement expression: ";
    for (auto &token : this->stmt_expr.tokens) {
      std::cout << token << " ";
    }
  }
  if (this->stmt_exprs.size() > 0) {
    std::cout << ", expressions:\n";
    for (auto &expr : this->stmt_exprs) {
      expr.print("", nest);
    }
  }
  std::cout << std::endl;
}

void Expression::print(const std::string &_name, int nest) {
  Node::print_nesting(nest);
  std::cout << _name;
  if (this->tokens.size() != 0) {
    std::cout << ", tokens: ";
    for (auto &token : this->tokens) {
      std::cout << token << " ";
    }
  }
  if (this->func_expr.type != FuncExpression::FuncType::NONE) {
    std::cout << "fn (ret " + this->func_expr.ret_type + ") params: ";
    for (auto &param : this->func_expr.params) {
      std::cout << param.param_name << "(" << param.type_name << ") ";
    }
    std::cout << "fn body\n";
    (*this->func_expr.instructions).print("fn block", nest);
  }
  if (this->type == Expression::ExprType::FUNC_CALL) {
    std::cout << "fn call [id: " + this->func_call.name + "], args:\n";
    for (auto &arg : this->func_call.arguments) {
      arg.print("");
    }
  }
  std::cout << std::endl;
}

void Declaration::print(const std::string &_name, int nest) {
  Node::print_nesting(nest);
  std::cout << _name;
  std::cout << " [type: " << this->var_type << "]" << " [id: " << this->id << "] = \n";
  this->var_expr->print("", nest);
}

void Node::print(const std::string &_name, int nest) {
  if (this->type == NodeType::DECL) this->decl.print(_name, nest);
  if (this->type == NodeType::EXPR) this->expr.print(_name, nest);
  if (this->type == NodeType::STMT) this->stmt.print(_name, nest);
  if (this->children.size() != 0) {
    print_nesting(nest);
    std::cout << "Node children:\n";
    for (auto &child : this->children) {
      child.print(child.name, nest + 1);
    }
  }
}