#if !defined(__CVM_)
#define __CVM_

#include <map>
#include <vector>
#include <string>

#include "utils.hpp"
#include "AST.hpp"

// Ckript Virtual Machine

class Value {
  public:
    Utils::VarType type;
    bool boolean_value = false;
    double float_value = 0.0;
    std::string string_value = "";
    std::int64_t number_value = 0;
    std::string reference_name = "";
    std::int64_t heap_reference = -1;
    std::int64_t this_ref = -1;
    FuncExpression func;
    ParamList members;
    std::map<std::string, Value> member_values;
    std::vector<Value> array_values;
    std::string array_type = "int";
    std::string class_name = "";
    std::string member_name = "";
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
    void free(std::int64_t ref);
};

typedef std::vector<Variable *> CallStack;

class NativeFunction;

class CVM {
  public:
    void load_stdlib(void);
    std::map<std::string, NativeFunction *> globals;
    Heap heap;
};

class NativeFunction {
  public:
    virtual Value execute(std::vector<Value> &args, std::int64_t line, CVM &VM) = 0;
};

#endif // __CVM_