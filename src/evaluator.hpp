#if !defined(__EVALUATOR_)
#define __EVALUATOR_

#include "CVM.hpp"
#include "AST.hpp"
#include "token.hpp"
#include "utils.hpp"

#include <cstdint>
#include <vector>

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
    RpnElement(const Operator &_op) : type(OPERATOR), op(_op) {};
    RpnElement(const Value &val) : type(VALUE), value(val) {};
};

typedef std::vector<RpnElement> RpnStack;

class Evaluator {
  private:
    NativeFunction *native_bind = nullptr;
  public:
    CVM &VM;
    const Node &AST;
    Utils &utils;
    CallStack stack;
    Evaluator(const Node &_AST, CVM &_VM, Utils &_utils) : 
      VM(_VM),
      AST(_AST), 
      utils(_utils) {};
    void start();
  private:
    bool inside_func = false;
    bool returns_ref = false;
    int nested_loops = 0;
    std::uint64_t current_line = 0;
    std::string *current_source = nullptr;
    void throw_error(const std::string &cause);
    int execute_statement(const Node &statement);
    Value evaluate_expression(const NodeList &expression_tree, const bool get_ref = false);
    void declare_variable(const Node &declaration);
    void register_class(const ClassStatement &_class);
    void flatten_tree(RpnStack &res, const NodeList &expression_tree);
    void node_to_element(const Node &node, RpnStack &container);
    std::shared_ptr<Variable> get_reference_by_name(const std::string &name);
    Value reduce_rpn(RpnStack &stack);
    std::string stringify(const Value &val);
    inline double to_double(const Value &val);
    const Value &get_value(const RpnElement &el);
    Value &get_mut_value(RpnElement &el);
    Value &get_heap_value(std::int64_t ref);
    void set_member(const std::vector<std::string> &members, const NodeList &expression);
    void set_index(const Statement &stmt);

    // Unary
    RpnElement logical_not(const RpnElement &x);
    RpnElement bitwise_not(const RpnElement &x);
    RpnElement delete_value(const RpnElement &x);

    // Binary
    // math operations
    RpnElement perform_addition(const RpnElement &x, const RpnElement &y);
    RpnElement perform_subtraction(const RpnElement &x, const RpnElement &y);
    RpnElement perform_multiplication(const RpnElement &x, const RpnElement &y);
    RpnElement perform_division(const RpnElement &x, const RpnElement &y);
    RpnElement perform_modulo(const RpnElement &x, const RpnElement &y);
    // bitwise operations
    RpnElement bitwise_and(const RpnElement &x, const RpnElement &y);
    RpnElement bitwise_or(const RpnElement &x, const RpnElement &y);
    RpnElement bitwise_xor(const RpnElement &x, const RpnElement &y);
    RpnElement shift_left(const RpnElement &x, const RpnElement &y);
    RpnElement shift_right(const RpnElement &x, const RpnElement &y);
    // logical operations
    RpnElement logical_and(const RpnElement &x, const RpnElement &y);
    RpnElement logical_or(const RpnElement &x, const RpnElement &y);
    // assignments
    RpnElement assign(RpnElement &x, const RpnElement &y);
    RpnElement plus_assign(RpnElement &x, const RpnElement &y);
    RpnElement minus_assign(RpnElement &x, const RpnElement &y);
    RpnElement mul_assign(RpnElement &x, const RpnElement &y);
    RpnElement div_assign(RpnElement &x, const RpnElement &y);
    RpnElement lshift_assign(RpnElement &x, const RpnElement &y);
    RpnElement rshift_assign(RpnElement &x, const RpnElement &y);
    RpnElement and_assign(RpnElement &x, const RpnElement &y);
    RpnElement or_assign(RpnElement &x, const RpnElement &y);
    RpnElement xor_assign(RpnElement &x, const RpnElement &y);
    RpnElement mod_assign(RpnElement &x, const RpnElement &y);
    // comparators
    RpnElement compare_eq(const RpnElement &x, const RpnElement &y);
    RpnElement compare_neq(const RpnElement &x, const RpnElement &y);
    RpnElement compare_gt(const RpnElement &x, const RpnElement &y);
    RpnElement compare_lt(const RpnElement &x, const RpnElement &y);
    RpnElement compare_gt_eq(const RpnElement &x, const RpnElement &y);
    RpnElement compare_lt_eq(const RpnElement &x, const RpnElement &y);
    // functions
    RpnElement execute_function(RpnElement &fn, const RpnElement &call);
    // misc
    RpnElement access_member(RpnElement &x, const RpnElement &y);
    RpnElement access_index(RpnElement &arr, const RpnElement &idx);
    RpnElement construct_object(const RpnElement &call, const RpnElement &_class);

    Value return_value;
};

#endif // __EVALUATOR_