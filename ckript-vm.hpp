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
    bool is_neg = false;
    std::string reference_name = "";
    FuncExpression func;
    bool is_lvalue();
    Value(void) : type(Utils::UNKNOWN) {};
    Value(Utils::VarType _type) : type(_type) {};
};

class Variable {
  public:
    std::string id;
    std::string type;
    std::int64_t heap_reference = -1; // its own reference on heap
    std::int64_t var_reference = -1; // reference to another variable on heap
    Value val;
    bool is_allocated() const;
    bool is_reference() const;
};

class Chunk {
  public:
    Variable *data = nullptr;
    bool used = false;
};

class Heap {
  public:
    std::vector<Chunk> chunks;
    Chunk &allocate();
    void free(Variable *var);
};

class CkriptVM {
  public:
    std::map<std::string, Variable *> globals;
    Heap heap;
    std::vector <Variable *> stack;
};

#endif // __CKRIPT_VM_