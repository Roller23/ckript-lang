#if !defined(__EVALUATOR_)
#define __EVALUATOR_

#include "ckript-vm.hpp"
#include "AST.hpp"

class Evaluator {
  public:
    CkriptVM &VM;
    Node &AST;
    Evaluator(Node &_AST, CkriptVM &_VM) : VM(_VM), AST(_AST) {};
    void start();
  private:
    void execute_statement(Node &statement);
    void evaluate_expression(Node &expression);
    void declare_variable(Node &declaration);
};

#endif // __EVALUATOR_