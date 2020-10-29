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
      FILE_ERROR
    };
    std::vector<Token> tokenize(const std::string &code);
    std::vector<Token> process_file(const std::string &filename);
    void log(std::string str);
    enum lexing_error last_error;
    bool verbose = false;
    static const char **builtin_types;
    static int types_count;
};

#endif // __LEXER