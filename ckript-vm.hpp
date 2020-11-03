#if !defined(__CKRIPT_VM_)
#define __CKRIPT_VM_

#include <map>
#include <vector>
#include <string>

class Variable {
  public:
    std::string id; // the identifier
    std::string type;
    bool is_reference;
    std::int64_t heap_reference = -1;
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
    std::map<std::string, Variable> globals;
    Heap heap;
};

#endif // __CKRIPT_VM_