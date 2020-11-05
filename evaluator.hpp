#if !defined(__EVALUATOR_)
#define __EVALUATOR_

#include "ckript-vm.hpp"
#include "AST.hpp"
#include "token.hpp"
#include "utils.hpp"

#include <cstdint>
#include <vector>

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
    bool is_lvalue();
    Value(void) : type(Utils::UNKNOWN) {};
    Value(Utils::VarType _type) : type(_type) {};
};

class Operator {
  public:
    typedef enum operator_type {
      BASIC, FUNC, INDEX, UNKNOWN
    } OperatorType;
    OperatorType op_type;
    FuncCall func_call;
    NodeList index_rpn;
    Token::TokenType type;
    Operator(void) : op_type(UNKNOWN) {};
    Operator(Token::TokenType _type) : op_type(BASIC), type(_type) {};
    Operator(const FuncCall &call) : op_type(FUNC), func_call(call) {};
    Operator(const NodeList &index) : op_type(INDEX), index_rpn(index) {};
};

class RpnElement {
  public:
    typedef enum element_type {
      OPERATOR, VALUE, UNKNOWN
    } ElementType;
    ElementType type;
    Operator op;
    Value value;
    RpnElement(void) : type(UNKNOWN) {};
    RpnElement(Operator _op) : type(OPERATOR), op(_op) {};
    RpnElement(Value val) : type(VALUE), value(val) {};
};

typedef std::vector<RpnElement> RpnStack;

class Evaluator {
  public:
    CkriptVM &VM;
    Node &AST;
    Utils &utils;
    Evaluator(Node &_AST, CkriptVM &_VM, Utils &_utils) : VM(_VM), AST(_AST), utils(_utils) {};
    void start();
  private:
    void execute_statement(Node &statement);
    Value evaluate_expression(NodeList &expression_tree);
    void declare_variable(Node &declaration);
    void flatten_tree(RpnStack &res, NodeList &expression_tree);
    RpnElement node_to_element(Node &node);
    Variable *get_reference_by_name(const std::string &name);
    Value reduce_rpn(RpnStack &stack);
    std::string stringify(Value &val);
    double to_double(Value &val);
    RpnElement perform_addition(RpnElement &x, RpnElement &y);
    RpnElement perform_subtraction(RpnElement &x, RpnElement &y);
    RpnElement perform_multiplication(RpnElement &x, RpnElement &y);
    RpnElement perform_division(RpnElement &x, RpnElement &y);
};

#endif // __EVALUATOR_