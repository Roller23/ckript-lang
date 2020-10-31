#if !defined(__PARSER_)
#define __PARSER_

#include "token.hpp"
#include "AST.hpp"
#include <vector>

class Parser {
  public:
    Parser(TokenList &_tokens, Token::TokenType _terminal, const std::string &_name = "MAIN") : 
      tokens(_tokens),
      curr_token(_tokens.at(0)),
      tokens_count(_tokens.size()),
      terminal(_terminal),
      parser_name(_name) {}
    Node parse(int *end_pos);
    void advance();
    void retreat();
    Token lookahead(int offset = 1);
    Node get_statement(Node &prev, Token::TokenType stop);
    Node get_expression(Node &prev, Token::TokenType stop1, Token::TokenType stop2 = Token::NONE);
    Node parse_func_expr();
    ParamList parse_func_params();
    NodeList get_many_expressions(Node &prev, Token::TokenType sep, Token::TokenType stop);
    Node get_declaration(Node &prev);
    void get_many_statements(Node &prev, Token::TokenType stop);
  private:
    TokenList &tokens;
    Token prev;
    Token curr_token;
    int pos = 0;
    int tokens_count;
    int statement_limit = -1;
    std::string parser_name;
    Token::TokenType terminal;
    void fail_if_EOF(Token::TokenType expected);
    int find_func_end_brace(TokenList &tokens, int start_pos);
    int find_func_end_semi(TokenList &tokens, int start_pos);
};

#endif // __PARSER_