#if !defined(__PARSER_)
#define __PARSER_

#include "token.hpp"
#include "AST.hpp"
#include "utils.hpp"
#include <vector>
#include <cstdint>
#include <cstring>

class Parser {
  public:
    Parser(TokenList &_tokens, Token::TokenType _terminal, const std::string &_name, Utils &_utils) : 
      tokens(_tokens),
      curr_token(_tokens.size() ? _tokens.at(0) : Token()),
      tokens_count(_tokens.size()),
      terminal(_terminal),
      parser_name(_name),
      utils(_utils) {
        std::memset(base_lut, 0, sizeof(base_lut));
        base_lut[Token::TokenType::BINARY] = 2;
        base_lut[Token::TokenType::DECIMAL] = 10;
        base_lut[Token::TokenType::OCTAL] = 8;
        base_lut[Token::TokenType::HEX] = 16;
      }
    Node parse(int *end_pos);
    void advance();
    void retreat();
    Token lookahead(int offset = 1);
    Node get_statement(Node &prev, Token::TokenType stop);
    Node get_expr_node();
    NodeList get_expression(Token::TokenType stop1, Token::TokenType stop2 = Token::TokenType::NONE);
    bool right_assoc(Node &n);
    Node parse_func_expr();
    Node parse_class_stmt();
    Node parse_array_expr();
    ParamList parse_func_params(bool is_class = false);
    NodeListList get_many_expressions(Token::TokenType sep, Token::TokenType stop);
    Node get_declaration(Node &prev);
    NodeList get_many_statements(Node &prev, Token::TokenType stop);
    char base_lut[200];
  private:
    Utils &utils;
    TokenList &tokens;
    Token prev;
    Token curr_token;
    int pos = 0;
    int tokens_count;
    std::string parser_name;
    Token::TokenType terminal;
    void fail_if_EOF(Token::TokenType expected);
    int find_enclosing_brace(int start_pos, int braces = 0);
    int find_enclosing_semi(int start_pos);
    int find_enclosing_paren();
    int find_block_end(void);
    void throw_error(const std::string &cause, std::uint32_t line);
};

#endif // __PARSER_