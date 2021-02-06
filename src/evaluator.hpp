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
    RpnElement(Operator _op) : type(OPERATOR), op(_op) {};
    RpnElement(Value &val) : type(VALUE), value(val) {};
};

typedef std::vector<RpnElement> RpnStack;

class Evaluator {
  private:
    NativeFunction *native_bind = nullptr;
    NativeFunction *native_println = nullptr;
    NativeFunction *native_stacktrace = nullptr;
  public:
    CVM &VM;
    Node &AST;
    Utils &utils;
    bool stream = false;
    CallStack stack;
    Evaluator(Node &_AST, CVM &_VM, Utils &_utils, bool is_stream = false) : 
      VM(_VM),
      AST(_AST), 
      utils(_utils),
      stream(is_stream) {
        native_bind = VM.globals.at("bind");
        native_println = VM.globals.at("println");
        native_stacktrace = VM.globals.at("stack_trace");
      };
    void start();
  private:
    bool inside_func = false;
    bool returns_ref = false;
    int nested_loops = 0;
    std::uint64_t current_line = 0;
    std::string *current_source = nullptr;
    void throw_error(const std::string &cause);
    int execute_statement(Node &statement);
    Value evaluate_expression(const NodeList &expression_tree, bool get_ref = false);
    void declare_variable(Node &declaration);
    void register_class(ClassStatement &_class);
    void flatten_tree(RpnStack &res, const NodeList &expression_tree);
    RpnElement node_to_element(const Node &node);
    Variable *get_reference_by_name(const std::string &name);
    Value reduce_rpn(RpnStack &stack);
    std::string stringify(Value &val);
    inline double to_double(Value &val);
    Value &get_value(RpnElement &el);
    Value &get_heap_value(std::int64_t ref);
    void set_member(const std::vector<std::string> &members, NodeList &expression);
    void set_index(Statement &stmt);

    // Unary
    RpnElement logical_not(RpnElement &x);
    RpnElement bitwise_not(RpnElement &x);
    RpnElement delete_value(RpnElement &x);

    // Binary
    // math operations
    RpnElement perform_addition(RpnElement &x, RpnElement &y);
    RpnElement perform_subtraction(RpnElement &x, RpnElement &y);
    RpnElement perform_multiplication(RpnElement &x, RpnElement &y);
    RpnElement perform_division(RpnElement &x, RpnElement &y);
    RpnElement perform_modulo(RpnElement &x, RpnElement &y);
    // bitwise operations
    RpnElement bitwise_and(RpnElement &x, RpnElement &y);
    RpnElement bitwise_or(RpnElement &x, RpnElement &y);
    RpnElement bitwise_xor(RpnElement &x, RpnElement &y);
    RpnElement shift_left(RpnElement &x, RpnElement &y);
    RpnElement shift_right(RpnElement &x, RpnElement &y);
    // logical operations
    RpnElement logical_and(RpnElement &x, RpnElement &y);
    RpnElement logical_or(RpnElement &x, RpnElement &y);
    // assignments
    RpnElement assign(RpnElement &x, RpnElement &y);
    RpnElement plus_assign(RpnElement &x, RpnElement &y);
    RpnElement minus_assign(RpnElement &x, RpnElement &y);
    RpnElement mul_assign(RpnElement &x, RpnElement &y);
    RpnElement div_assign(RpnElement &x, RpnElement &y);
    RpnElement lshift_assign(RpnElement &x, RpnElement &y);
    RpnElement rshift_assign(RpnElement &x, RpnElement &y);
    RpnElement and_assign(RpnElement &x, RpnElement &y);
    RpnElement or_assign(RpnElement &x, RpnElement &y);
    RpnElement xor_assign(RpnElement &x, RpnElement &y);
    RpnElement mod_assign(RpnElement &x, RpnElement &y);
    // comparators
    RpnElement compare_eq(RpnElement &x, RpnElement &y);
    RpnElement compare_neq(RpnElement &x, RpnElement &y);
    RpnElement compare_gt(RpnElement &x, RpnElement &y);
    RpnElement compare_lt(RpnElement &x, RpnElement &y);
    RpnElement compare_gt_eq(RpnElement &x, RpnElement &y);
    RpnElement compare_lt_eq(RpnElement &x, RpnElement &y);
    // functions
    RpnElement execute_function(RpnElement &call, RpnElement &fn);
    // misc
    RpnElement access_member(RpnElement &x, RpnElement &y);
    RpnElement access_index(RpnElement &arr, RpnElement &idx);
    RpnElement construct_object(RpnElement &call, RpnElement &_class);

    Value return_value;
};

#endif // __EVALUATOR_