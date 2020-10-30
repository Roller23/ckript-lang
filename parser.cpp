#include "parser.hpp"
#include "AST.hpp"

#include <vector>
#include <iostream>

typedef Expression::ExprType ExprType;
typedef Declaration::DeclType DeclType;
typedef Statement::StmtType StmtType;
typedef Node::NodeType NodeType;

void Parser::advance(void) {
  pos++;
  prev = curr_token;
  if (pos < tokens_count) {
    curr_token = tokens.at(pos);
  } else {
    curr_token = Token(Token::NONE);
    pos--;
  }
}

void Parser::parse(void) {
  while (curr_token.type != Token::NONE) {
    
    advance(); // advance to the next token
  }
}