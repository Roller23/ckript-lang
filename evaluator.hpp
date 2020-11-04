#if !defined(__EVALUATOR_)
#define __EVALUATOR_

#include "ckript-vm.hpp"
#include "AST.hpp"
#include "token.hpp"

#include <cstdint>
#include <vector>

class Value {
  public:
    std::string type;
    bool boolean_value = 0;
    double float_value = 0;
    std::string string_value;
    std::uint64_t number_value = 0;
    Variable *reference = nullptr;
    bool is_lvalue();
};

class Operator {
  public:
    Token::TokenType type;
    Operator(void) : type(Token::TokenType::UNKNOWN) {};
    Operator(Token::TokenType _type) : type(_type) {};
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
    Evaluator(Node &_AST, CkriptVM &_VM) : VM(_VM), AST(_AST) {};
    void start();
  private:
    void execute_statement(Node &statement);
    Value evaluate_expression(NodeList &expression_tree);
    void declare_variable(Node &declaration);
    void flatten_tree(RpnStack &res, NodeList &expression_tree);
    RpnElement node_to_element(Node &node);
    Value reduce_rpn(RpnStack &stack);
};

#endif // __EVALUATOR_