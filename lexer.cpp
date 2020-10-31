#include "lexer.hpp"
#include "error-handler.hpp"
#include <iterator>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <cstdint>
#include <cstdlib>

static const char *_builtin_types[] = {
  "int8", "int16", "int32", "int64",
  "uint8", "uint16", "uint32", "uint64",
  "f64", "f32",
  "func", "thr", "str", "void"
};

const char **Lexer::builtin_types = _builtin_types;
int Lexer::types_count = sizeof(_builtin_types) / sizeof(char *);

void Lexer::log(std::string str) const {
  if (!this->verbose) return;
  std::cout << str << std::endl;
}

bool Lexer::contains(const std::string &str, const char needle) const {
  return str.find(needle) != std::string::npos;
}

bool Lexer::valid_number(const std::string &str, int base) const {
  char *endptr;
  std::strtoull(str.c_str(), &endptr, base);
  return *endptr == 0;
}

bool Lexer::valid_float(const std::string &str) const {
  char *endptr;
  std::strtod(str.c_str(), &endptr);
  return *endptr == 0;
}

void Lexer::consume_whitespace(void) {
  while (ptr != end && isspace(*ptr, loc)) ptr++;
}

void Lexer::consume_comment(void) {
  while (ptr != end) {
    const char c = *++ptr;
    if (c == '\n' || c == '#') break;
  }
  if (ptr != end) ptr++; // skip the closing \n or #
}

void Lexer::add_unknown_token(TokenList &tokens, std::string str) {
  log("[UNKNOWN: " + str + "], ");
  tokens.push_back(Token(Token::UNKNOWN, str));
  ErrorHandler::thow_syntax_error("unknown token '" + str + "'");
}

void Lexer::add_char_token(TokenList &tokens, const char c) const {
  std::stringstream s;
  s << "[" << c << "], ";
  log(s.str());
  std::cout << std::flush;
  tokens.push_back(Token((Token::TokenType)c));
}

TokenList Lexer::tokenize(const std::string &code) {
  TokenList tokens;
  this->ptr = code.begin();
  this->end = code.end();
  std::string chars = ".,:;{}[]()";
  std::string chars2 = "=+-*&|/<>!%";
  while (ptr != end) {
    consume_whitespace();
    if (ptr == end) break;
    if (*ptr == '#') {
      // start of a comment
      consume_comment();
      if (ptr == end) break;
      // take care of all the whilespace after the comment
      consume_whitespace();
      if (ptr == end) break;
    }
    const char c = *ptr;
    if (contains(chars, c)) {
      add_char_token(tokens, c);
    } else {
      if (isalpha(c, loc)) {
        std::string token_str(1, c);
        while (++ptr != end && isalnum(*ptr, loc)) {
          token_str += *ptr;
        }
        ptr--;
        if (token_str == "function") {
          log("[FUNCTION], ");
          tokens.push_back(Token(Token::FUNCTION));
        } else if (token_str == "thread") {
          log("[THREAD], ");
          tokens.push_back(Token(Token::THREAD));
        } else if (token_str == "return") {
          log("[RETURN], ");
          tokens.push_back(Token(Token::RETURN));
        } else if (token_str == "if") {
          log("[IF], ");
          tokens.push_back(Token(Token::IF));
        } else if (token_str == "else") {
          log("[ELSE], ");
          tokens.push_back(Token(Token::ELSE));
        } else if (token_str == "elseif") {
          log("[ELSEIF], ");
          tokens.push_back(Token(Token::ELSEIF));
        } else if (token_str == "for") {
          log("[FOR], ");
          tokens.push_back(Token(Token::FOR));
        } else if (token_str == "while") {
          log("[WHILE], ");
          tokens.push_back(Token(Token::WHILE));
        } else if (token_str == "alloc") {
          log("[ALLOC], ");
          tokens.push_back(Token(Token::ALLOC));
        } else if (token_str == "true") {
          log("[TRUE], ");
          tokens.push_back(Token(Token::TRUE));
        } else if (token_str == "false") {
          log("[FALSE], ");
          tokens.push_back(Token(Token::FALSE));
        } else if (token_str == "undef") {
          log("[UNDEF], ");
          tokens.push_back(Token(Token::UNDEF));
        } else if (token_str == "const") {
          log("[CONST], ");
          tokens.push_back(Token(Token::CONST));
        } else {
          // to do - speed this up (LUT maybe?)
          std::string found_type = "";
          for (int i = 0; i < Lexer::types_count; i++) {
            if (token_str == Lexer::builtin_types[i]) {
              found_type = Lexer::builtin_types[i];
              break;
            }
          }
          if (found_type == "") {
            // probably an identifier
            log("[IDENTIFIER: " + token_str + "], ");
            tokens.push_back(Token(Token::IDENTIFIER, token_str));
          } else {
            log("[TYPE: " + token_str + "], ");
            tokens.push_back(Token(Token::TYPE, token_str));
          }
        }
      } else if (c == '"' || c == '\'' || c == '`') {
        // start of a string
        std::string str = "";
        ptr++;
        while (*ptr != c && ptr != end) {
          str += *ptr;
          ptr++;
        }
        log("[STRING_LITERAL: " + str + "], ");
        tokens.push_back(Token(Token::STRING_LITERAL, str));
      } else if (isdigit(c, loc)) {
        // might be some kind of number
        std::string number_str = "";
        while (isdigit(*ptr, loc) || *ptr == '.' || *ptr == 'x' || *ptr == 'b') {
          number_str += *ptr++;
        }
        ptr--;
        bool converted = false;
        if (contains(number_str, 'x')) {
          // might be a hex
          if (valid_number(number_str, 16)) {
            log("[HEX: " + number_str + "], ");
            tokens.push_back(Token(Token::HEX, number_str));
            converted = true;
          }
        } else if (contains(number_str, 'b') && number_str.size() > 2) {
          // might be a binary number
          std::string binary_num = number_str.substr(2);
          if (valid_number(binary_num, 2)) {
            log("[BINARY: " + binary_num + "], ");
            tokens.push_back(Token(Token::BINARY, binary_num));
            converted = true;
          }
        } else if (contains(number_str, '.')) {
          // might be a float
          if (valid_float(number_str)) {
            log("[FLOAT: " + number_str + "], ");
            tokens.push_back(Token(Token::FLOAT, number_str));
            converted = true;
          }
        } else if (number_str.c_str()[0] == '0') {
          // might be an octal number
          if (valid_number(number_str, 8)) {
            log("[OCTAL: " + number_str + "], ");
            tokens.push_back(Token(Token::OCTAL, number_str));
            converted = true;
          }
        } else {
          // might be a decimal
          if (valid_number(number_str, 16)) {
            log("[DECIMAL: " + number_str + "], ");
            tokens.push_back(Token(Token::DECIMAL, number_str));
            converted = true;
          }
        }
        if (!converted) {
          // couldn't convert the string to any type of number
          add_unknown_token(tokens, number_str);
        }
      } else if (contains(chars2, c)) {
        std::string op = "";
        // get any combination of "=+-*&|/<>!%"
        while (contains(chars2, *ptr)) {
          op += *ptr++;
        }
        ptr--;
        if (op.size() == 1) {
          add_char_token(tokens, c);
        } else if (op.size() == 2) {
          if (op == "==") {
            log("[OP_EQ: " + op + "], ");
            tokens.push_back(Token(Token::OP_EQ));
          } else if (op == "&&") {
            log("[OP_AND: " + op + "], ");
            tokens.push_back(Token(Token::OP_AND));
          } else if (op == "||") {
            log("[OP_OR: " + op + "], ");
            tokens.push_back(Token(Token::OP_OR));
          } else {
            add_unknown_token(tokens, op);
          }
        } else {
          add_unknown_token(tokens, op);
        }
      } else {
        std::string junk(1, c);
        add_unknown_token(tokens, junk);
      }
    }
    ptr++;
  }
  return tokens;
}

TokenList Lexer::process_file(const std::string &filename) {
  TokenList result;
  std::ifstream file(filename);
  if (!file) {
    ErrorHandler::throw_file_error("Couldn't open " + filename);
  }
  std::string buffer(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>{});
  result = tokenize(buffer);
  return result;
}