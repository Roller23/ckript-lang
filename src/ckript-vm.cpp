#include "ckript-vm.hpp"
#include "utils.hpp"
#include "error-handler.hpp"

#include <cassert>
#include <iostream>
#include <chrono>
#include <ctime>

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

static std::string stringify(Value &val) {
  if (val.type == Utils::VarType::STR) {
    return val.string_value;
  }
  if (val.type == Utils::VarType::INT) {
    return std::to_string(val.number_value);
  }
  if (val.type == Utils::VarType::FLOAT) {
    return std::to_string(val.float_value);
  }
  if (val.type == Utils::VarType::FUNC) {
    return "function";
  }
  if (val.type == Utils::VarType::BOOL) {
    return val.boolean_value ? "true" : "false";
  }
  if (val.type == Utils::VarType::CLASS) {
    return "class " + val.class_name;
  }
  if (val.type == Utils::VarType::VOID) {
    return "void";
  }
  if (val.type == Utils::VarType::UNKNOWN) {
    return "null";
  }
  if (val.type == Utils::VarType::ARR) {
    std::string str = "array(";
    int i = 0;
    for (auto &el : val.array_values) {
      str += stringify(el);
      if (i != val.array_values.size() - 1) {
        str += ", ";
      }
      i++;
    }
    str += ")";
    return str;
  }
  if (val.type == Utils::VarType::OBJ) {
    std::string str = "object(";
    int i = 0;
    for (auto &member : val.member_values) {
      str += member.first + ": " + stringify(member.second);
      if (i != val.member_values.size() - 1) {
        str += ", ";
      }
      i++;
    }
    str += ")";
    return str;
  }
  return "";
}

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
      return val;
    }
};

class NativeSize : public NativeFunction {
  public:
    Value execute(std::vector<Value> &args, std::int64_t line) {
      if (args.size() != 1) {
        ErrorHandler::throw_runtime_error("size() expects one argument", line);
      }
      Value &arg = args.at(0);
      Value val(Utils::VarType::INT);
      if (arg.type == Utils::ARR) {
        val.number_value = arg.array_values.size();
      } else if (arg.type == Utils::STR) {
        val.number_value = arg.string_value.size();
      } else {
        ErrorHandler::throw_runtime_error("Cannot get the size of " + stringify(arg), line);
      }
      return val;
    }
};

class NativeTostr : public NativeFunction {
  public:
    Value execute(std::vector<Value> &args, std::int64_t line) {
      if (args.size() != 1) {
        ErrorHandler::throw_runtime_error("to_str() expects one argument", line);
      }
      Value val(Utils::VarType::STR);
      val.string_value = stringify(args.at(0));
      return val;
    }
};

class NativeToint : public NativeFunction {
  public:
    Value execute(std::vector<Value> &args, std::int64_t line) {
      if (args.size() != 1) {
        ErrorHandler::throw_runtime_error("to_int() expects one argument", line);
      }
      Value &arg = args.at(0);
      Value val(Utils::INT);
      if (arg.type == Utils::INT) {
        val.number_value = arg.number_value;
      } else if (arg.type == Utils::FLOAT) {
        val.number_value = (std::int64_t)arg.float_value;
      } else if (arg.type == Utils::STR) {
        char *endptr;
        val.number_value = std::strtoll(arg.string_value.c_str(), &endptr, 0);
        if (*endptr != 0) {
          ErrorHandler::throw_runtime_error(arg.string_value + " cannot be converted to int", line);
        }
      } else if (arg.type == Utils::BOOL) {
        val.number_value = (std::int64_t)arg.boolean_value;
      } else {
        ErrorHandler::throw_runtime_error(stringify(arg) + " cannot be converted to int", line);
      }
      return val;
    }
};

class NativeTodouble : public NativeFunction {
  public:
    Value execute(std::vector<Value> &args, std::int64_t line) {
      if (args.size() != 1) {
        ErrorHandler::throw_runtime_error("to_double() expects one argument", line);
      }
      Value &arg = args.at(0);
      Value val(Utils::FLOAT);
      if (arg.type == Utils::INT) {
        val.float_value = (double)arg.number_value;
      } else if (arg.type == Utils::FLOAT) {
        val.float_value = arg.float_value;
      } else if (arg.type == Utils::STR) {
        char *endptr;
        val.float_value = std::strtod(arg.string_value.c_str(), &endptr);
        if (*endptr != 0) {
          ErrorHandler::throw_runtime_error(arg.string_value + " cannot be converted to double", line);
        }
      } else if (arg.type == Utils::BOOL) {
        val.float_value = (double)arg.boolean_value;
      } else {
        ErrorHandler::throw_runtime_error(stringify(arg) + " cannot be converted to double", line);
      }
      return val;
    }
};

class NativeExit : public NativeFunction {
  public:
    Value execute(std::vector<Value> &args, std::int64_t line) {
      if (args.size() != 1 || args.at(0).type != Utils::VarType::INT) {
        ErrorHandler::throw_runtime_error("exit() expects one argument (int)", line);
      }
      std::exit(args.at(0).number_value);
      return {};
    }
};

class NativeTimestamp : public NativeFunction {
  public:
    Value execute(std::vector<Value> &args, std::int64_t line) {
      if (args.size() != 0) {
        ErrorHandler::throw_runtime_error("timestamp() expects no arguments", line);
      }
      Value val(Utils::INT);
      val.number_value = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
      ).count();
      return val;
    }
};

void CkriptVM::load_stdlib(void) {
  NativeInput *input = new NativeInput;
  NativeFunction *input_ptr = input;
  globals.insert(std::make_pair("input", input_ptr));
  NativePrint *print = new NativePrint;
  NativeFunction *print_ptr = print;
  globals.insert(std::make_pair("print", print_ptr));
  NativeTostr *to_str = new NativeTostr;
  NativeFunction *to_str_ptr = to_str;
  globals.insert(std::make_pair("to_str", to_str_ptr));
  NativeExit *exit = new NativeExit;
  NativeFunction *exit_ptr = exit;
  globals.insert(std::make_pair("exit", exit_ptr));
  NativeToint *to_int = new NativeToint;
  NativeFunction *to_int_ptr = to_int;
  globals.insert(std::make_pair("to_int", to_int_ptr));
  NativeTodouble *to_double = new NativeTodouble;
  NativeFunction *to_double_ptr = to_double;
  globals.insert(std::make_pair("to_double", to_double_ptr));
  NativeSize *size = new NativeSize;
  NativeFunction *size_ptr = size;
  globals.insert(std::make_pair("size", size_ptr));
}