#if !defined(__LEXER_)
#define __LEXER_

#include <string>
#include <fstream>
#include <vector>

      // FUNC, THR,
      // INT8, INT16, INT32, INT64,
      // UINT8, UINT16, UINT32, UINT64,
      // DOUBLE, FLOAT,
      // STRING, ARRAY, OBJECT, VOID, // build int types

class Token {
  public:
    typedef enum type {
      FUNCTION, THREAD, RETURN, IF, FOR, WHILE, ALLOC, TYPE, // keywords

      OP_AND, OP_OR, OP_PLUS = '+', OP_MINUS = '-', OP_DIV = '/', OP_MUL = '*',
      OP_MOD = '%', OP_EQ = '=', OP_LT = '<', OP_GT = '>', OP_NOT = '!', OP_OR_BIT = '|', OP_AND_BIT = '&',
      OP_GT_BIT, OP_LT_BIT, // operators

      DOT = '.', COMMA = '.', COLON = ':', SEMI_COLON = ';', IDENTIFIER,
      LEFT_BRACE = '{', LEFT_BRACKET = '[', LEFT_PAREN = '(',
      RIGHT_BRACE = '}', RIGHT_BRACKET = ']', RIGHT_PAREN = ')',
      NUMBER, STRING_LITERAL,

      FALSE, TRUE, // booleans

      _EOF, // end of file
      UNKNOWN // junk
    } _type_;
    enum type type;
    std::string value;
    Token(enum type _type, std::string _value);
    static enum type get_type(char c);
    static std::string get_type_name(enum type _type);
};

class Lexer {
  public:
    enum lexing_error {
      OK = 0,
      FILE_ERROR
    };
    std::vector<Token> tokenize(const std::string &code);
    void process_file(const std::string &filename);
    void log(std::string str);
    enum lexing_error last_error;
    bool verbose = false;
    static const char **builtin_types;
    static int types_count;
};

#endif // __LEXER