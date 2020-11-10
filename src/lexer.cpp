#include "lexer.hpp"
#include "error-handler.hpp"
#include <iterator>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <cstdint>
#include <cstdlib>
#include <cassert>
#include <regex>

#define REG(tok_str, tok_sym) if(op==#tok_str){log("token ["#tok_sym"], ");add_token(Token::tok_sym);}else

static const char *_builtin_types[] = {
  "int", "double",
  "func", "str", "void",
  "obj", "arr"
};

const char **Lexer::builtin_types = _builtin_types;
int Lexer::types_count = sizeof(_builtin_types) / sizeof(char *);

void Lexer::log(std::string str) const {
  if (!this->verbose) return;
  std::cout << str << std::endl;
}

void Lexer::unescape(std::string &str) {
  std::vector<std::string> raws = {
    R"(\\n)", R"(\\t)", R"(\\a)", R"(\\r)", R"(\\b)", R"(\\v)"
  };
  std::vector<std::string> actual = {
    "\n", "\t", "\a", "\r", "\b", "\v"
  };
  for (int i = 0; i < raws.size(); i++) {
    str = std::regex_replace(str, std::regex(raws.at(i)), actual.at(i));
  }
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
  while (ptr != end && isspace(*ptr, loc)) {
    deleted_spaces++;
    if (*ptr++ == '\n') current_line++;
  }
}

void Lexer::add_token(Token::TokenType type) {
  add_token(type, "");
}

void Lexer::add_token(Token::TokenType type, const std::string &val) {
  tokens.push_back(Token(type, val, current_line));
}

void Lexer::add_unknown_token(std::string str) {
  log("token [UNKNOWN: " + str + "], ");
  add_token(Token::UNKNOWN, str);
  ErrorHandler::throw_syntax_error("unknown token '" + str + "'", current_line);
}

void Lexer::add_char_token(const char c) {
  std::stringstream s;
  s << "token [" << c << "], ";
  log(s.str());
  std::cout << std::flush;
  add_token((Token::TokenType)c);
}

TokenList Lexer::tokenize(const std::string &code) {
  this->ptr = code.begin();
  this->end = code.end();
  std::string chars = ".,:;{}[]()~$#";
  std::string chars2 = "=+-*&|/<>!%^";
  while (ptr != end) {
    deleted_spaces = 0;
    consume_whitespace();
    if (ptr == end) break;
    char c = *ptr;
    if (contains(chars, c)) {
      add_char_token(c);
    } else {
      c = *ptr;
      if (isalpha(c, loc) || c == '_') {
        std::string token_str(1, c);
        while (++ptr != end && (isalnum(*ptr, loc) || *ptr == '_')) {
          token_str += *ptr;
        }
        ptr--;
        if (token_str == "function") {
          log("token [FUNCTION], ");
          add_token(Token::FUNCTION);
        } else if (token_str == "class") {
          log("token [CLASS], ");
          add_token(Token::CLASS);
        } else if (token_str == "array") {
          log("token [ARRAY], ");
          add_token(Token::ARRAY);
        } else if (token_str == "return") {
          log("token [RETURN], ");
          add_token(Token::RETURN);
        } else if (token_str == "if") {
          log("token [IF], ");
          add_token(Token::IF);
        } else if (token_str == "else") {
          log("token [ELSE], ");
          add_token(Token::ELSE);
        } else if (token_str == "for") {
          log("token [FOR], ");
          add_token(Token::FOR);
        } else if (token_str == "while") {
          log("token [WHILE], ");
          add_token(Token::WHILE);
        } else if (token_str == "break") {
          log("token [WHILE], ");
          add_token(Token::BREAK);
        } else if (token_str == "continue") {
          log("token [CONTINUE], ");
          add_token(Token::CONTINUE);
        } else if (token_str == "alloc") {
          log("token [ALLOC], ");
          add_token(Token::ALLOC);
        } else if (token_str == "del") {
          log("token [DEL], ");
          add_token(Token::DEL);
        } else if (token_str == "ref") {
          log("token [REF], ");
          add_token(Token::REF);
        } else if (token_str == "true") {
          log("token [TRUE], ");
          add_token(Token::TRUE);
        } else if (token_str == "false") {
          log("token [FALSE], ");
          add_token(Token::FALSE);
        } else if (token_str == "const") {
          log("token [CONST], ");
          add_token(Token::CONST);
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
            log("token [IDENTIFIER: " + token_str + "], ");
            add_token(Token::IDENTIFIER, token_str);
          } else {
            log("token [TYPE: " + token_str + "], ");
            add_token(Token::TYPE, token_str);
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
        log("token [STRING_LITERAL: " + str + "], ");
        unescape(str);
        add_token(Token::STRING_LITERAL, str);
      } else if (isdigit(c, loc)) {
        // might be some kind of number
        std::string number_str = "";
        while (isdigit(*ptr, loc) || *ptr == '.' || *ptr == 'x' || *ptr == 'b') {
          number_str += *ptr++;
        }
        ptr--;
        bool converted = false;
        bool negation = tokens.size() != 0 && tokens.back().type == Token::OP_MINUS && !deleted_spaces;
        if (negation && !prev_deleted_spaces) {
          if (tokens.size()) {
            Token::TokenType t = tokens.back().type;
            if (t == Token::IDENTIFIER || t == Token::BINARY || t == Token::DECIMAL || t == Token::OCTAL || t == Token::FLOAT || t == Token::LEFT_PAREN) {
              negation = false;
            }
          }
        } 
        if (negation) {
          number_str = "-" + number_str;
          tokens.pop_back();
        }
        if (contains(number_str, 'x')) {
          // might be a hex
          if (valid_number(number_str, 16)) {
            log("token [HEX: " + number_str + "], ");
            add_token(Token::HEX, number_str);
            converted = true;
          }
        } else if (contains(number_str, 'b') && number_str.size() > 2) {
          // might be a binary number
          std::string binary_num = number_str.substr(2 + negation);
          if (negation) {
            binary_num = "-" + binary_num;
          }
          if (valid_number(binary_num, 2)) {
            log("token [BINARY: " + binary_num + "], ");
            add_token(Token::BINARY, binary_num);
            converted = true;
          }
        } else if (contains(number_str, '.')) {
          // might be a float
          if (valid_float(number_str)) {
            log("token [FLOAT: " + number_str + "], ");
            add_token(Token::FLOAT, number_str);
            converted = true;
          }
        } else if (number_str.c_str()[0] == '0') {
          // might be an octal number
          if (valid_number(number_str, 8)) {
            log("token [OCTAL: " + number_str + "], ");
            add_token(Token::OCTAL, number_str);
            converted = true;
          }
        } else {
          // might be a decimal
          if (valid_number(number_str, 10)) {
            log("token [DECIMAL: " + number_str + "], ");
            add_token(Token::DECIMAL, number_str);
            converted = true;
          }
        }
        if (!converted) {
          // couldn't convert the string to any type of number
          add_unknown_token(number_str);
        }
      } else if (contains(chars2, c)) {
        std::string op = "";
        // get any combination of "=+-*&|/<>!%"
        while (contains(chars2, *ptr)) {
          op += *ptr++;
        }
        ptr--;
        if (op.size() == 1) {
          add_char_token(c);
        } else if (op.size() > 1 && op.size() < 4) {
          REG(==, OP_EQ)
          REG(!=, OP_NOT_EQ)
          REG(&&, OP_AND)
          REG(||, OP_OR)
          REG(>>, RSHIFT)
          REG(<<, LSHIFT)
          REG(>>=, RSHIFT_ASSIGN)
          REG(<<=, LSHIFT_ASSIGN)
          REG(+=, PLUS_ASSIGN)
          REG(-=, MINUS_ASSIGN)
          REG(*=, MUL_ASSIGN)
          REG(/=, DIV_ASSIGN)
          REG(|=, OR_ASSIGN)
          REG(&=, AND_ASSIGN)
          REG(^=, XOR_ASSIGN)
          REG(%=, MOD_ASSIGN)
          REG(>=, OP_GT_EQ)
          REG(<=, OP_LT_EQ)
          add_unknown_token(op);
        } else {
          add_unknown_token(op);
        }
      } else {
        std::string junk(1, c);
        add_unknown_token(junk);
      }
    }
    prev_deleted_spaces = deleted_spaces;
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