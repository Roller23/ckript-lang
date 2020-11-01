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
      curr_token(_tokens.size() ? _tokens.at(0) : Token::TokenType::NONE),
      tokens_count(_tokens.size()),
      terminal(_terminal),
      parser_name(_name) {
        std::memset(binary_tokens_lut, 0, 200 * sizeof(bool));
        std::memset(base_lut, 0, 200 * sizeof(char));
        binary_tokens_lut[Token::TokenType::OP_AND] = true;
        binary_tokens_lut[Token::TokenType::OP_AND_BIT] = true;
        binary_tokens_lut[Token::TokenType::OP_OR] = true;
        binary_tokens_lut[Token::TokenType::OP_OR_BIT] = true;
        binary_tokens_lut[Token::TokenType::OP_XOR] = true;
        binary_tokens_lut[Token::TokenType::OP_EQ] = true;
        binary_tokens_lut[Token::TokenType::OP_NOT_EQ] = true;
        binary_tokens_lut[Token::TokenType::OP_ASSIGN] = true;
        binary_tokens_lut[Token::TokenType::OP_GT] = true;
        binary_tokens_lut[Token::TokenType::LSHIFT] = true;
        binary_tokens_lut[Token::TokenType::OP_LT] = true;
        binary_tokens_lut[Token::TokenType::RSHIFT] = true;
        binary_tokens_lut[Token::TokenType::OP_PLUS] = true;
        binary_tokens_lut[Token::TokenType::OP_MINUS] = true;
        binary_tokens_lut[Token::TokenType::OP_DIV] = true;
        binary_tokens_lut[Token::TokenType::OP_MUL] = true;
        binary_tokens_lut[Token::TokenType::LSHIFT_ASSIGN] = true;
        binary_tokens_lut[Token::TokenType::RSHIFT_ASSIGN] = true;
        binary_tokens_lut[Token::TokenType::PLUS_ASSIGN] = true;
        binary_tokens_lut[Token::TokenType::MINUS_ASSIGN] = true;
        binary_tokens_lut[Token::TokenType::MUL_ASSIGN] = true;
        binary_tokens_lut[Token::TokenType::DIV_ASSIGN] = true;
        binary_tokens_lut[Token::TokenType::AND_ASSIGN] = true;
        binary_tokens_lut[Token::TokenType::OR_ASSIGN] = true;
        binary_tokens_lut[Token::TokenType::XOR_ASSIGN] = true;

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
    NodeList get_many_statements(Node &prev, Token::TokenType stop);
    bool op_binary(Token::TokenType token);
    bool op_unary(Token::TokenType token);
    bool binary_tokens_lut[200];
    char base_lut[200];
  private:
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
    int find_block_end(void);
    void throw_error(const std::string &cause, std::uint32_t line);
};

#endif // __PARSER_