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
    std::cout << "Reached EOF\n";
    curr_token = Token(Token::NONE);
    pos--;
  }
}

void Parser::retreat(void) {
  if (pos > 0) {
    pos--;
    curr_token = tokens.at(pos);
    if (pos > 0) {
      prev = tokens.at(pos - 1);
    }
  }
}

Node Parser::get_expression(Node &prev, Token::TokenType stop) {
  std::cout << "consuming expression\n";
  Node res(Expression(ExprType::BOOL_EXPR), "Expression");
  while (curr_token.type != stop) {
    res.expr.tokens.push_back(curr_token);
    advance();
  }
  return res;
}

NodeList Parser::get_multiple_statements(Node &prev, Token::TokenType stop) {
  NodeList res;
  std::cout << "consuming statements in a compound statement\n";
  while (true) {
    Node stmt = get_statement(prev, stop);
    if (curr_token.type != stop) {
      res.push_back(stmt); 
    } else {
      break;
    }
  }
  advance(); // skip stop
  return res;
}

Node Parser::get_statement(Node &prev, Token::TokenType stop) {
  // std::cout << "get statement at " << (char)curr_token.type << std::endl;
  if (curr_token.type == stop) {
    // std::cout << "reached stop " << (char)stop << std::endl;
    return prev;
  }
  if (curr_token.type == Token::LEFT_BRACE) {
    std::cout << "Found {\n";
    // { statement(s) }
    Node stmt(Statement(StmtType::COMPOUND), "Compund");
    advance(); // skip the {
    NodeList inner_statements = get_multiple_statements(stmt, Token::RIGHT_BRACE);
    stmt.add_children(inner_statements);
    std::cout << "compound count " << inner_statements.size() << "\n";
    prev.add_children(stmt);
    return get_statement(prev, stop);
  } else if (curr_token.type == Token::IF) {
    std::cout << "Found if\n";
    // if (expression) statement
    advance(); // skip the if
    if (curr_token.type != Token::LEFT_PAREN) {
      // invalid if statement
      throw;
    }
    Node if_stmt = Node(Statement(StmtType::IF), "IF");
    advance(); // skip the (
    Node expr = get_expression(prev, Token::TokenType::RIGHT_PAREN);
    if_stmt.stmt.stmt_expr = expr.expr;
    advance(); // skip the )
    prev.add_children(if_stmt);
    return get_statement(prev, stop);
  } else if (curr_token.type == Token::WHILE) {
    std::cout << "Found while\n";
    // while (expression) statement
  } else if (curr_token.type == Token::FOR) {
    std::cout << "Found for\n";
    // for (expression; expression; expression) statement
  } else if (curr_token.type == Token::RETURN) {
    std::cout << "Found return\n";
    // return expression;
    advance(); // skip the return
    Node return_stmt(Statement(StmtType::RETURN), "RETURN");
    Node return_expr = get_expression(prev, Token::TokenType::SEMI_COLON);
    return_stmt.stmt.stmt_expr = return_expr.expr;
    prev.add_children(return_stmt);
    advance(); // skip the semicolon
    return get_statement(prev, stop);
  } else if (curr_token.type == Token::TYPE) {
    std::cout << "Found type\n";
    // type identifier = expression;
  } else if (curr_token.type == Token::SEMI_COLON) {
    std::cout << "no operation\n";
    // nop;
    advance(); // skip the semicolon
    return prev;
  } else if (curr_token.type == Token::NONE) {
    std::cout << "EOF\n";
    // end of file
    return prev;
  } else {
    std::cout << "found an expression\n";
    // expression;
    Node expr = get_expression(prev, Token::TokenType::SEMI_COLON);
    prev.add_children(expr);
    advance(); // skip the semicolon
    return get_statement(prev, stop);
  }
  std::cout << "Nothing matched\n";
  return prev;
}

void Parser::parse(void) {
  Node start(Statement(StmtType::COMPOUND), "Compound (PROGRAM START)");
  Node program = get_statement(start, Token::TokenType::NONE);
  std::cout << "AST:\n";
  program.print(program.name);
}