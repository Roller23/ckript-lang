#if !defined(__LEXER_)
#define __LEXER_

#include "token.hpp"
#include <string>
#include <fstream>
#include <vector>
#include <locale>

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
    void add_unknown_token(TokenList &tokens, std::string str);
    void add_char_token(TokenList &tokens, const char c) const;
    void consume_whitespace(void);
    void consume_comment(void);
    void log(std::string str) const;
    std::string::const_iterator ptr;
    std::string::const_iterator end;
    std::locale loc{""};
};

#endif // __LEXER