#if !defined(__TOKEN_)
#define __TOKEN_

#include <string>
#include <vector>

class Token;

typedef std::vector<Token> TokenList;

class Token {
  public:
    typedef enum type {
      OP_PLUS = '+', OP_MINUS = '-', OP_DIV = '/', OP_MUL = '*',
      OP_MOD = '%', OP_EQ, OP_LT = '<', OP_GT = '>', OP_NOT = '!',
      OP_OR_BIT = '|', OP_AND_BIT = '&', OP_ASSIGN = '=',

      DOT = '.', COMMA = '.', COLON = ':', SEMI_COLON = ';',
      LEFT_BRACE = '{', LEFT_BRACKET = '[', LEFT_PAREN = '(',
      RIGHT_BRACE = '}', RIGHT_BRACKET = ']', RIGHT_PAREN = ')',

      FUNCTION = 130, THREAD, RETURN, IF, ELSE, ELSEIF,
      FOR, WHILE, ALLOC, TYPE, CONST,

      STRING_LITERAL, DECIMAL, FLOAT, HEX, OCTAL, BINARY,

      OP_GT_BIT, OP_LT_BIT, OP_AND, OP_OR,

      IDENTIFIER,

      FALSE, TRUE, UNDEF,

      UNKNOWN,

      NONE
    } TokenType;
    TokenType type;
    std::string value;
    Token(void);
    Token(TokenType _type, std::string _value);
};

#endif // __TOKEN_