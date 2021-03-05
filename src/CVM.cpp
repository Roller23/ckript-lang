#include "CVM.hpp"
#include "utils.hpp"
#include "error-handler.hpp"

#include <cassert>
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <ctime>
#include <cmath>
#include <cstdlib>
#include <random>
#include <cstring>
#include <thread>

#define REG_FN(name, fn)\
  class name : public NativeFunction {\
    public:\
      Value execute(std::vector<Value> &args, std::int64_t line, CVM &VM) {\
        if (args.size() != 1 || args[0].type != Utils::FLOAT && args[0].type != Utils::INT) {\
          ErrorHandler::throw_runtime_error(#fn "() expects one argument (double|int)", line);\
        }\
        Value val(Utils::FLOAT);\
        double arg = args[0].float_value;\
        if (args[0].type == Utils::INT) arg = (double)args[0].number_value;\
        val.float_value = std::fn(arg);\
        return val;\
      }\
  };

#define ADD_FN(name, fn)\
  NativeFunction *fn = new name;\
  globals.insert(std::make_pair(#fn, fn));


void StackTrace::pop(void) {
  if (stack.size() == 0) return;
  stack.pop_back();
}

void StackTrace::push(const std::string &_name, const std::uint64_t &_line, std::string *&_source) {
  stack.emplace_back(_line, _name, _source);
}

bool Value::is_lvalue() const {
  return reference_name.size() != 0;
}

bool Variable::is_allocated() const {
  return val.heap_reference != -1;
}

void Cache::push(std::int64_t ref) {
  if (index == refs.size() - 2) {
    refs.resize(refs.size() + CACHE_SIZE / 2);
  }
  refs[++index] = ref;
}

std::int64_t Cache::pop(void) {
  if (index == -1) return -1;
  return refs[index--];
}

Chunk &Heap::allocate() {
  std::int64_t index = cache.pop();
  if (index != -1) {
    Chunk &chunk = chunks[index];
    chunk.used = true;
    return chunk;
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
  Chunk &chunk = chunks[ref];
  chunk.used = false;
  cache.push(ref);
}

std::string CVM::stringify(Value &val) {
  if (val.heap_reference != -1) {
    if (val.heap_reference >= this->heap.chunks.size()) {
      return "null";
    }
    Value *ptr = this->heap.chunks[val.heap_reference].data;
    if (ptr == nullptr) {
      return "null";
    } else {
      return "ref to " + stringify(*ptr);
    }
  }
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
    std::string str = "array<" + val.array_type + ">(";
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
    std::string str = "object<" + val.class_name + ">(";
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

// stdlib

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
      std::size_t i = 0;
      const std::size_t end_index = args.size() - 1;
      for (auto &arg : args) {
        std::printf("%s%s", VM.stringify(arg).c_str(), i != end_index ? " " : "");
        i++;
      }
      return {Utils::VOID};
    }
};

class NativePrintln : public NativeFunction {
  public:
    Value execute(std::vector<Value> &args, std::int64_t line, CVM &VM) {
      std::size_t i = 0;
      const std::size_t end_index = args.size() - 1;
      for (auto &arg : args) {
        std::printf("%s%s", VM.stringify(arg).c_str(), i != end_index ? " " : "");
        i++;
      }
      std::printf("\n");
      return {Utils::VOID};
    }
};

class NativeFlush : public NativeFunction {
  public:
    Value execute(std::vector<Value> &args, std::int64_t line, CVM &VM) {
      if (args.size() != 0) {
        ErrorHandler::throw_runtime_error("flush() takes no arguments", line);
      }
      std::fflush(stdout);
      return {Utils::VOID};
    }
};

class NativeSize : public NativeFunction {
  public:
    Value execute(std::vector<Value> &args, std::int64_t line, CVM &VM) {
      if (args.size() != 1) {
        ErrorHandler::throw_runtime_error("size() expects one argument", line);
      }
      Value &arg = args[0];
      Value val(Utils::INT);
      if (arg.type == Utils::ARR) {
        val.number_value = arg.array_values.size();
      } else if (arg.type == Utils::STR) {
        val.number_value = arg.string_value.size();
      } else {
        ErrorHandler::throw_runtime_error("Cannot get the size of " + VM.stringify(arg), line);
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
      val.string_value = VM.stringify(args[0]);
      return val;
    }
};

class NativeToint : public NativeFunction {
  public:
    Value execute(std::vector<Value> &args, std::int64_t line, CVM &VM) {
      if (args.size() != 1) {
        ErrorHandler::throw_runtime_error("to_int() expects one argument", line);
      }
      Value &arg = args[0];
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
        ErrorHandler::throw_runtime_error(VM.stringify(arg) + " cannot be converted to int", line);
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
      Value &arg = args[0];
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
        ErrorHandler::throw_runtime_error(VM.stringify(arg) + " cannot be converted to double", line);
      }
      return val;
    }
};

class NativeExit : public NativeFunction {
  public:
    Value execute(std::vector<Value> &args, std::int64_t line, CVM &VM) {
      if (args.size() != 1 || args[0].type != Utils::INT) {
        ErrorHandler::throw_runtime_error("exit() expects one argument (int)", line);
      }
      std::exit(args[0].number_value);
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
      if (!(args[0].type == Utils::FLOAT || args[0].type == Utils::INT)
       || !(args[1].type == Utils::FLOAT || args[1].type == Utils::INT)) {
         ErrorHandler::throw_runtime_error("pow() arguments must be either int or double", line);
      }
      Value val(Utils::FLOAT);
      double arg1 = args[0].float_value;
      double arg2 = args[1].float_value;
      if (args[0].type == Utils::INT) arg1 = (double)args[0].number_value;
      if (args[1].type == Utils::INT) arg2 = (double)args[1].number_value;
      val.float_value = std::pow(arg1, arg2);
      return val;
    }
};

class NativeFileread : public NativeFunction {
  public:
    Value execute(std::vector<Value> &args, std::int64_t line, CVM &VM) {
      if (args.size() != 1 || args[0].type != Utils::STR) {
        ErrorHandler::throw_runtime_error("file_read() expects one argument (str)", line);
      }
      Value val(Utils::STR);
      std::ifstream f(args[0].string_value);
      if (!f.good()) {
        ErrorHandler::throw_runtime_error("couldn't read " + args[0].string_value, line);
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
      if (args.size() != 2 || args[0].type != Utils::STR || args[1].type != Utils::STR) {
        ErrorHandler::throw_runtime_error("file_write() expects two arguments (str, str)", line);
      }
      Value val(Utils::BOOL);
      std::ofstream f(args[0].string_value);
      if (!f.good()) {
        val.boolean_value = false;
        return val;
      }
      f << args[1].string_value;
      f.close();
      val.boolean_value = true;
      return val;
    }
};

class NativeFileexists : public NativeFunction {
  public:
    Value execute(std::vector<Value> &args, std::int64_t line, CVM &VM) {
      if (args.size() != 1 || args[0].type != Utils::STR) {
        ErrorHandler::throw_runtime_error("file_exists() expects one argument (str)", line);
      }
      Value val(Utils::BOOL);
      std::ifstream f(args[0].string_value);
      val.boolean_value = f.good();
      return val;
    }
};

class NativeFileremove : public NativeFunction {
  public:
    Value execute(std::vector<Value> &args, std::int64_t line, CVM &VM) {
      if (args.size() != 1 || args[0].type != Utils::STR) {
        ErrorHandler::throw_runtime_error("file_remove() expects one argument (str)", line);
      }
      Value val(Utils::BOOL);
      val.boolean_value = std::remove(args[0].string_value.c_str()) == 0;
      return val;
    }
};

class NativeAbs : public NativeFunction {
  public:
    Value execute(std::vector<Value> &args, std::int64_t line, CVM &VM) {
      if (args.size() != 1 || (args[0].type != Utils::INT && args[0].type != Utils::FLOAT)) {
        ErrorHandler::throw_runtime_error("abs() expects one argument (int|double)", line);
      }
      Value val;
      val.type = args[0].type;
      if (args[0].type == Utils::INT) {
        val.number_value = args[0].number_value;
        if (val.number_value < 0) {
          val.number_value = -val.number_value;
        }
      } else {
        val.float_value = args[0].float_value;
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
      if (args.size() != 2 || args[0].type != Utils::INT || args[1].type != Utils::INT) {
        ErrorHandler::throw_runtime_error("rand() expects two arguments (int, int)", line);
      }
      Value val(Utils::INT);
      std::random_device rd;
      std::default_random_engine generator(rd());
      std::uniform_int_distribution<std::int64_t> distribution(
        args[0].number_value, args[1].number_value
      );
      val.number_value = distribution(generator);
      return val;
    }
};

class NativeRandf : public NativeFunction {
  public:
    Value execute(std::vector<Value> &args, std::int64_t line, CVM &VM) {
      if (args.size() != 2 || args[0].type != Utils::FLOAT || args[1].type != Utils::FLOAT) {
        ErrorHandler::throw_runtime_error("randf() expects two arguments (double, double)", line);
      }
      Value val(Utils::FLOAT);
      std::random_device rd;
      std::default_random_engine generator(rd());
      std::uniform_real_distribution<double> distribution(
        args[0].float_value, args[1].float_value
      );
      val.float_value = distribution(generator);
      return val;
    }
};

class NativeContains : public NativeFunction {
  public:
    Value execute(std::vector<Value> &args, std::int64_t line, CVM &VM) {
      if (args.size() != 2 || args[0].type != Utils::STR || args[1].type != Utils::STR) {
        ErrorHandler::throw_runtime_error("contains() expects two arguments (str, str)", line);
      }
      Value val(Utils::BOOL);
      val.boolean_value = args[0].string_value.find(args[1].string_value) != std::string::npos;
      return val;
    }
};

class NativeSubstr : public NativeFunction {
  public:
    Value execute(std::vector<Value> &args, std::int64_t line, CVM &VM) {
      if (args.size() != 3 || args[0].type != Utils::STR || args[1].type != Utils::INT || args[2].type != Utils::INT) {
        ErrorHandler::throw_runtime_error("substr() expects three arguments (str, int, int)", line);
      }
      if (args[1].number_value < 0) {
        ErrorHandler::throw_runtime_error("starting position cannot be negative", line);
      }
      if (args[2].number_value < 0) {
        ErrorHandler::throw_runtime_error("length cannot be negative", line);
      }
      if (args[1].number_value + args[2].number_value > args[0].string_value.size()) {
        ErrorHandler::throw_runtime_error("out of string range", line);
      }
      Value val(Utils::STR);
      val.string_value = args[0].string_value.substr(args[1].number_value, args[2].number_value);
      return val;
    }
};

class NativeSplit : public NativeFunction {
  public:
    Value execute(std::vector<Value> &args, std::int64_t line, CVM &VM) {
      if (args.size() != 2 || args[0].type != Utils::STR || args[1].type != Utils::STR) {
        ErrorHandler::throw_runtime_error("split() expects two arguments (str, str)", line);
      }
      Value res(Utils::ARR);
      res.array_type = "str";
      std::string delim_copy = args[1].string_value;
      char *c_str = strdup(args[0].string_value.c_str());
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
      if (args.size() != 1 || args[0].type != Utils::STR) {
        ErrorHandler::throw_runtime_error("to_bytes() expects one argument (str)", line);
      }
      Value res(Utils::ARR);
      res.array_type = "int";
      const char *c_str = args[0].string_value.c_str();
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
      if (args.size() != 1 || args[0].type != Utils::ARR) {
        ErrorHandler::throw_runtime_error("from_bytes() expects one argument (arr)", line);
      }
      if (args[0].array_type != "int") {
        ErrorHandler::throw_runtime_error("from_bytes() expects an int array", line);
      }
      Value res(Utils::STR);
      res.string_value = "";
      for (auto &el : args[0].array_values) {
        res.string_value += (char)el.number_value;
      }
      return res;
    }
};

class NativeBind : public NativeFunction {
  public:
    Value execute(std::vector<Value> &args, std::int64_t line, CVM &VM) {
      if (args.size() != 1 || args[0].heap_reference == -1) {
        ErrorHandler::throw_runtime_error("bind() expects one argument (ref obj)", line);
      }
      std::int64_t ref = args[0].heap_reference;
      if (ref < 0 || ref >= VM.heap.chunks.size()) {
        ErrorHandler::throw_runtime_error("dereferencing a value that is not on the heap");
      }
      Value *ptr = VM.heap.chunks[ref].data;
      if (ptr == nullptr) {
        ErrorHandler::throw_runtime_error("dereferencing a null pointer");
      }
      if (ptr->type != Utils::OBJ) {
        ErrorHandler::throw_runtime_error("Can only bind a reference");
      }
      for (auto &pair : ptr->member_values) {
        Value *v = &pair.second;
        if (v->heap_reference != -1) {
          v = VM.heap.chunks[v->heap_reference].data;
        }
        if (v == nullptr) {
          ErrorHandler::throw_runtime_error("dereferencing a null pointer");
        }
        if (v->type == Utils::FUNC) {
          v->this_ref = ref;
        }
      }
      return {Utils::VOID};
    }
};

class NativeClassname : public NativeFunction {
  public:
    Value execute(std::vector<Value> &args, std::int64_t line, CVM &VM) {
      if (args.size() != 1 || args[0].type != Utils::OBJ) {
        ErrorHandler::throw_runtime_error("class_name() expects one argument (obj)", line);
      }
      Value res(Utils::STR);
      res.string_value = args[0].class_name;
      return res;
    }
};

class NativeArraytype : public NativeFunction {
  public:
    Value execute(std::vector<Value> &args, std::int64_t line, CVM &VM) {
      if (args.size() != 1 || args[0].type != Utils::ARR) {
        ErrorHandler::throw_runtime_error("array_type() expects one argument (arr)", line);
      }
      Value res(Utils::STR);
      res.string_value = args[0].array_type;
      return res;
    }
};

class NativeStacktrace : public NativeFunction {
  public:
    Value execute(std::vector<Value> &args, std::int64_t line, CVM &VM) {
      if (args.size() != 0) {
        ErrorHandler::throw_runtime_error("stack_trace() expects no arguments", line);
      }
      int limit = 100;
      int printed = 0;
      for (auto crumb = VM.trace.stack.rbegin(); crumb != VM.trace.stack.rend(); crumb++) {
        if (printed > limit) {
          std::cout << "    and " << (VM.trace.stack.size() - printed) << " more\n";
          break;
        }
        std::string name = crumb->name.size() == 0 ? "<anonymous function>" : "function '" + crumb->name + "'";
        std::cout << "  in " << name << " called on line " << crumb->line;
        if (crumb->source != nullptr) {
          std::cout << " in file " << *crumb->source;
        }
        std::cout << std::endl;
        printed++;
      }
      return {Utils::VOID};
    }
};

class NativeSleep : public NativeFunction {
  public:
    Value execute(std::vector<Value> &args, std::int64_t line, CVM &VM) {
      if (args.size() != 1 || args[0].type != Utils::INT) {
        ErrorHandler::throw_runtime_error("sleep() expects one argument (int)", line);
      }
      if (args[0].number_value < 0) {
        ErrorHandler::throw_runtime_error("Sleep time must be greater than -1", line);
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(args[0].number_value));
      return {Utils::VOID};
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
  globals.reserve(41);
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
  ADD_FN(NativeClassname, class_name);
  ADD_FN(NativeArraytype, array_type);
  ADD_FN(NativeStacktrace, stack_trace);
  ADD_FN(NativeSleep, sleep);
}