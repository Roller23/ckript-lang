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
        std::memset(base_lut, 0, 200 * sizeof(char));
        std::memset(op_precedence, 0, 200 * sizeof(char));
        base_lut[Token::TokenType::BINARY] = 2;
        base_lut[Token::TokenType::DECIMAL] = 10;
        base_lut[Token::TokenType::OCTAL] = 8;
        base_lut[Token::TokenType::HEX] = 16;
        // from lowest to highest
        op_precedence[Token::TokenType::AND_ASSIGN] = 1; // &=
        op_precedence[Token::TokenType::OR_ASSIGN] = 1; // |=
        op_precedence[Token::TokenType::XOR_ASSIGN] = 1; // ^=
        op_precedence[Token::TokenType::OP_ASSIGN] = 1; // =
        op_precedence[Token::TokenType::PLUS_ASSIGN] = 1; // +=
        op_precedence[Token::TokenType::MINUS_ASSIGN] = 1; // -=
        op_precedence[Token::TokenType::MUL_ASSIGN] = 1; // *=
        op_precedence[Token::TokenType::DIV_ASSIGN] = 1; // /=
        op_precedence[Token::TokenType::MOD_ASSIGN] = 1; // %=
        op_precedence[Token::TokenType::LSHIFT_ASSIGN] = 1; // <<=
        op_precedence[Token::TokenType::RSHIFT_ASSIGN] = 1; // >>=
        op_precedence[Token::TokenType::OP_OR] = 2; // ||
        op_precedence[Token::TokenType::OP_AND] = 3; // &&
        op_precedence[Token::TokenType::OP_OR_BIT] = 4; // |
        op_precedence[Token::TokenType::OP_XOR] = 5; // ^
        op_precedence[Token::TokenType::OP_AND] = 6; // &
        op_precedence[Token::TokenType::OP_NOT_EQ] = 7; // !=
        op_precedence[Token::TokenType::OP_EQ] = 7; // ==
        op_precedence[Token::TokenType::OP_GT] = 8; // >
        op_precedence[Token::TokenType::OP_LT] = 8; // <
        op_precedence[Token::TokenType::OP_GT_EQ] = 8; // >=
        op_precedence[Token::TokenType::OP_LT_EQ] = 8; // <=
        op_precedence[Token::TokenType::LSHIFT] = 9; // <<
        op_precedence[Token::TokenType::RSHIFT] = 9; // >>
        op_precedence[Token::TokenType::OP_PLUS] = 10; // +
        op_precedence[Token::TokenType::OP_MINUS] = 10; // -
        op_precedence[Token::TokenType::OP_MUL] = 11; // *
        op_precedence[Token::TokenType::OP_DIV] = 11; // /
        op_precedence[Token::TokenType::OP_MOD] = 11; // %
        op_precedence[Token::TokenType::DOT] = 13; // .
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
    char get_precedence(Token::TokenType token);
    char base_lut[200];
    char op_precedence[200];
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