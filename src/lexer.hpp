#if !defined(__LEXER_)
#define __LEXER_

#include "token.hpp"
#include <string>
#include <fstream>
#include <vector>
#include <locale>
#include <cstdint>

class Lexer {
  public:
    TokenList tokenize(const std::string &code);
    TokenList process_file(const std::string &filename);
    bool verbose = false;
    static const char **builtin_types;
    static int types_count;
  private:
    bool contains(const std::string &str, const char needle) const;
    bool valid_number(const std::string &str, int base) const;
    bool valid_float(const std::string &str) const;
    void add_unknown_token(std::string str);
    void add_char_token(const char c);
    void consume_whitespace(void);
    void consume_comment(void);
    void add_token(Token::TokenType type);
    void add_token(Token::TokenType type, const std::string &val);
    void log(std::string str) const;
    void unescape(std::string &str) const;
    std::string::const_iterator ptr;
    std::string::const_iterator end;
    std::int32_t current_line = 1;
    int deleted_spaces = 0;
    int prev_deleted_spaces = 0;
    std::string file_dir = "";
    std::string *file_name = new std::string;
    TokenList tokens;
};

#endif // __LEXER