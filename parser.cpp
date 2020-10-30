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

std::vector<Statement> Parser::get_multiple_statements(Token::TokenType stop) {
  std::vector<Statement> res;
  advance();
  while (curr_token.type != stop) {
    res.push_back(get_statement());
  }
  return res;
}

Statement Parser::get_statement(void) {
  if (curr_token.type == Token::LEFT_BRACE) {
    // { statement(s) }
    Statement stmt(StmtType::COMPOUND);
    std::vector<Statement> inner_statements = get_multiple_statements(Token::RIGHT_BRACE);
    // stmt.add_children(inner_statements);
  } else if (curr_token.type == Token::IF) {
    // if (expression) statement
  } else if (curr_token.type == Token::WHILE) {
    // while (expression) statement
  } else if (curr_token.type == Token::FOR) {
    // for (expression; expression; expression) statement
  } else if (curr_token.type == Token::RETURN) {
    // return expression;
  } else if (curr_token.type == Token::TYPE) {
    // type identifier = expression;
  } else if (curr_token.type == Token::SEMI_COLON) {
    // nop;
  } else {
    // expression;
  }
  return {StmtType::UNKNOWN};
}

void Parser::parse(void) {
  while (curr_token.type != Token::NONE) {
    
    advance(); // advance to the next token
  }
}