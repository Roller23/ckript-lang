#if !defined(__TOKEN_)
#define __TOKEN_

#include <string>
#include <vector>

class Token;

typedef std::vector<Token> TokenList;

class Token {
  public:
    typedef enum type {
      FUNCTION, THREAD, RETURN, IF, ELSE, ELSEIF, FOR, WHILE, ALLOC, TYPE, CONST, // keywords

      OP_AND, OP_OR, OP_PLUS = '+', OP_MINUS = '-', OP_DIV = '/', OP_MUL = '*',
      OP_MOD = '%', OP_EQ, OP_LT = '<', OP_GT = '>', OP_NOT = '!', OP_OR_BIT = '|', OP_AND_BIT = '&',
      OP_GT_BIT, OP_LT_BIT,
      OP_ASSIGN = '=', // operators

      DOT = '.', COMMA = '.', COLON = ':', SEMI_COLON = ';', IDENTIFIER,
      LEFT_BRACE = '{', LEFT_BRACKET = '[', LEFT_PAREN = '(',
      RIGHT_BRACE = '}', RIGHT_BRACKET = ']', RIGHT_PAREN = ')',
      STRING_LITERAL, DECIMAL, FLOAT, HEX, OCTAL, BINARY,

      FALSE, TRUE, UNDEF,

      UNKNOWN, // junk

      NONE // empty token
    } TokenType;
    TokenType type;
    std::string value;
    Token(TokenType _type, std::string _value);;
};

#endif // __TOKEN_