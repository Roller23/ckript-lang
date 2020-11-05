#if !defined(__VALUE_)
#define __VALUE_

#include "utils.hpp"
#include "AST.hpp"
#include "variable.hpp"

class Value {
  public:
    Utils::VarType type;
    bool boolean_value = 0;
    double float_value = 0;
    std::string string_value;
    std::int64_t number_value = 0;
    bool is_neg = false;
    Variable *reference = nullptr;
    std::string reference_name = "";
    FuncExpression func;
    bool is_lvalue();
    Value(void) : type(Utils::UNKNOWN) {};
    Value(Utils::VarType _type) : type(_type) {};
};

#endif // __VALUE_