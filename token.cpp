#include "token.hpp"

std::string Token::get_name(TokenType type) {
  if (type == FUNCTION) return "function declaration";
  if (type == THREAD) return "thread declaration";
  if (type == RETURN) return "return";
  if (type == IF) return "if";
  if (type == ELSE) return "else";
  if (type == ELSEIF) return "elseif";
  if (type == FOR) return "for";
  if (type == WHILE) return "while";
  if (type == ALLOC) return "alloc";
  if (type == TYPE) return "type";
  if (type == CONST) return "const";
  if (type == STRING_LITERAL) return "string";
  if (type == DECIMAL) return "decimal number";
  if (type == FLOAT) return "floating point number";
  if (type == HEX) return "hex number";
  if (type == OCTAL) return "octal number";
  if (type == BINARY) return "binary number";
  if (type == OP_GT_BIT) return ">> operator";
  if (type == OP_LT_BIT) return "<< operator";
  if (type == OP_EQ) return "== operator";
  if (type == OP_AND) return "&& operator";
  if (type == OP_OR) return "|| operator";
  if (type == IDENTIFIER) return "identifier";
  if (type == FALSE) return "false";
  if (type == TRUE) return "true";
  if (type == UNDEF) return "undef";
  if (type == UNKNOWN) return "unknown token";
  if (type == NONE) return "empty token";
  if (type == GENERAL_EXPRESSION) return "expression";
  if (type == GENERAL_STATEMENT) return "statement";
  std::string res(1, (char)type);
  return "'" + res + "'";
}

std::string Token::get_name(void) const {
  return Token::get_name(this->type);
}

std::ostream &operator<<(std::ostream &os, const Token &t) {
  os << t.get_name();
  return os;
}

std::string &operator+(std::string &s, const Token &t) {
  s += t.get_name();
  return s;
}