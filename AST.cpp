#include "AST.hpp"

#include <string>

void Node::add_children(Node &node) {
  this->children.push_back(node);
}

void Node::add_children(Nodes &nodes) {
  for (auto &node : nodes) {
    this->children.push_back(node);
  }
}