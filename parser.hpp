#if !defined(__PARSER_)
#define __PARSER_

#include "token.hpp"
#include "AST.hpp"
#include <vector>

class Parser {
  public:
    Parser(TokenList &_tokens) : 
      tokens(_tokens),
      curr_token(_tokens.at(0)),
      tokens_count(_tokens.size()) {}
    void parse();
    void advance();
    void retreat();
    Node get_statement(Node &prev, Token::TokenType stop);
    Node get_expression(Node &prev, Token::TokenType stop);
    Node get_declaration(Node &prev);
    NodeList get_multiple_statements(Node &prev, Token::TokenType stop);
  private:
    TokenList &tokens;
    Token prev;
    Token curr_token;
    int pos = 0;
    int tokens_count;
};

#endif // __PARSER_