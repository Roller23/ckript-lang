#include "token.hpp"

// register a new name
#define REG(id, ret) if (type == id) return ret

std::string Token::get_name(TokenType type) {
  // trust me there's no better way
  REG(FUNCTION, "function");
  REG(THREAD, "thread");
  REG(RETURN, "return");
  REG(IF, "if");
  REG(ELSE, "else");
  REG(FOR, "for");
  REG(WHILE, "while");
  REG(ALLOC, "alloc");
  REG(DEL, "del");
  REG(TYPE, "type");
  REG(CONST, "const");
  REG(STRING_LITERAL, "string");
  REG(DECIMAL, "decimal number");
  REG(FLOAT, "floating point number");
  REG(HEX, "hex number");
  REG(OCTAL, "octal number");
  REG(BINARY, "binary number");
  REG(OP_EQ, "==");
  REG(OP_NOT_EQ, "!=");
  REG(OP_GT_EQ, ">=");
  REG(OP_LT_EQ, "<=");
  REG(OP_AND, "&&");
  REG(OP_OR, "||");
  REG(LSHIFT, "<<");
  REG(RSHIFT, ">>");
  REG(LSHIFT_ASSIGN, "<<=");
  REG(RSHIFT_ASSIGN, ">>=");
  REG(PLUS_ASSIGN, "+=");
  REG(MINUS_ASSIGN, "-=");
  REG(MUL_ASSIGN, "*=");
  REG(DIV_ASSIGN, "/=");
  REG(OR_ASSIGN, "|=");
  REG(AND_ASSIGN, "&=");
  REG(XOR_ASSIGN, "^=");
  REG(IDENTIFIER, "identifier");
  REG(FALSE, "false");
  REG(TRUE, "true");
  REG(UNDEF, "undef");
  REG(UNKNOWN, "unknown token");
  REG(NONE, "empty token");
  REG(GENERAL_EXPRESSION, "expression");
  REG(GENERAL_STATEMENT, "statement");
  return {1, (char)type};
}

std::string Token::get_name(void) const {
  return Token::get_name(this->type);
}

std::ostream &operator<<(std::ostream &os, const Token &t) {
  os << t.get_name();
  return os;
}