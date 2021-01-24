#if !defined(__TOKEN_)
#define __TOKEN_

#include <string>
#include <sstream>
#include <vector>
#include <cstdint>

class Token;

typedef std::vector<Token> TokenList;

class Token {
  public:
    typedef enum type {
      OP_PLUS = '+', OP_MINUS = '-', OP_DIV = '/', OP_MUL = '*',
      OP_MOD = '%', OP_LT = '<', OP_GT = '>', OP_NOT = '!', OP_NEG = '~',
      OP_OR_BIT = '|', OP_AND_BIT = '&', OP_ASSIGN = '=', OP_XOR = '^',

      DOT = '.', COMMA = ',', COLON = ':', SEMI_COLON = ';',
      LEFT_BRACE = '{', LEFT_BRACKET = '[', LEFT_PAREN = '(',
      RIGHT_BRACE = '}', RIGHT_BRACKET = ']', RIGHT_PAREN = ')', SET = '$', SET_IDX = '#',

      FUNCTION = 130, RETURN, IF, ELSE, BREAK, CONTINUE,
      FOR, WHILE, ALLOC, DEL, TYPE, REF, CONST,

      STRING_LITERAL, DECIMAL, FLOAT, HEX, OCTAL, BINARY, ARRAY, CLASS,

      PLUS_ASSIGN, MINUS_ASSIGN, MUL_ASSIGN, DIV_ASSIGN, MOD_ASSIGN,
      LSHIFT_ASSIGN, RSHIFT_ASSIGN, AND_ASSIGN, OR_ASSIGN, XOR_ASSIGN,

      LSHIFT, RSHIFT,

      OP_AND, OP_OR, OP_EQ, OP_NOT_EQ, OP_GT_EQ, OP_LT_EQ,

      IDENTIFIER,

      FALSE, TRUE,

      UNKNOWN,

      NONE, GENERAL_EXPRESSION, GENERAL_STATEMENT // special tokens
    } TokenType;
    TokenType type = NONE;
    std::string value = "";
    std::uint64_t line = 0;
    std::string *source = nullptr;
    Token(void) : type(NONE) {};
    Token(TokenType _type, const std::string &val, int _line, std::string *&_source) :
      type(_type), value(val), line(_line), source(_source) {}
    std::string get_name(void) const;
    static std::string get_name(TokenType type);
    friend std::ostream &operator<<(std::ostream &os, const Token &t);
};

#endif // __TOKEN_