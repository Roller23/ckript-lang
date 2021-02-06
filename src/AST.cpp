#include "AST.hpp"

#include <string>
#include <iostream>

bool Expression::is_operation() const {
  return type == BINARY_OP || type == UNARY_OP || type == FUNC_CALL || type == INDEX;
}

bool Expression::is_paren() const {
  return type == LPAREN || type == RPAREN;
}

bool Expression::is_evaluable() {
  return !is_operation() && !is_paren();
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
    std::cout << "for (";
    for (auto &expr_rpns : expressions) {
      for (auto &rpn : expr_rpns) {
        rpn.expr.print();
        std::cout << " ";
      }
      std::cout << ";";
    }
    std::cout << "):";
    for (auto &statement : statements) {
      std::cout << std::endl;
      statement.print(nest);
      std::cout << std::endl;
    }
  } else {
    if (type == RETURN) std::cout << "return";
    if (type == WHILE) std::cout << "while";
    if (type == IF) std::cout << "if";
    if (type == COMPOUND) std::cout << "compound";
    if (type == EXPR) std::cout << "expr";
    if (type == NOP) std::cout << "nop";
    if (type == DECL) std::cout << "decl";
    if (type == BREAK) std::cout << "break";
    std::cout << " ";
    for (auto &expr_rpns : expressions) {
      std::cout << "(";
      for (auto &rpn : expr_rpns) {
        rpn.expr.print();
        std::cout << " ";
      }
      std::cout << ")";
    }
    for (auto &statement : statements) {
      std::cout << std::endl;
      statement.print();
      std::cout << std::endl;
    }
    if (declaration.size() != 0) {
      declaration.at(0).print();
      std::cout << std::endl;
    }
  }
}

void Expression::print(int nest) {
  Node::print_nesting(nest);
  if (this->type == NONE) {
    std::cout << " none";
  }
  if (this->is_operation() && type != FUNC_CALL && type != INDEX) {
    std::cout << " " + Token::get_name(this->op);
  }
  if (this->type == NOP) {
    std::cout << " nop";
  }
  if (this->type == RPN) {
    std::cout << "(";
    for (auto &r : rpn_stack) {
      r.print();
    }
    std::cout << ")";
  }
  if (this->type == INDEX) {
    std::cout << "[";
    for (auto &r : index) {
      r.print();
    }
    std::cout << "]";
  }
  if (this->type == STR_EXPR) {
    std::cout << " str \"" << this->string_literal << "\"";
  }
  if (this->type == FLOAT_EXPR) {
    std::cout << " float " << this->float_literal;
  }
  if (this->type == BOOL_EXPR) {
    std::cout << " bool " << this->bool_literal;
  }
  if (this->type == NUM_EXPR) {
    if (this->is_negative) {
      std::cout << " num " << (std::int64_t)this->number_literal;
    } else {
      std::cout << " num " << this->number_literal;
    }
  }
  if (this->type == IDENTIFIER_EXPR) {
    std::cout << " id " << this->id_name;
  }
  if (this->type == FUNC_EXPR) {
    std::cout << " fn (ret " + this->func_expr.ret_type + ") params: ";
    for (auto &param : this->func_expr.params) {
      std::cout << param.param_name << "(" << param.type_name << ") ";
    }
    std::cout << ":\n";
    this->func_expr.instructions.at(0).print(nest);
  }
  if (this->type == FUNC_CALL) {
    std::cout << " call(";
    for (auto &arg_rpn : this->func_call.arguments) {
      for (auto &el : arg_rpn) {
        el.print();
        std::cout << " ";
      }
    }
    std::cout << ")";
  }
}

void Declaration::print(int nest) {
  Node::print_nesting(nest);
  if (constant) std::cout << "const ";
  if (allocated) std::cout << "allocated ";
  std::cout << id + " (" + var_type + ") = ";
  for (auto &var : var_expr) {
    var.print(nest);
  }
}

void Node::print(int nest) {
  if (type == DECL) decl.print(nest);
  if (type == EXPR) expr.print(nest);
  if (type == STMT) stmt.print(nest);
  if (children.size() == 0) return;
  print_nesting(nest);
  std::cout << "Node children(" << children.size() << "):\n";
  int i = 0;
  for (auto &child : children) {
    std::cout << "\nCHILD " << (i++) << " "; 
    child.print(nest + 1);
  }
}