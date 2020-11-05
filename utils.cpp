#include "utils.hpp"
#include "token.hpp"

#include <cstring>
#include <iostream>

Utils::Utils(void) {
  for (int i = 0; i < sizeof(op_precedence) / sizeof(int); i++) {
    op_precedence[i] = 0;
  }
  // from lowest to highest
  op_precedence[Token::TokenType::AND_ASSIGN] = 1; // &=
  op_precedence[Token::TokenType::OR_ASSIGN] = 1; // |=
  op_precedence[Token::TokenType::XOR_ASSIGN] = 1; // ^=
  op_precedence[Token::TokenType::OP_ASSIGN] = 1; // =
  op_precedence[Token::TokenType::PLUS_ASSIGN] = 1; // +=
  op_precedence[Token::TokenType::MINUS_ASSIGN] = 1; // -=
  op_precedence[Token::TokenType::MUL_ASSIGN] = 1; // *=
  op_precedence[Token::TokenType::DIV_ASSIGN] = 1; // /=
  op_precedence[Token::TokenType::MOD_ASSIGN] = 1; // %=
  op_precedence[Token::TokenType::LSHIFT_ASSIGN] = 1; // <<=
  op_precedence[Token::TokenType::RSHIFT_ASSIGN] = 1; // >>=
  op_precedence[Token::TokenType::OP_OR] = 2; // ||
  op_precedence[Token::TokenType::OP_AND] = 3; // &&
  op_precedence[Token::TokenType::OP_OR_BIT] = 4; // |
  op_precedence[Token::TokenType::OP_XOR] = 5; // ^
  op_precedence[Token::TokenType::OP_AND] = 6; // &
  op_precedence[Token::TokenType::OP_NOT_EQ] = 7; // !=
  op_precedence[Token::TokenType::OP_EQ] = 7; // ==
  op_precedence[Token::TokenType::OP_GT] = 8; // >
  op_precedence[Token::TokenType::OP_LT] = 8; // <
  op_precedence[Token::TokenType::OP_GT_EQ] = 8; // >=
  op_precedence[Token::TokenType::OP_LT_EQ] = 8; // <=
  op_precedence[Token::TokenType::LSHIFT] = 9; // <<
  op_precedence[Token::TokenType::RSHIFT] = 9; // >>
  op_precedence[Token::TokenType::OP_PLUS] = 10; // +
  op_precedence[Token::TokenType::OP_MINUS] = 10; // -
  op_precedence[Token::TokenType::OP_MUL] = 11; // *
  op_precedence[Token::TokenType::OP_DIV] = 11; // /
  op_precedence[Token::TokenType::OP_MOD] = 11; // %
  op_precedence[Token::TokenType::DOT] = 13; // .
  op_precedence[Token::TokenType::LEFT_BRACKET] = 13; // []
}

bool Utils::op_binary(Token::TokenType token) {
  return op_precedence[(int)token] && !op_unary(token);
}

bool Utils::op_unary(Token::TokenType token) {
  return token == Token::OP_NOT || token == Token::OP_NEG || token == Token::DEL;
}

int Utils::get_precedence(Expression &e) {
  if (e.type == Expression::FUNC_CALL || e.type == Expression::INDEX) {
    return 13;
  }
  if (op_unary(e.op)) return 12;
  return op_precedence[(int)e.op];
}

bool Utils::right_assoc(Node &n) {
  auto precedence = get_precedence(n.expr);
  return precedence == 12 || precedence == 1; // REMEMBER TO CHANGE IF YOU EVER CHANGE PRECEDENCE
}