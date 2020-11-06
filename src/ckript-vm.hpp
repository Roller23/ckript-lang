#if !defined(__CKRIPT_VM_)
#define __CKRIPT_VM_

#include <map>
#include <vector>
#include <string>

#include "utils.hpp"
#include "AST.hpp"

class Value {
  public:
    Utils::VarType type;
    bool boolean_value = 0;
    double float_value = 0;
    std::string string_value;
    std::int64_t number_value = 0;
    std::string reference_name = "";
    std::int64_t heap_reference = -1;
    std::int64_t reference = -1;
    FuncExpression func;
    bool is_lvalue();
    Value(void) : type(Utils::UNKNOWN) {};
    Value(Utils::VarType _type) : type(_type) {};
};

class Variable {
  public:
    std::string id;
    std::string type;
    Value val;
    bool constant = false;
    bool is_allocated() const;
    bool is_reference() const;
};

class Chunk {
  public:
    Value *data = nullptr;
    std::int64_t heap_reference;
    bool used = false;
};

class Heap {
  public:
    std::vector<Chunk> chunks;
    Chunk &allocate();
    void free(Variable *var);
};

typedef std::vector<Variable *> CallStack;

class CkriptVM {
  public:
    std::map<std::string, Variable *> globals;
    Heap heap;
    std::vector<CallStack> stacks;
    CallStack &new_callstack();
};

#endif // __CKRIPT_VM_