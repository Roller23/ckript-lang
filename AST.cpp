#include "AST.hpp"

#include <string>
#include <iostream>

BinaryOp::BinaryOp(const Node &l, Token::TokenType o, const Node &r) {
  this->op = o;
  this->operands.push_back(l);
  this->operands.push_back(r);
}

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
  std::cout << ", statement expression:\n";
  if (stmt_expr.size() != 0) {
    if (this->stmt_expr.at(0).type == Node::NodeType::EXPR) {
      this->stmt_expr.at(0).print("", nest);
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
  if (this->type == BINARY_OP) {
    std::cout << "op(";
    this->op.operands.at(0).print("", 0);
    std::cout << " " << (char)this->op.op << " ";
    this->op.operands.at(1).print("", 0);
    std::cout << ")";
  }
  if (this->type == STR_EXPR) {
    std::cout << "str\"" << this->string_literal << "\"";
  }
  if (this->type == FLOAT_EXPR) {
    std::cout << "float" << this->float_literal;
  }
  if (this->type == NUM_EXPR) {
    std::cout << "num " << this->number_literal;
  }
  if (this->type == FUNC_EXPR) {
    std::cout << "fn (ret " + this->func_expr.ret_type + ") params: ";
    for (auto &param : this->func_expr.params) {
      std::cout << param.param_name << "(" << param.type_name << ") ";
    }
    std::cout << "fn body\n";
    this->func_expr.instructions.at(0).print("fn block", nest);
  }
  if (this->type == FUNC_CALL) {
    std::cout << "fn call [id: " + this->func_call.name + "], args:\n";
    int argc = 1;
    for (auto &arg : this->func_call.arguments) {
      arg.print("arg " + std::to_string(argc++) + " ", nest);
      std::cout << std::endl;
    }
  }
}

void Declaration::print(const std::string &_name, int nest) {
  Node::print_nesting(nest);
  std::cout << _name;
  std::cout << " [type: " << this->var_type << "]" << " [id: " << this->id << "] = \n";
  this->var_expr.at(0).print("", nest);
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