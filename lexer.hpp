#if !defined(__LEXER_)
#define __LEXER_

#include "token.hpp"
#include <string>
#include <fstream>
#include <vector>

class Lexer {
  public:
    enum lexing_error {
      OK = 0,
      FILE_ERROR,
      TOKENIZING_ERROR
    };
    TokenList tokenize(const std::string &code);
    TokenList process_file(const std::string &filename);
    void log(std::string str) const;
    enum lexing_error last_error;
    bool verbose = false;
    static const char **builtin_types;
    static int types_count;
  private:
    bool contains(const std::string &str, const char needle) const;
    bool valid_number(const std::string &str, int base);
    bool valid_float(const std::string &str);
    void add_unknown_token(TokenList &tokens, std::string str);
    void add_char_token(TokenList &tokens, const char c);
};

#endif // __LEXER