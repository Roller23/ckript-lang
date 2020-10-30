#include "parser.hpp"
#include "AST.hpp"

#include <vector>

typedef Expression::ExprType ExprType;
typedef Declaration::DeclType DeclType;
typedef Statement::StmtType StmtType;

void Parser::parse(TokenList &tokens) {
  Nodes AST;
  Node start();
  for (auto token : tokens) {
    if (token.type == Token::TYPE) {

    }
    parse_expression(AST, -1, 0);
  }
}

Expression Parser::parse_expression(const Nodes &tree, int prev, int current) {
  if (tree.at(current).type == Token::SEMI_COLON) {
    return {ExprType::BINARY_OP}; // xd
  }
}