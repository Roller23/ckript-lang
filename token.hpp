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
      OP_MOD = '%', OP_LT = '<', OP_GT = '>', OP_NOT = '!',
      OP_OR_BIT = '|', OP_AND_BIT = '&', OP_ASSIGN = '=',

      DOT = '.', COMMA = '.', COLON = ':', SEMI_COLON = ';',
      LEFT_BRACE = '{', LEFT_BRACKET = '[', LEFT_PAREN = '(',
      RIGHT_BRACE = '}', RIGHT_BRACKET = ']', RIGHT_PAREN = ')',

      FUNCTION = 130, THREAD, RETURN, IF, ELSE, ELSEIF,
      FOR, WHILE, ALLOC, TYPE, CONST,

      STRING_LITERAL, DECIMAL, FLOAT, HEX, OCTAL, BINARY,

      OP_GT_BIT, OP_LT_BIT, OP_AND, OP_OR, OP_EQ,

      IDENTIFIER,

      FALSE, TRUE, UNDEF,

      UNKNOWN,

      NONE, GENERAL_EXPRESSION, GENERAL_STATEMENT // special tokens
    } TokenType;
    TokenType type;
    std::string value;
    Token(TokenType _type = Token::NONE, std::string _value = "") : type(_type), value(_value) {};
    std::string get_name(void) const;
    static std::string get_name(TokenType type);
    friend std::ostream &operator<<(std::ostream& os, const Token &t);
    friend std::string &operator+(std::string &s, const Token &t);
};

#endif // __TOKEN_