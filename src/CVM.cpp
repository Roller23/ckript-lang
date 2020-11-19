#include "CVM.hpp"
#include "utils.hpp"
#include "error-handler.hpp"

#include <cassert>
#include <iostream>
#include <fstream>
#include <streambuf>
#include <sstream>
#include <chrono>
#include <ctime>
#include <cmath>
#include <cstdlib>
#include <random>
#include <algorithm>
#include <cstring>

#define REG_FN(name, fn)\
  class name : public NativeFunction {\
    public:\
      Value execute(std::vector<Value> &args, std::int64_t line, CVM &VM) {\
        if (args.size() != 1 || args.at(0).type != Utils::FLOAT && args.at(0).type != Utils::INT) {\
          ErrorHandler::throw_runtime_error(#fn "() expects one argument (double|int)", line);\
        }\
        Value val(Utils::FLOAT);\
        double arg = args.at(0).float_value;\
        if (args.at(0).type == Utils::INT) arg = (double)args.at(0).number_value;\
        val.float_value = std::fn(arg);\
        return val;\
      }\
  };

#define ADD_FN(name, fn)\
  NativeFunction *fn = new name;\
  globals.insert(std::make_pair(#fn, fn));

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

void Heap::free(std::int64_t ref) {
  // check for all possible errors
  assert(ref < chunks.size());
  Chunk &chunk = chunks.at(ref);
  assert(chunk.used == true);
  assert(chunk.data != nullptr);
  delete chunk.data;
  chunk.data = nullptr;
  chunk.used = false;
  chunk.heap_reference = -1;
  // shrink the heap if possible
  while (chunks.size() && !chunks.back().used) {
    chunks.pop_back();
  }
}

// stdlib

static std::string stringify(Value &val) {
  if (val.type == Utils::STR) {
    return val.string_value;
  }
  if (val.type == Utils::INT) {
    return std::to_string(val.number_value);
  }
  if (val.type == Utils::FLOAT) {
    return std::to_string(val.float_value);
  }
  if (val.type == Utils::FUNC) {
    return "function";
  }
  if (val.type == Utils::BOOL) {
    return val.boolean_value ? "true" : "false";
  }
  if (val.type == Utils::CLASS) {
    return "class " + val.class_name;
  }
  if (val.type == Utils::VOID) {
    return "void";
  }
  if (val.type == Utils::UNKNOWN) {
    return "null";
  }
  if (val.type == Utils::ARR) {
    std::string str = "array(";
    int i = 0;
    for (auto &el : val.array_values) {
      if (el.type == Utils::STR) str += "\"";
      str += stringify(el);
      if (el.type == Utils::STR) str += "\"";
      if (i != val.array_values.size() - 1) {
        str += ", ";
      }
      i++;
    }
    str += ")";
    return str;
  }
  if (val.type == Utils::OBJ) {
    std::string str = "object(";
    int i = 0;
    for (auto &member : val.member_values) {
      str += member.first + ": ";
      if (member.second.type == Utils::STR) str += "\"";
      str += stringify(member.second);
      if (member.second.type == Utils::STR) str += "\"";
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
    Value execute(std::vector<Value> &args, std::int64_t line, CVM &VM) {
      if (args.size() != 0) {
        ErrorHandler::throw_runtime_error("input() doesn't take any arguments", line);
      }
      Value str;
      str.type = Utils::STR;
      str.string_value = "";
      std::getline(std::cin, str.string_value);
      return str;
    }
};

class NativePrint : public NativeFunction {
  public:
    Value execute(std::vector<Value> &args, std::int64_t line, CVM &VM) {
      if (args.size() == 0) {
        ErrorHandler::throw_runtime_error("print() expects at least one argument", line);
      }
      int i = 0;
      for (auto &arg : args) {
        std::cout << stringify(arg);
        if (i != args.size() - 1) std::cout << " ";
        i++;
      }
      Value val(Utils::VOID);
      return val;
    }
};

class NativePrintln : public NativeFunction {
  public:
    Value execute(std::vector<Value> &args, std::int64_t line, CVM &VM) {
      int i = 0;
      for (auto &arg : args) {
        std::cout << stringify(arg);
        if (i != args.size() - 1) std::cout << " ";
        i++;
      }
      std::cout << std::endl;
      Value val(Utils::VOID);
      return val;
    }
};

class NativeFlush : public NativeFunction {
  public:
    Value execute(std::vector<Value> &args, std::int64_t line, CVM &VM) {
      if (args.size() != 0) {
        ErrorHandler::throw_runtime_error("flush() takes no arguments", line);
      }
      std::cout << std::flush;
      Value val(Utils::VOID);
      return val;
    }
};

class NativeSize : public NativeFunction {
  public:
    Value execute(std::vector<Value> &args, std::int64_t line, CVM &VM) {
      if (args.size() != 1) {
        ErrorHandler::throw_runtime_error("size() expects one argument", line);
      }
      Value &arg = args.at(0);
      Value val(Utils::INT);
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
    Value execute(std::vector<Value> &args, std::int64_t line, CVM &VM) {
      if (args.size() != 1) {
        ErrorHandler::throw_runtime_error("to_str() expects one argument", line);
      }
      Value val(Utils::STR);
      val.string_value = stringify(args.at(0));
      return val;
    }
};

class NativeToint : public NativeFunction {
  public:
    Value execute(std::vector<Value> &args, std::int64_t line, CVM &VM) {
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
    Value execute(std::vector<Value> &args, std::int64_t line, CVM &VM) {
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
    Value execute(std::vector<Value> &args, std::int64_t line, CVM &VM) {
      if (args.size() != 1 || args.at(0).type != Utils::INT) {
        ErrorHandler::throw_runtime_error("exit() expects one argument (int)", line);
      }
      std::exit(args.at(0).number_value);
      return {};
    }
};

class NativeTimestamp : public NativeFunction {
  public:
    Value execute(std::vector<Value> &args, std::int64_t line, CVM &VM) {
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

class NativePow : public NativeFunction {
  public:
    Value execute(std::vector<Value> &args, std::int64_t line, CVM &VM) {
      if (args.size() != 2) {
        ErrorHandler::throw_runtime_error("pow() expects two arguments (double|int, double|int)", line);
      }
      if (!(args.at(0).type == Utils::FLOAT || args.at(0).type == Utils::INT)
       || !(args.at(1).type == Utils::FLOAT || args.at(1).type == Utils::INT)) {
         ErrorHandler::throw_runtime_error("pow() arguments must be either int or double", line);
      }
      Value val(Utils::FLOAT);
      double arg1 = args.at(0).float_value;
      double arg2 = args.at(1).float_value;
      if (args.at(0).type == Utils::INT) arg1 = (double)args.at(0).number_value;
      if (args.at(1).type == Utils::INT) arg2 = (double)args.at(1).number_value;
      val.float_value = std::pow(arg1, arg2);
      return val;
    }
};

class NativeFileread : public NativeFunction {
  public:
    Value execute(std::vector<Value> &args, std::int64_t line, CVM &VM) {
      if (args.size() != 1 || args.at(0).type != Utils::STR) {
        ErrorHandler::throw_runtime_error("file_read() expects one argument (str)", line);
      }
      Value val(Utils::STR);
      std::ifstream f(args.at(0).string_value);
      if (!f.good()) {
        ErrorHandler::throw_runtime_error("couldn't read " + args.at(0).string_value, line);
      }
      std::stringstream buffer;
      buffer << f.rdbuf();
      val.string_value = buffer.str();
      return val;
    }
};

class NativeFilewrite : public NativeFunction {
  public:
    Value execute(std::vector<Value> &args, std::int64_t line, CVM &VM) {
      if (args.size() != 2 || args.at(0).type != Utils::STR || args.at(1).type != Utils::STR) {
        ErrorHandler::throw_runtime_error("file_write() expects two arguments (str, str)", line);
      }
      Value val(Utils::BOOL);
      std::ofstream f(args.at(0).string_value);
      if (!f.good()) {
        val.boolean_value = false;
        return val;
      }
      f << args.at(1).string_value;
      f.close();
      val.boolean_value = true;
      return val;
    }
};

class NativeFileexists : public NativeFunction {
  public:
    Value execute(std::vector<Value> &args, std::int64_t line, CVM &VM) {
      if (args.size() != 1 || args.at(0).type != Utils::STR) {
        ErrorHandler::throw_runtime_error("file_exists() expects one argument (str)", line);
      }
      Value val(Utils::BOOL);
      std::ifstream f(args.at(0).string_value);
      val.boolean_value = f.good();
      return val;
    }
};

class NativeFileremove : public NativeFunction {
  public:
    Value execute(std::vector<Value> &args, std::int64_t line, CVM &VM) {
      if (args.size() != 1 || args.at(0).type != Utils::STR) {
        ErrorHandler::throw_runtime_error("file_remove() expects one argument (str)", line);
      }
      Value val(Utils::BOOL);
      val.boolean_value = std::remove(args.at(0).string_value.c_str()) == 0;
      return val;
    }
};

class NativeAbs : public NativeFunction {
  public:
    Value execute(std::vector<Value> &args, std::int64_t line, CVM &VM) {
      if (args.size() != 1 || (args.at(0).type != Utils::INT && args.at(0).type != Utils::FLOAT)) {
        ErrorHandler::throw_runtime_error("abs() expects one argument (int|double)", line);
      }
      Value val;
      val.type = args.at(0).type;
      if (args.at(0).type == Utils::INT) {
        val.number_value = args.at(0).number_value;
        if (val.number_value < 0) {
          val.number_value = -val.number_value;
        }
      } else {
        val.float_value = args.at(0).float_value;
        if (val.float_value < 0) {
          val.float_value = -val.float_value;
        }
      }
      return val;
    }
};

class NativeRand : public NativeFunction {
  public:
    Value execute(std::vector<Value> &args, std::int64_t line, CVM &VM) {
      if (args.size() != 2 || args.at(0).type != Utils::INT || args.at(1).type != Utils::INT) {
        ErrorHandler::throw_runtime_error("rand() expects two arguments (int, int)", line);
      }
      Value val(Utils::INT);
      std::random_device rd;
      std::default_random_engine generator(rd());
      std::uniform_int_distribution<std::int64_t> distribution(
        args.at(0).number_value, args.at(1).number_value
      );
      val.number_value = distribution(generator);
      return val;
    }
};

class NativeRandf : public NativeFunction {
  public:
    Value execute(std::vector<Value> &args, std::int64_t line, CVM &VM) {
      if (args.size() != 2 || args.at(0).type != Utils::FLOAT || args.at(1).type != Utils::FLOAT) {
        ErrorHandler::throw_runtime_error("randf() expects two arguments (double, double)", line);
      }
      Value val(Utils::FLOAT);
      std::random_device rd;
      std::default_random_engine generator(rd());
      std::uniform_real_distribution<double> distribution(
        args.at(0).float_value, args.at(1).float_value
      );
      val.float_value = distribution(generator);
      return val;
    }
};

class NativeContains : public NativeFunction {
  public:
    Value execute(std::vector<Value> &args, std::int64_t line, CVM &VM) {
      if (args.size() != 2 || args.at(0).type != Utils::STR || args.at(1).type != Utils::STR) {
        ErrorHandler::throw_runtime_error("contains() expects two arguments (str, str)", line);
      }
      Value val(Utils::BOOL);
      val.boolean_value = args.at(0).string_value.find(args.at(1).string_value) != std::string::npos;
      return val;
    }
};

class NativeSubstr : public NativeFunction {
  public:
    Value execute(std::vector<Value> &args, std::int64_t line, CVM &VM) {
      if (args.size() != 3 || args.at(0).type != Utils::STR || args.at(1).type != Utils::INT || args.at(2).type != Utils::INT) {
        ErrorHandler::throw_runtime_error("substr() expects three arguments (str, int, int)", line);
      }
      if (args.at(1).number_value < 0) {
        ErrorHandler::throw_runtime_error("starting position cannot be negative", line);
      }
      if (args.at(2).number_value < 0) {
        ErrorHandler::throw_runtime_error("length cannot be negative", line);
      }
      if (args.at(1).number_value + args.at(2).number_value > args.at(0).string_value.size()) {
        ErrorHandler::throw_runtime_error("out of string range", line);
      }
      Value val(Utils::STR);
      val.string_value = args.at(0).string_value.substr(args.at(1).number_value, args.at(2).number_value);
      return val;
    }
};

class NativeSplit : public NativeFunction {
  public:
    Value execute(std::vector<Value> &args, std::int64_t line, CVM &VM) {
      if (args.size() != 2 || args.at(0).type != Utils::STR || args.at(1).type != Utils::STR) {
        ErrorHandler::throw_runtime_error("split() expects two arguments (str, str)", line);
      }
      Value res(Utils::ARR);
      res.array_type = "str";
      std::string delim_copy = args.at(1).string_value;
      char *c_str = strdup(args.at(0).string_value.c_str());
      const char *c_delim = delim_copy.c_str();
      char *token = std::strtok(c_str, c_delim);
      while (token != NULL) {
        Value element;
        element.type = Utils::STR;
        element.string_value = token;
        res.array_values.push_back(element);
        token = std::strtok(NULL, c_delim);
      }
      std::free(c_str);
      return res;
    }
};

class NativeTobytes : public NativeFunction {
  public:
    Value execute(std::vector<Value> &args, std::int64_t line, CVM &VM) {
      if (args.size() != 1 || args.at(0).type != Utils::STR) {
        ErrorHandler::throw_runtime_error("to_bytes() expects one argument (str)", line);
      }
      Value res(Utils::ARR);
      res.array_type = "int";
      const char *c_str = args.at(0).string_value.c_str();
      int i = 0;
      while (c_str[i]) {
        Value element;
        element.type = Utils::INT;
        element.number_value = (std::int64_t)c_str[i++];
        res.array_values.push_back(element);
      }
      return res;
    }
};

class NativeFrombytes : public NativeFunction {
  public:
    Value execute(std::vector<Value> &args, std::int64_t line, CVM &VM) {
      if (args.size() != 1 || args.at(0).type != Utils::ARR) {
        ErrorHandler::throw_runtime_error("to_bytes() expects one argument (arr)", line);
      }
      if (args.at(0).array_type != "int") {
        ErrorHandler::throw_runtime_error("to_bytes() expects an int array", line);
      }
      Value res(Utils::STR);
      res.string_value = "";
      for (auto &el : args.at(0).array_values) {
        res.string_value += (char)el.number_value;
      }
      return res;
    }
};

class NativeBind : public NativeFunction {
  public:
    Value execute(std::vector<Value> &args, std::int64_t line, CVM &VM) {
      if (args.size() != 1 || args.at(0).heap_reference == -1) {
        ErrorHandler::throw_runtime_error("to_bytes() expects one argument (ref obj)", line);
      }
      std::int64_t ref = args.at(0).heap_reference;
      if (ref < 0 || ref >= VM.heap.chunks.size()) {
        ErrorHandler::throw_runtime_error("dereferencing a value that is not on the heap");
      }
      Value *ptr = VM.heap.chunks.at(ref).data;
      if (ptr == nullptr) {
        ErrorHandler::throw_runtime_error("dereferencing a null pointer");
      }
      if (ptr->type != Utils::OBJ) {
        ErrorHandler::throw_runtime_error("Can only bind a reference");
      }
      for (auto &pair : ptr->member_values) {
        Value *v = &pair.second;
        if (v->heap_reference != -1) {
          v = VM.heap.chunks.at(v->heap_reference).data;
        }
        if (v == nullptr) {
          ErrorHandler::throw_runtime_error("dereferencing a null pointer");
        }
        if (v->type == Utils::FUNC) {
          v->this_ref = ref;
        }
      }
      Value res(Utils::VOID);
      return res;
    }
};

// used only for math functions

REG_FN(NativeSin, sin)
REG_FN(NativeSinh, sinh)
REG_FN(NativeCos, cos)
REG_FN(NativeCosh, cosh)
REG_FN(NativeTan, tan)
REG_FN(NativeTanh, tanh)
REG_FN(NativeSqrt, sqrt)
REG_FN(NativeLog, log)
REG_FN(NativeLogten, log10)
REG_FN(NativeExp, exp)
REG_FN(NativeFloor, floor)
REG_FN(NativeCeil, ceil)
REG_FN(NativeRound, round)

void CVM::load_stdlib(void) {
  // allocate 
  ADD_FN(NativeTimestamp, timestamp)
  ADD_FN(NativeInput, input)
  ADD_FN(NativePrint, print)
  ADD_FN(NativePrintln, println)
  ADD_FN(NativeFlush, flush)
  ADD_FN(NativeTostr, to_str)
  ADD_FN(NativeExit, exit)
  ADD_FN(NativeToint, to_int)
  ADD_FN(NativeTodouble, to_double)
  ADD_FN(NativeSize, size)
  ADD_FN(NativeSin, sin)
  ADD_FN(NativeSinh, sinh)
  ADD_FN(NativeCos, cos)
  ADD_FN(NativeCosh, cosh)
  ADD_FN(NativeTan, tan)
  ADD_FN(NativeTanh, tanh)
  ADD_FN(NativeSqrt, sqrt)
  ADD_FN(NativeLog, log)
  ADD_FN(NativeLogten, log10)
  ADD_FN(NativeExp, exp)
  ADD_FN(NativeFloor, floor)
  ADD_FN(NativeCeil, ceil)
  ADD_FN(NativeRound, round)
  ADD_FN(NativePow, pow)
  ADD_FN(NativeFileread, file_read)
  ADD_FN(NativeFilewrite, file_write)
  ADD_FN(NativeFileexists, file_exists)
  ADD_FN(NativeFileremove, file_remove)
  ADD_FN(NativeAbs, abs)
  ADD_FN(NativeRand, rand)
  ADD_FN(NativeRandf, randf)
  ADD_FN(NativeContains, contains)
  ADD_FN(NativeSubstr, substr)
  ADD_FN(NativeSplit, split)
  ADD_FN(NativeTobytes, to_bytes)
  ADD_FN(NativeFrombytes, from_bytes)
  ADD_FN(NativeBind, bind);
}