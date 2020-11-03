#if !defined(__CKRIPT_VM_)
#define __CKRIPT_VM_

#include <map>
#include <vector>
#include <string>

class Variable {
  public:
    const std::string id;
    const std::string type;
    std::int64_t heap_reference = -1; // its own reference on heap
    std::int64_t var_reference = -1; // reference to another variable on heap
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
};

#endif // __CKRIPT_VM_