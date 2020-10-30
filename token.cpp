#include "token.hpp"

Token::Token(void) {
  this->type = Token::NONE;
  this->value = "";
}

Token::Token(enum type _type, std::string _value) {
  this->type = _type;
  this->value = _value;
}