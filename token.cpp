#include "token.hpp"

Token::Token(enum type _type, std::string _value) {
  this->type = _type;
  this->value = _value;
}