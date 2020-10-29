#if !defined(__LEXER_)
#define __LEXER_

#include <string>
#include <fstream>

class Lexer {
  public:
    enum lexing_error {
      OK = 0,
      FILE_ERROR
    };
    static enum lexing_error lexing_errors; 
    void tokenize(const std::string &code);
    void process_file(const std::string &filename);
    enum lexing_error last_error;
};

#endif // __LEXER