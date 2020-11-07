#if !defined(__UTILS_)
#define __UTILS_

#include "token.hpp"
#include "AST.hpp"

#include <map>

class Utils {
  public:
    typedef enum var_type {
      INT, FLOAT, STR, ARR, OBJ, BOOL, FUNC, REF, ID, VOID, UNKNOWN
    } VarType;
    bool op_binary(Token::TokenType token);
    bool op_unary(Token::TokenType token);
    int get_precedence(Expression &e);
    bool right_assoc(Node &n);
    std::map<std::string, VarType> var_lut;
    std::map<Token::TokenType, int> op_precedence;
    bool has_key(Token::TokenType key);
    Utils(void);
};

#endif // __UTILS_