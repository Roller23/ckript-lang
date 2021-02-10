#if !defined(__CVM_)
#define __CVM_

#include <map>
#include <unordered_map>
#include <vector>
#include <string>
#include <cstring>
#include <memory>

#include "utils.hpp"
#include "AST.hpp"

// Ckript Virtual Machine

class Value {
  public:
    Utils::VarType type = Utils::UNKNOWN;
    bool boolean_value = false;
    double float_value = 0.0;
    std::string string_value = "";
    std::int64_t number_value = 0;
    std::string reference_name = "";
    std::int64_t heap_reference = -1;
    std::int64_t this_ref = -1;
    FuncExpression func;
    std::string func_name = "";
    ParamList members;
    std::map<std::string, Value> member_values;
    std::vector<Value> array_values;
    std::string array_type = "int";
    std::string class_name = "";
    std::string member_name = "";
    bool is_lvalue() const;
    Value(void) : type(Utils::UNKNOWN) {};
    Value(Utils::VarType _type) : type(_type) {};
    Value(const FuncExpression &fn) : type(Utils::FUNC), func(fn) {}
};

class Variable {
  public:
    std::string type;
    Value val;
    bool constant = false;
    bool is_allocated() const;
};

class Chunk {
  public:
    Value *data = nullptr;
    std::int64_t heap_reference = -1;
    bool used = false;
};

class Cache {
  private:
    static const int CACHE_SIZE = 10000;
    int index = -1;
  public:
    std::vector<std::int64_t> refs;
    void push(std::int64_t ref);
    std::int64_t pop(void);
    Cache(void) {
      refs.resize(CACHE_SIZE);
    }
};

class Heap {
  public:
    std::vector<Chunk> chunks;
    Cache cache;
    Chunk &allocate();
    void free(std::int64_t ref);
};

typedef std::unordered_map<std::string, std::shared_ptr<Variable>> CallStack;

class Call {
  public:
    std::uint64_t line;
    std::string name;
    std::string *source;
    Call(const std::uint64_t &__l, const std::string &__n, std::string *&__s) {
      this->line = __l;
      this->name = __n;
      this->source = __s;
    }
};

class StackTrace {
  public:
    std::vector<Call> stack;
    void pop(void);
    void push(const std::string &_name, const std::uint64_t &_line, std::string *&_source);
    StackTrace(void) {
      stack.reserve(1000);
    }
};

class NativeFunction;

class CVM {
  private:
    void load_stdlib(void);
  public:
    std::string stringify(Value &val);
    std::unordered_map<std::string, NativeFunction *> globals;
    Heap heap;
    StackTrace trace;
    CVM(void) {
      load_stdlib();
    }
};

class NativeFunction {
  public:
    virtual Value execute(std::vector<Value> &args, std::int64_t line, CVM &VM) = 0;
};

#endif // __CVM_