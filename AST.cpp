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

void Node::print(void) {
  std::cout << this->name << std::endl;
  if (this->type == NodeType::EXPR) {
    if (this->expr.tokens.size() != 0) {
      std::cout << "Expression tokens:\n";
      for (auto &token : this->expr.tokens) {
        std::cout << (char)token.type << " ";
      }
      std::cout << std::endl;
    }
  }
  if (this->children.size() != 0) {
    std::cout << "Node children:\n";
    for (auto &child : this->children) {
      child.print();
    }
  }
}