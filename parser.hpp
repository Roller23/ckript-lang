#if !defined(__PARSER_)
#define __PARSER_

#include "token.hpp"
#include <vector>

class Parser {
  public:
    void parse(std::vector<Token> &tokens);
};

#endif // __PARSER_