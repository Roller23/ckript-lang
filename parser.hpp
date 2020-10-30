#if !defined(__PARSER_)
#define __PARSER_

#include "token.hpp"
#include <vector>

class Parser {
  public:
    void parse(TokenList &tokens);
    Expression parse_expression(const Nodes &tree, int prev, int current);
};

#endif // __PARSER_