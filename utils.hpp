#if !defined(__UTILS_)
#define __UTILS_

#include "token.hpp"
#include "AST.hpp"

class Utils {
  public:
    bool op_binary(Token::TokenType token);
    bool op_unary(Token::TokenType token);
    int get_precedence(Expression &e);
    bool right_assoc(Node &n);
    int op_precedence[200];
    Utils(void);
};

#endif // __UTILS_