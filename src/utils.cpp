#include "utils.hpp"
#include "token.hpp"

#include <cstring>
#include <iostream>

#define REG(op, val) op_precedence[Token::TokenType::op] = val

Utils::Utils(void) {
  op_precedence.reserve(31);
  // from lowest to highest
  REG(AND_ASSIGN, 1); // &=
  REG(OR_ASSIGN, 1); // |=
  REG(XOR_ASSIGN, 1); // ^=
  REG(OP_ASSIGN, 1); // =
  REG(PLUS_ASSIGN, 1); // +=
  REG(MINUS_ASSIGN, 1); // -=
  REG(MUL_ASSIGN, 1); // *=
  REG(DIV_ASSIGN, 1); // /=
  REG(MOD_ASSIGN, 1); // %=
  REG(LSHIFT_ASSIGN, 1); // <<=
  REG(RSHIFT_ASSIGN , 1); // >>=
  REG(OP_OR, 2); // ||
  REG(OP_AND, 3); // &&
  REG(OP_OR_BIT, 4); // |
  REG(OP_XOR, 5); // ^
  REG(OP_AND_BIT, 6); // &
  REG(OP_NOT_EQ, 7); // !=
  REG(OP_EQ, 7); // ==
  REG(OP_GT, 8); // >
  REG(OP_LT, 8); // <
  REG(OP_GT_EQ, 8); // >=
  REG(OP_LT_EQ, 8); // <=
  REG(LSHIFT, 9); // <<
  REG(RSHIFT, 9); // >>
  REG(OP_PLUS, 10); // +
  REG(OP_MINUS, 10); // -
  REG(OP_MUL, 11); // *
  REG(OP_DIV, 11); // /
  REG(OP_MOD, 11); // %
  REG(DOT, 13); // .
  REG(LEFT_BRACKET, 13); // []
  var_lut.reserve(9);
  var_lut["double"] = FLOAT;
  var_lut["int"] = INT;
  var_lut["str"] = STR;
  var_lut["arr"] = ARR;
  var_lut["obj"] = OBJ;
  var_lut["bool"] = BOOL;
  var_lut["func"] = FUNC;
  var_lut["class"] = CLASS;
  var_lut["void"] = VOID;
}

bool Utils::has_key(Token::TokenType key) {
  return op_precedence.find(key) != op_precedence.end();
}

bool Utils::op_binary(Token::TokenType token) {
  return has_key(token) && !op_unary(token);
}

bool Utils::op_unary(Token::TokenType token) {
  return token == Token::OP_NOT || token == Token::OP_NEG || token == Token::DEL;
}

int Utils::get_precedence(Expression &e) {
  if (e.type == Expression::FUNC_CALL || e.type == Expression::INDEX) {
    return 13;
  }
  if (op_unary(e.op)) return 12;
  return op_precedence[e.op];
}

bool Utils::right_assoc(Node &n) {
  auto precedence = get_precedence(n.expr);
  return precedence == 12 || precedence == 1; // REMEMBER TO CHANGE IF YOU EVER CHANGE PRECEDENCE
}