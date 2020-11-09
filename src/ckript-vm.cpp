#include "ckript-vm.hpp"
#include "utils.hpp"
#include "error-handler.hpp"

#include <cassert>
#include <iostream>

bool Value::is_lvalue() {
  return reference_name.size() != 0;
}

bool Variable::is_allocated() const {
  return val.heap_reference != -1;
}

Chunk &Heap::allocate() {
  std::int64_t index = 0;
  // try to find a free chunk
  for (auto &chunk : chunks) {
    if (!chunk.used) {
      chunk.used = true;
      chunk.data = new Value;
      chunk.heap_reference = index;
      return chunk;
    }
    index++;
  }
  // add a new chunk
  chunks.push_back(Chunk());
  Chunk &chunk_ref = chunks.back();
  chunk_ref.used = true;
  chunk_ref.data = new Value;
  chunk_ref.heap_reference = chunks.size() - 1;
  return chunk_ref;
}

void Heap::free(Variable *var) {
  // check for all possible errors
  assert(var->is_allocated());
  assert(var->val.heap_reference < chunks.size());
  Chunk &chunk = chunks.at(var->val.heap_reference);
  assert(chunk.used == true);
  assert(chunk.data != nullptr);
  std::cout << "(heap) deleting " << var->val.heap_reference << "\n";
  delete chunk.data;
  chunk.data = nullptr;
  chunk.used = false;
  chunk.heap_reference = -1;
  var->val.heap_reference = -1;
  // shrink the heap if possible
  while (chunks.size() && !chunks.back().used) {
    chunks.pop_back();
  }
}

// stdlib

class NativeInput : public NativeFunction {
  public:
    Value execute(std::vector<Value> &args, std::int64_t line) {
      if (args.size() != 0) {
        ErrorHandler::throw_runtime_error("input() doesn't take any arguments", line);
      }
      Value str;
      str.type = Utils::VarType::STR;
      str.string_value = "";
      std::getline(std::cin, str.string_value);
      return str;
    }
};

class NativePrint : public NativeFunction {
  public:
    Value execute(std::vector<Value> &args, std::int64_t line) {
      if (args.size() != 1 || args.at(0).type != Utils::VarType::STR) {
        ErrorHandler::throw_runtime_error("print() expects one argument (str)", line);
      }
      std::cout << args.at(0).string_value;
      Value val(Utils::VarType::VOID);
      return {val};
    }
};

class NativeTostr : public NativeFunction {
  public:
    Value execute(std::vector<Value> &args, std::int64_t line) {
      if (args.size() != 1) {
        ErrorHandler::throw_runtime_error("to_str() expects one argument", line);
      }
      
      Value val(Utils::VarType::STR);
      val.string_value = "";
      return {val};
    }
};

void CkriptVM::load_stdlib(void) {
  NativeInput *input = new NativeInput;
  NativeFunction *input_ptr = input;
  globals.insert(std::make_pair("input", input_ptr));
  NativePrint *print = new NativePrint;
  NativeFunction *print_ptr = print;
  globals.insert(std::make_pair("print", print_ptr));
}