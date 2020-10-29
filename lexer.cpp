#include "lexer.hpp"
#include <iterator>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <cstdint>

static void add_junk(std::vector<Token> &v, Lexer *l, const char c) {
  std::string junk;
  junk = c;
  l->log("[UNKNOWN: " + junk + "], ");
  v.push_back(Token(Token::UNKNOWN, junk));
}

static void add_junk(std::vector<Token> &v, Lexer *l, const std::string &s) {
  l->log("[UNKNOWN: " + s + "], ");
  v.push_back(Token(Token::UNKNOWN, s));
}

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
  "func", "thr", "str", "void"
};

const char **Lexer::builtin_types = _builtin_types;
int Lexer::types_count = sizeof(_builtin_types) / sizeof(char *);

void Lexer::log(std::string str) {
  if (!this->verbose) return;
  std::cout << str << std::endl;
}

std::vector<Token> Lexer::tokenize(const std::string &code) {
  std::vector<Token> tokens;
  auto ptr = code.begin();
  auto end = code.end();
  std::string chars = ".,:;{}[]()";
  std::string chars2 = "=+-*&|/<>!%";
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
        ptr--;
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
        } else if (token_str == "undef") {
          log("[UNDEF], ");
          tokens.push_back(Token(Token::UNDEF, ""));
        } else if (token_str == "const") {
          log("[CONST], ");
          tokens.push_back(Token(Token::CONST, ""));
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
        std::string number_str = "";
        while (isdigit(*ptr, loc)) {
          number_str += *ptr++;
        }
        ptr--;
        log("[NUMBER: " + number_str + "], ");
        tokens.push_back(Token(Token::NUMBER, number_str));
      } else if (chars2.find(c) != std::string::npos) {
        std::string op = "";
        while (chars2.find(*ptr) != std::string::npos) {
          op += *ptr++;
        }
        ptr--;
        if (op.size() == 1) {
          std::stringstream s;
          s << "[" << c << "], ";
          log(s.str());
          std::cout << std::flush;
          tokens.push_back(Token(Token::get_type(c), ""));
        } else if (op.size() == 2) {
          if (op == "==") {
            log("[OP_ASSIGN: " + op + "], ");
            tokens.push_back(Token(Token::OP_ASSIGN, op));
          } else if (op == "&&") {
            log("[OP_AND: " + op + "], ");
            tokens.push_back(Token(Token::OP_AND, op));
          } else if (op == "||") {
            log("[OP_OR: " + op + "], ");
            tokens.push_back(Token(Token::OP_OR, op));
          } else {
            add_junk(tokens, this, op);
          }
        } else {
          add_junk(tokens, this, op);
        }
      } else {
        add_junk(tokens, this, c);
      }
    }
    ptr++;
  }
  log("\n");
  return tokens;
}

void Lexer::process_file(const std::string &filename) {
  std::ifstream file(filename);
  if (!file) {
    this->last_error = FILE_ERROR;
    return;
  }
  std::string buffer(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>{});
  tokenize(buffer);
}