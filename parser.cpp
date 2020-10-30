#include "parser.hpp"
#include "AST.hpp"

#include <vector>
#include <iostream>

typedef Expression::ExprType ExprType;
typedef Declaration::DeclType DeclType;
typedef Statement::StmtType StmtType;
typedef Node::NodeType NodeType;

void Parser::move(void) {
  pos++;
  prev = current;
  if (pos < tokens_count) {
    current = tokens.at(pos);
  } else {
    current = Token(Token::NONE, "");
    pos--;
  }
}

void Parser::parse(void) {
  while (this->current.type != Token::NONE) {
    
    this->move();
  }
}