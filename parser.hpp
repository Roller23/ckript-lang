#if !defined(__PARSER_)
#define __PARSER_

#include "token.hpp"
#include "AST.hpp"
#include <vector>
#include <cstdint>
#include <cstring>

class Parser {
  public:
    Parser(TokenList &_tokens, Token::TokenType _terminal, const std::string &_name = "MAIN") : 
      tokens(_tokens),
      curr_token(_tokens.at(0)),
      tokens_count(_tokens.size()),
      terminal(_terminal),
      parser_name(_name) {
        std::memset(binary_tokens_lut, 0, 200 * sizeof(bool));
        std::memset(base_lut, 0, 200 * sizeof(char));
        binary_tokens_lut[Token::TokenType::OP_AND] = true;
        binary_tokens_lut[Token::TokenType::OP_AND_BIT] = true;
        binary_tokens_lut[Token::TokenType::OP_OR] = true;
        binary_tokens_lut[Token::TokenType::OP_OR_BIT] = true;
        binary_tokens_lut[Token::TokenType::OP_EQ] = true;
        binary_tokens_lut[Token::TokenType::OP_GT] = true;
        binary_tokens_lut[Token::TokenType::OP_GT_BIT] = true;
        binary_tokens_lut[Token::TokenType::OP_LT] = true;
        binary_tokens_lut[Token::TokenType::OP_LT_BIT] = true;
        binary_tokens_lut[Token::TokenType::OP_PLUS] = true;
        binary_tokens_lut[Token::TokenType::OP_MINUS] = true;
        binary_tokens_lut[Token::TokenType::OP_DIV] = true;
        binary_tokens_lut[Token::TokenType::OP_MUL] = true;
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
    Node get_expression(Node &prev, Token::TokenType stop1, Token::TokenType stop2 = Token::NONE);
    Node parse_func_expr();
    ParamList parse_func_params();
    NodeList get_many_expressions(Node &prev, Token::TokenType sep, Token::TokenType stop);
    Node get_declaration(Node &prev);
    void get_many_statements(Node &prev, Token::TokenType stop);
    bool op_binary(Token::TokenType token);
    bool binary_tokens_lut[200];
    char base_lut[200];
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
    void throw_error(const std::string &cause, std::uint32_t line);
};

#endif // __PARSER_