#if !defined(__PARSER_)
#define __PARSER_

#include "token.hpp"
#include "AST.hpp"
#include <vector>

class Parser {
  public:
    Parser(TokenList &_tokens) : 
      tokens(_tokens),
      current(_tokens.at(0)),
      tokens_count(_tokens.size()) {}
    void parse();
    void move();
    Expression parse_expression();
  private:
    TokenList &tokens;
    Token prev;
    Token current;
    int pos = 0;
    int tokens_count;
};

#endif // __PARSER_