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
      std::cout << (char)token.type << " ";
    }
  }
  if (this->stmt_exprs.size() > 0) {
    std::cout << ", expressions: ";
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
      std::cout << (char)token.type << " ";
    }
  }
  std::cout << std::endl;
}

void Declaration::print(const std::string &_name, int nest) {
  Node::print_nesting(nest);
  std::cout << _name << std::endl;
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