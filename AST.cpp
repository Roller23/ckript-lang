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

void Statement::print(int nest) {
  Node::print_nesting(nest);
  if (type == FOR) {
    std::cout << "for ( ";
    for (auto &expr : this->expressions) {
      expr.print();
      std::cout << ";";
    }
    std::cout << "):";
  } else {
    if (type == RETURN) std::cout << "return";
    if (type == WHILE) std::cout << "while";
    if (type == IF) std::cout << "if";
    if (type == COMPOUND) std::cout << "compound";
    if (type == EXPR) std::cout << "expression";
    if (type == NOP) std::cout << "nop";
    if (type == DECL) std::cout << "declaration";
    std::cout << " ";
    if (expressions.size() != 0) {
      for (auto &expr : expressions) {
        expr.print(nest);
      }
    }
    if (statements.size() != 0) {
      statements.at(0).print(nest + 1);
    }
    if (type != EXPR) std::cout << std::endl;
  }
}

void Expression::print(int nest) {
  Node::print_nesting(nest);
  if (this->type == BINARY_OP) {
    std::cout << "(";
    this->op.operands.at(0).expr.print();
    std::cout << " " << Token::get_name(op.op) << " ";
    this->op.operands.at(1).expr.print();
    std::cout << ")";
  }
  if (this->type == NOP) {
    std::cout << "nop";
  }
  if (this->type == STR_EXPR) {
    std::cout << "str \"" << this->string_literal << "\"";
  }
  if (this->type == FLOAT_EXPR) {
    std::cout << "float " << this->float_literal;
  }
  if (this->type == BOOL_EXPR) {
    std::cout << "bool " << this->bool_literal;
  }
  if (this->type == NUM_EXPR) {
    std::cout << "num " << this->number_literal;
  }
  if (this->type == IDENTIFIER_EXPR) {
    std::cout << "id " << this->id_name;
  }
  if (this->type == FUNC_EXPR) {
    std::cout << "fn (ret " + this->func_expr.ret_type + ") params: ";
    for (auto &param : this->func_expr.params) {
      std::cout << param.param_name << "(" << param.type_name << ") ";
    }
    std::cout << ":\n";
    this->func_expr.instructions.at(0).print(nest);
  }
  if (this->type == FUNC_CALL) {
    std::cout << "call " + this->func_call.name + ", args: ";
    for (auto &arg : this->func_call.arguments) {
      arg.print();
      std::cout << " ";
    }
  }
}

void Declaration::print(int nest) {
  Node::print_nesting(nest);
  if (constant) std::cout << "const ";
  if (allocated) std::cout << "allocated ";
  std::cout << "decl " + id + " (" + var_type + ") = ";
  var_expr.at(0).expr.print();
  std::cout << std::endl;
}

void Node::print(int nest) {
  if (type == DECL) decl.print(nest);
  if (type == EXPR) {
    expr.print(nest);
    std::cout << std::endl;
  };
  if (type == STMT) stmt.print(nest);
  if (children.size() == 0) return;
  print_nesting(nest);
  std::cout << "Node children(" << children.size() << "):\n";
  print_nesting(nest);
  for (auto &child : children) {
    child.print(nest + 1);
  }
}