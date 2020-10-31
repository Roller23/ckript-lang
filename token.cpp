#include "token.hpp"

// register a new name
#define REG(id, ret) if (type == id) return ret

std::string Token::get_name(TokenType type) {
  // trust me there's no better way
  REG(FUNCTION, "function declaration");
  REG(THREAD, "thread declaration");
  REG(RETURN, "return");
  REG(IF, "if");
  REG(ELSE, "else");
  REG(ELSEIF, "elseif");
  REG(FOR, "for");
  REG(WHILE, "while");
  REG(ALLOC, "alloc");
  REG(TYPE, "type");
  REG(CONST, "const");
  REG(STRING_LITERAL, "string");
  REG(DECIMAL, "decimal number");
  REG(FLOAT, "floating point number");
  REG(HEX, "hex number");
  REG(OCTAL, "octal number");
  REG(BINARY, "binary number");
  REG(OP_GT_BIT, ">> operator");
  REG(OP_LT_BIT, "<< operator");
  REG(OP_EQ, "== operator");
  REG(OP_AND, "&& operator");
  REG(OP_OR, "|| operator");
  REG(IDENTIFIER, "identifier");
  REG(FALSE, "false");
  REG(TRUE, "true");
  REG(UNDEF, "undef");
  REG(UNKNOWN, "unknown token");
  REG(NONE, "empty token");
  REG(GENERAL_EXPRESSION, "expression");
  REG(GENERAL_STATEMENT, "statement");
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