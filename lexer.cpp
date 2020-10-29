#include "lexer.hpp"
#include <iterator>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <codecvt>
#include <cstdint>
#include <map>

Token::Token(enum type _type, std::string _value) {
  this->type = _type;
  this->value = _value;
}

Token::_type_ Token::get_type(char c) {
  return (enum type)c;
}

static const char *_builtin_types[] = {
  "int8", "int16", "int32", "int64",
  "uint8", "uint16", "uint32", "uint64",
  "f64", "f32",
  "func", "thr", "string", "void"
};

const char **Lexer::builtin_types = _builtin_types;

void Lexer::log(std::string str) {
  if (!this->verbose) return;
  std::cout << str;
}

std::vector<Token> Lexer::tokenize(const std::string &code) {
  std::vector<Token> tokens;
  auto ptr = code.begin();
  auto end = code.end();
  std::string chars = ".,:;{}[]()!";
  std::string chars2 = "=+-*&^|/<>";
  std::locale loc{""};
  while (ptr != end) {
    while (ptr != end && isspace(*ptr, loc)) {
      ptr++;
    }
    if (ptr == end) {
      break;
    }
    const char c = *ptr;
    if (chars.find(c) != std::string::npos) {
      std::stringstream s;
      s << "[" << c << "], ";
      log(s.str());
      std::cout << std::flush;
      tokens.push_back(Token(Token::get_type(c), ""));
    } else {
      std::string token_str;
      if (isalpha(c, loc)) {
        token_str = c;
        while (++ptr != end && isalnum(*ptr, loc)) {
          token_str += *ptr;
        }
        if (token_str == "function") {
          log("[FUNCTION], ");
          tokens.push_back(Token(Token::FUNCTION, ""));
        } else if (token_str == "thread") {
          log("[THREAD], ");
          tokens.push_back(Token(Token::THREAD, ""));
        } else if (token_str == "return") {
          log("[RETURN], ");
          tokens.push_back(Token(Token::RETURN, ""));
        } else if (token_str == "if") {
          log("[IF], ");
          tokens.push_back(Token(Token::IF, ""));
        } else if (token_str == "for") {
          log("[FOR], ");
          tokens.push_back(Token(Token::FOR, ""));
        } else if (token_str == "while") {
          log("[WHILE], ");
          tokens.push_back(Token(Token::WHILE, ""));
        } else if (token_str == "alloc") {
          log("[ALLOC], ");
          tokens.push_back(Token(Token::ALLOC, ""));
        } else if (token_str == "true") {
          log("[TRUE], ");
          tokens.push_back(Token(Token::TRUE, ""));
        } else if (token_str == "false") {
          log("[FALSE], ");
          tokens.push_back(Token(Token::FALSE, ""));
        } else {
          log("[IDENTIFIER: " + token_str + "], ");
          tokens.push_back(Token(Token::IDENTIFIER, token_str));
        }
      }
    }
    ptr++;
  }
  log("[EOF]");
  log("\n");
  tokens.push_back(Token(Token::_EOF, ""));
  return tokens;
}

// else if (token_str.size() == 1) {
//   if (chars2.find(token_str.c_str()[0]) != std::string::npos) {
//     std::stringstream s;
//     s << "[" << c << "], ";
//     log(s.str());
//     std::cout << std::flush;
//     tokens.push_back(Token(Token::get_type(c), ""));
//   }
// }

void Lexer::process_file(const std::string &filename) {
  std::ifstream file(filename);
  if (!file) {
    this->last_error = FILE_ERROR;
    return;
  }
  std::string buffer(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>{});
  tokenize(buffer);
}