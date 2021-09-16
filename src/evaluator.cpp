#include "evaluator.hpp"
#include "utils.hpp"
#include "error-handler.hpp"
#include "CVM.hpp"

#include <iostream>
#include <cassert>
#include <regex>
#include <memory>

#define FLAG_OK 0
#define FLAG_BREAK 1
#define FLAG_CONTINUE 2
#define FLAG_RETURN 3
#define FLAG_ERROR 4

typedef Statement::StmtType StmtType;
typedef Utils::VarType VarType;
typedef std::shared_ptr<RpnElement> SharedRpnElement;
typedef std::vector<SharedRpnElement> SharedRpnStack;

#define SHARE_RPN(rpn) std::make_shared<RpnElement>(rpn)

#define REG(OP, FN) if (token.op.type == Token::OP) {res_stack.emplace_back(SHARE_RPN(FN(*x, *y)));} else

#define BITWISE(OP, NAME)\
  Value val;\
  const Value &x_val = get_value(x);\
  const Value &y_val = get_value(y);\
  if (x_val.type == VarType::INT && y_val.type == VarType::INT) {\
    val.type = VarType::INT;\
    val.number_value = x_val.number_value OP y_val.number_value;\
    return {val};\
  }\
  const std::string &msg = "Cannot perform bitwise " NAME " on " + stringify(x_val) + " and " + stringify(y_val);\
  throw_error(msg);\
  return {};

void Evaluator::throw_error(const std::string &cause) {
  if (current_source != nullptr) {
    std::cout << "(" << *current_source << ") ";
  }
  std::cout << "Runtime error: " << cause << " (line " << current_line << ")\n";
  if (VM.trace.stack.size() == 0) std::exit(EXIT_FAILURE);
  std::vector<Value> args;
  VM.globals.at("stack_trace")->execute(args, current_line, VM);
  std::exit(EXIT_FAILURE);
}

void Evaluator::start() {
  for (const auto &statement : AST.children) {
    int flag = execute_statement(statement);
    if (flag == FLAG_RETURN) {
      break;
    }
  }
  if (return_value.type == VarType::UNKNOWN) {
    return_value.type = VarType::VOID;
  }
}

int Evaluator::execute_statement(const Node &statement) {
  current_line = statement.stmt.line;
  current_source = statement.stmt.source;
  if (statement.stmt.type == StmtType::NONE) {
    return FLAG_OK;
  } else if (statement.stmt.type == StmtType::EXPR) {
    if (statement.stmt.expressions.size() != 1) return FLAG_OK;
    evaluate_expression(statement.stmt.expressions[0]);
    return FLAG_OK;
  } else if (statement.stmt.type == StmtType::CLASS) {
    register_class(statement.stmt.class_stmt);
    return FLAG_OK;
  } else if (statement.stmt.type == StmtType::SET) {
    if (statement.stmt.expressions.size() == 0) return FLAG_OK; // might break something
    set_member(statement.stmt.obj_members, statement.stmt.expressions[0]);
    return FLAG_OK;
  } else if (statement.stmt.type == StmtType::SET_IDX) {
    set_index(statement.stmt);
    return FLAG_OK;
  } else if (statement.stmt.type == StmtType::DECL) {
    if (statement.stmt.declaration.size() != 1) return FLAG_OK;
    declare_variable(statement.stmt.declaration[0]);
    return FLAG_OK;
  } else if (statement.stmt.type == StmtType::COMPOUND) {
    if (statement.stmt.statements.size() == 0) return FLAG_OK;
    for (auto &stmt : statement.stmt.statements[0].children) {
      int flag = execute_statement(stmt);
      if (flag) return flag;
    }
    return FLAG_OK;
  } else if (statement.stmt.type == StmtType::BREAK) {
    if (!nested_loops) {
      throw_error("break statement outside of loops is illegal");
    }
    return FLAG_BREAK;
  } else if (statement.stmt.type == StmtType::CONTINUE) {
    if (!nested_loops) {
      throw_error("continue statement outside of loops is illegal");
    }
    return FLAG_CONTINUE;
  } else if (statement.stmt.type == StmtType::RETURN) {
    if (!inside_func) {
      throw_error("return statement outside of functions is illegal");
    }
    const NodeList &return_expr = statement.stmt.expressions[0];
    if (statement.stmt.expressions.size() != 0 && return_expr.size() != 0) {
      return_value = evaluate_expression(return_expr, returns_ref);
    }
    return FLAG_RETURN;
  } else if (statement.stmt.type == StmtType::WHILE) {
    assert(statement.stmt.expressions.size() != 0);
    if (statement.stmt.statements.size() == 0) return FLAG_OK; // might cause bugs
    if (statement.stmt.expressions[0].size() == 0) {
      throw_error("while expects an expression");
    }
    nested_loops++;
    while (true) {
      const Value &result = evaluate_expression(statement.stmt.expressions[0]);
      if (result.type != VarType::BOOL) {
        const std::string &msg = "Expected a boolean value in while statement, found " + stringify(result);
        throw_error(msg);
      }
      if (!result.boolean_value) break;
      int flag = execute_statement(statement.stmt.statements[0]);
      if (flag == FLAG_BREAK) break;
      if (flag == FLAG_RETURN) return flag;
    }
    nested_loops--;
    return FLAG_OK;
  } else if (statement.stmt.type == StmtType::FOR) {
    if (statement.stmt.expressions.size() != 3) {
      std::string given = std::to_string(statement.stmt.expressions.size());
      throw_error("For expects 3 expressions, " + given + " given");
    }
    if (statement.stmt.statements.size() == 0) return FLAG_OK; // might cause bugs
    if (statement.stmt.expressions[0].size() != 0) {
      evaluate_expression(statement.stmt.expressions[0]);
    }
    nested_loops++;
    const NodeList &cond = statement.stmt.expressions[1];
    bool auto_true = cond.size() == 0; // empty conditions evaluate to true
    while (true) {
      if (!auto_true) {
        const Value &result = evaluate_expression(cond);
        if (result.type != VarType::BOOL) {
          const std::string &msg = "Expected a boolean value in while statement, found " + stringify(result);
          throw_error(msg);
        }
        if (!result.boolean_value) break;
      }
      int flag = execute_statement(statement.stmt.statements[0]);
      if (flag == FLAG_BREAK) break;
      if (flag == FLAG_RETURN) return flag;
      const NodeList &increment_expr = statement.stmt.expressions[2];
      if (increment_expr.size() != 0) {
        evaluate_expression(increment_expr);
      }
    }
    nested_loops--;
    return FLAG_OK;
  } else if (statement.stmt.type == StmtType::IF) {
    if (statement.stmt.statements.size() == 0) return FLAG_OK; // might cause bugs
    assert(statement.stmt.expressions.size() != 0);
    if (statement.stmt.expressions[0].size() == 0) {
      throw_error("if expects an expression");
    }
    const Value &result = evaluate_expression(statement.stmt.expressions[0]);
    if (result.type != VarType::BOOL) {
      const std::string &msg = "Expected a boolean value in if statement, found " + stringify(result);
      throw_error(msg);
    }
    if (result.boolean_value) {
      int flag = execute_statement(statement.stmt.statements[0]);
      if (flag) return flag;
    } else {
      if (statement.stmt.statements.size() == 2) {
        int flag = execute_statement(statement.stmt.statements[1]);
        if (flag) return flag;
      }
    }
    return FLAG_OK;
  }
  throw_error("Unknown statement! (" + std::to_string(statement.stmt.type) + ")");
  return FLAG_ERROR;
}

Value Evaluator::evaluate_expression(const NodeList &expression_tree, const bool get_ref) {
  RpnStack rpn_stack;
  rpn_stack.reserve(100);
  flatten_tree(rpn_stack, expression_tree);
  // TODO: cache the result somehow
  SharedRpnStack res_stack;
  assert(rpn_stack.size() != 0);
  res_stack.reserve(rpn_stack.size() * 3);
  for (const auto &token : rpn_stack) {
    if (token.type == RpnElement::OPERATOR) {
      if (token.op.op_type == Operator::BASIC) {
        if (utils.op_binary(token.op.type)) {
          if (res_stack.size() < 2) {
            const std::string &msg = "Operator " + Token::get_name(token.op.type) + " expects two operands"; 
            throw_error(msg);
          }
          const SharedRpnElement y = res_stack.back();
          res_stack.pop_back();
          const SharedRpnElement x = res_stack.back();
          res_stack.pop_back();
          REG(DOT, access_member)
          REG(OP_PLUS, perform_addition)
          REG(OP_MINUS, perform_subtraction)
          REG(OP_MUL, perform_multiplication)
          REG(OP_DIV, perform_division)
          REG(OP_MOD, perform_modulo)
          REG(OP_ASSIGN, assign)
          REG(OP_EQ, compare_eq)
          REG(OP_NOT_EQ, compare_neq)
          REG(OP_GT, compare_gt)
          REG(OP_LT, compare_lt)
          REG(OP_GT_EQ, compare_gt_eq)
          REG(OP_LT_EQ, compare_lt_eq)
          REG(PLUS_ASSIGN, plus_assign)
          REG(MINUS_ASSIGN, minus_assign)
          REG(MUL_ASSIGN, mul_assign)
          REG(DIV_ASSIGN, div_assign)
          REG(OP_OR, logical_or)
          REG(OP_AND, logical_and)
          REG(LSHIFT, shift_left)
          REG(RSHIFT, shift_right)
          REG(OP_XOR, bitwise_xor)
          REG(OP_AND_BIT, bitwise_and)
          REG(OP_OR_BIT, bitwise_or)
          REG(RSHIFT_ASSIGN, rshift_assign)
          REG(LSHIFT_ASSIGN, lshift_assign)
          REG(AND_ASSIGN, and_assign)
          REG(OR_ASSIGN, or_assign)
          REG(XOR_ASSIGN, xor_assign) {
            const std::string &msg = "Unknown binary operator " + Token::get_name(token.op.type);
            throw_error(msg);
          }
        } else if (utils.op_unary(token.op.type)) {
          if (res_stack.size() < 1) {
            const std::string &msg = "Operator " + Token::get_name(token.op.type) + " expects one operand"; 
            throw_error(msg);
          }
          const SharedRpnElement x = res_stack.back();
          res_stack.pop_back();
          if (token.op.type == Token::OP_NOT) {
            res_stack.emplace_back(SHARE_RPN(logical_not(*x)));
          } else if (token.op.type == Token::OP_NEG) {
            res_stack.emplace_back(SHARE_RPN(bitwise_not(*x)));
          } else if (token.op.type == Token::DEL) {
            res_stack.emplace_back(SHARE_RPN(delete_value(*x)));
          } else {
            const std::string &msg = "Unknown unary operator " + Token::get_name(token.op.type);
            throw_error(msg);
          }
        }
      } else if (token.op.op_type == Operator::FUNC) {
        const SharedRpnElement fn = res_stack.back();
        res_stack.pop_back();
        res_stack.emplace_back(SHARE_RPN(execute_function(*fn, token)));
      } else if (token.op.op_type == Operator::INDEX) {
        const SharedRpnElement arr = res_stack.back();
        res_stack.pop_back();
        res_stack.emplace_back(SHARE_RPN(access_index(*arr, token)));
      }
    } else {
      res_stack.emplace_back(SHARE_RPN(token));
    }
  }
  Value &res_val = res_stack[0]->value;
  if (get_ref) {
    if (res_val.is_lvalue()) {
      std::shared_ptr<Variable> var = get_reference_by_name(res_val.reference_name);
      if (var == nullptr) {
        const std::string &msg = "'" + res_val.reference_name + "' is not defined";
        throw_error(msg);
      }
      if (var->val.heap_reference != -1) {
        return var->val;
      } else {
        throw_error("Expression expected to be a reference");
      }
    } else if (res_val.heap_reference != -1) {
      return res_val;
    } else {
      throw_error("Expression expected to be a reference");
    }
  }
  if (res_val.is_lvalue() || res_val.heap_reference > -1) {
    RpnElement wrapper = res_val;
    return get_value(wrapper);
  }
  return res_val;
}

std::string Evaluator::stringify(const Value &val) {
  if (val.heap_reference != -1) {
    return "reference to " + stringify(get_heap_value(val.heap_reference));
  } else if (val.type == VarType::STR) {
    return val.string_value;
  } else if (val.type == VarType::BOOL) {
    return val.boolean_value ? "true" : "false";
  } else if (val.type == VarType::FLOAT) {
    return std::to_string(val.float_value);
  } else if (val.type == VarType::INT) {
    return std::to_string(val.number_value);
  } else if (val.type == VarType::FUNC) {
    return "function";
  } else if (val.type == VarType::CLASS) {
    return "class";
  } else if (val.type == VarType::OBJ) {
    return "object";
  } else if (val.type == VarType::ARR) {
    return "array";
  } else if (val.type == VarType::VOID) {
    return "void";
  } else if (val.type == VarType::UNKNOWN) {
    return "null";
  } else if (val.type == VarType::ID) {
    return val.reference_name;
  }
  return "";
}

inline double Evaluator::to_double(const Value &val) {
  if (val.type == VarType::FLOAT) {
    return val.float_value;
  } else if (val.type == VarType::INT) {
    return (double)val.number_value;
  }
  throw_error("Cannot convert " + stringify(val) + " to double");
  return 0;
}

RpnElement Evaluator::logical_not(const RpnElement &x) {
  const Value &x_val = get_value(x);
  Value val;
  if (x_val.type == VarType::BOOL) {
    val.type = VarType::BOOL;
    val.boolean_value = !x_val.boolean_value;
    return {val};
  }
  throw_error("Cannot perform logical not on " + stringify(x_val) + "\n");
  return {};
}

RpnElement Evaluator::Evaluator::bitwise_not(const RpnElement &x) {
  const Value &x_val = get_value(x);
  Value val;
  if (x_val.type == VarType::INT) {
    val.type = VarType::INT;
    val.number_value = ~x_val.number_value;
    return {val};
  }
  throw_error("Cannot perform bitwise not on " + stringify(x_val) + "\n");
  return {};
}

RpnElement Evaluator::delete_value(const RpnElement &x) {
  const Value *val = &x.value;
  std::shared_ptr<Variable> v = nullptr;
  if (val->is_lvalue()) {
    v = get_reference_by_name(val->reference_name);
    if (v == nullptr) {
      throw_error(val->reference_name + " is not defined");
    }
    val = &v->val;
  }
  if (val->heap_reference == -1) {
    throw_error(x.value.reference_name + " is not allocated on heap");
  }
  if (val->heap_reference >= VM.heap.chunks.size()) {
    throw_error("deleting a value that is not on the heap");
  }
  if (VM.heap.chunks[val->heap_reference].used == false) {
    throw_error("double delete");
  }
  VM.heap.free(val->heap_reference);
  if (v != nullptr) {
    v->val.heap_reference = -1;
  }
  Value res;
  res.type = VarType::VOID;
  return {res};
}

RpnElement Evaluator::perform_addition(const RpnElement &x, const RpnElement &y) {
  Value val;
  const Value &x_val = get_value(x);
  const Value &y_val = get_value(y);
  if (x_val.type == VarType::ARR) {
    if (y_val.type == utils.var_lut.at(x_val.array_type)) {
      // append to array
      Value x_val_cpy = x_val;
      x_val_cpy.array_values.push_back(y_val);
      return {x_val_cpy};
    } else {
      throw_error("Cannot append " + stringify(y_val) + " to an array of " + x_val.array_type + "s");
    }
  } else if (y_val.type == VarType::ARR) {
    if (x_val.type == utils.var_lut.at(y_val.array_type)) {
      // prepend to array
      Value y_val_cpy = y_val;
      y_val_cpy.array_values.insert(y_val.array_values.begin(), x_val);
      return {y_val_cpy};
    } else {
      throw_error("Cannot prepend " + stringify(x_val) + " to an array of " + y_val.array_type + "s");
    }
  } else if (x_val.type == VarType::STR || y_val.type == VarType::STR) {
    val.type = VarType::STR;
    val.string_value = stringify(x_val) + stringify(y_val);
    return {val};
  } else if (x_val.type == VarType::INT && y_val.type == VarType::INT) {
    val.type = VarType::INT;
    val.number_value = x_val.number_value + y_val.number_value;
    return {val};
  } else if (x_val.type == VarType::FLOAT || y_val.type == VarType::FLOAT) {
    val.type = VarType::FLOAT;
    val.float_value = to_double(x_val) + to_double(y_val);
    return {val};
  }
  const std::string &msg = "Cannot perform addition on " + stringify(x_val) + " and " + stringify(y_val);
  throw_error(msg);
  return {};
}

RpnElement Evaluator::perform_subtraction(const RpnElement &x, const RpnElement &y) {
  Value val;
  const Value &x_val = get_value(x);
  const Value &y_val = get_value(y);
  if (x_val.type == VarType::INT && y_val.type == VarType::INT) {
    val.type = VarType::INT;
    val.number_value = x_val.number_value - y_val.number_value;
    return {val};
  } else if (x_val.type == VarType::ARR && y_val.type == VarType::INT) {
    // remove from array
    Value x_val_cpy = x_val;
    if (y_val.number_value < 0 || y_val.number_value >= x_val_cpy.array_values.size()) {
      const std::string &msg = "cannot remove index [" + std::to_string(y_val.number_value) + "] (out of range)";
      throw_error(msg);
    }
    x_val_cpy.array_values.erase(x_val_cpy.array_values.begin() + y_val.number_value);
    return {x_val_cpy};
  } else if (x_val.type == VarType::FLOAT || y_val.type == VarType::FLOAT) {
    val.type = VarType::FLOAT;
    val.float_value = to_double(x_val) - to_double(y_val);
    return {val};
  }
  const std::string &msg = "Cannot perform subtraction on " + stringify(x_val) + " and " + stringify(y_val);
  throw_error(msg);
  return {};
}

RpnElement Evaluator::perform_multiplication(const RpnElement &x, const RpnElement &y) {
  Value val;
  const Value &x_val = get_value(x);
  const Value &y_val = get_value(y);
  if (x_val.type == VarType::INT && y_val.type == VarType::INT) {
    val.type = VarType::INT;
    val.number_value = x_val.number_value * y_val.number_value;
    return {val};
  } else if (x_val.type == VarType::FLOAT || y_val.type == VarType::FLOAT) {
    val.type = VarType::FLOAT;
    val.float_value = to_double(x_val) * to_double(y_val);
    return {val};
  }
  const std::string &msg = "Cannot perform multiplication on " + stringify(x_val) + " and " + stringify(y_val);
  throw_error(msg);
  return {};
}

RpnElement Evaluator::perform_division(const RpnElement &x, const RpnElement &y) {
  Value val;
  const Value &x_val = get_value(x);
  const Value &y_val = get_value(y);
  if (x_val.type == VarType::INT && y_val.type == VarType::INT) {
    if (y_val.number_value == 0) {
      throw_error("Cannot divide by zero");
    }
    val.type = VarType::INT;
    val.number_value = x_val.number_value / y_val.number_value;
    return {val};
  } else if (x_val.type == VarType::FLOAT || y_val.type == VarType::FLOAT) {
    double f1 = to_double(x_val);
    double f2 = to_double(y_val);
    if (f2 == 0.0f) {
      throw_error("Cannot divide by zero");
    }
    val.type = VarType::FLOAT;
    val.float_value = f1 / f2;
    return {val};
  }
  const std::string &msg = "Cannot perform division on " + stringify(x_val) + " and " + stringify(y_val);
  throw_error(msg);
  return {};
}

RpnElement Evaluator::perform_modulo(const RpnElement &x, const RpnElement &y) {
  Value val;
  const Value &x_val = get_value(x);
  const Value &y_val = get_value(y);
  if (x_val.type == VarType::INT && y_val.type == VarType::INT) {
    if (y_val.number_value == 0) {
      throw_error("Cannot divide by 0");
    }
    val.type = VarType::INT;
    val.number_value = x_val.number_value % y_val.number_value;
    return {val};
  }
  const std::string &msg = "Cannot perform modulo on " + stringify(x_val) + " and " + stringify(y_val);
  throw_error(msg);
  return {};
}

RpnElement Evaluator::bitwise_and(const RpnElement &x, const RpnElement &y) {
  BITWISE(&, "and")
}

RpnElement Evaluator::bitwise_or(const RpnElement &x, const RpnElement &y) {
  BITWISE(|, "or")
}

RpnElement Evaluator::shift_left(const RpnElement &x, const RpnElement &y) {
  BITWISE(<<, "left shift")
}

RpnElement Evaluator::shift_right(const RpnElement &x, const RpnElement &y) {
  BITWISE(>>, "right shift")
}

RpnElement Evaluator::bitwise_xor(const RpnElement &x, const RpnElement &y) {
  const Value &x_val = get_value(x);
  const Value &y_val = get_value(y);
  if (x_val.type == VarType::ARR && y_val.type == VarType::ARR) {
    // concat arrays
    if (x_val.array_type == y_val.array_type) {
      Value x_val_cpy = x_val;
      x_val_cpy.array_values.insert(x_val.array_values.end(), y_val.array_values.begin(), y_val.array_values.end());
      return {x_val_cpy};
    } else {
      const std::string &msg = "Cannot concatenate arrays of type " + x_val.array_type + " and " + y_val.array_type;
      throw_error(msg);
    }
  }
  Value val;
  if (x_val.type == VarType::INT && y_val.type == VarType::INT) {
    val.type = VarType::INT;
    val.number_value = x_val.number_value ^ y_val.number_value;
    return {val};
  }
  const std::string &msg = "Cannot perform bitwise xor on " + stringify(x_val) + " and " + stringify(y_val);
  throw_error(msg);
  return {};
}

RpnElement Evaluator::logical_and(const RpnElement &x, const RpnElement &y) {
  Value val;
  const Value &x_val = get_value(x);
  const Value &y_val = get_value(y);
  if (x_val.type == VarType::BOOL && y_val.type == VarType::BOOL) {
    val.type = VarType::BOOL;
    val.boolean_value = x_val.boolean_value && y_val.boolean_value;
    return {val};
  }
  const std::string &msg = "Cannot perform logical and on " + stringify(x_val) + " and " + stringify(y_val);
  throw_error(msg);
  return {};
}

RpnElement Evaluator::logical_or(const RpnElement &x, const RpnElement &y) {
  Value val;
  const Value &x_val = get_value(x);
  const Value &y_val = get_value(y);
  if (x_val.type == VarType::BOOL && y_val.type == VarType::BOOL) {
    val.type = VarType::BOOL;
    val.boolean_value = x_val.boolean_value || y_val.boolean_value;
    return {val};
  }
  const std::string &msg = "Cannot perform logical or on " + stringify(x_val) + " and " + stringify(y_val);
  throw_error(msg);
  return {};
}

RpnElement Evaluator::assign(RpnElement &x, const RpnElement &y) {
  if (!x.value.is_lvalue()) {
    throw_error("Cannot assign to an rvalue");
  }
  std::shared_ptr<Variable> var = get_reference_by_name(x.value.reference_name);
  if (var == nullptr) {
    const std::string &msg = x.value.reference_name + " is not defined";
    throw_error(msg);
  }
  if (var->constant) {
    const std::string &msg = "Cannot reassign a constant variable (" + x.value.reference_name + ")";
    throw_error(msg);
  }
  Value &x_value = get_mut_value(x);
  const Value y_value = get_value(y);
  if (x_value.type == VarType::UNKNOWN) {
    const std::string &msg = x_value.reference_name + " doesn't point to anything on the heap";
    throw_error(msg);
  }
  if (x_value.type != y_value.type) {
    const std::string &msg = "Cannot assign " + stringify(y_value) + " to " + x.value.reference_name;
    throw_error(msg);
  }
  x_value = y_value;
  return {x_value};
}

RpnElement Evaluator::plus_assign(RpnElement &x, const RpnElement &y) {
  const RpnElement &rvalue = perform_addition(x, y);
  return assign(x, rvalue);
}

RpnElement Evaluator::minus_assign(RpnElement &x, const RpnElement &y) {
  const RpnElement &rvalue = perform_subtraction(x, y);
  return assign(x, rvalue);
}

RpnElement Evaluator::mul_assign(RpnElement &x, const RpnElement &y) {
  const RpnElement &rvalue = perform_multiplication(x, y);
  return assign(x, rvalue);
}

RpnElement Evaluator::div_assign(RpnElement &x, const RpnElement &y) {
  const RpnElement &rvalue = perform_division(x, y);
  return assign(x, rvalue);
}

RpnElement Evaluator::mod_assign(RpnElement &x, const RpnElement &y) {
  const RpnElement &rvalue = perform_modulo(x, y);
  return assign(x, rvalue);
}

RpnElement Evaluator::lshift_assign(RpnElement &x, const RpnElement &y) {
  const RpnElement &rvalue = shift_left(x, y);
  return assign(x, rvalue);
}

RpnElement Evaluator::rshift_assign(RpnElement &x, const RpnElement &y) {
  const RpnElement &rvalue = shift_right(x, y);
  return assign(x, rvalue);
}

RpnElement Evaluator::and_assign(RpnElement &x, const RpnElement &y) {
  const RpnElement &rvalue = bitwise_and(x, y);
  return assign(x, rvalue);
}

RpnElement Evaluator::or_assign(RpnElement &x, const RpnElement &y) {
  const RpnElement &rvalue = bitwise_or(x, y);
  return assign(x, rvalue);
}

RpnElement Evaluator::xor_assign(RpnElement &x, const RpnElement &y) {
  const RpnElement &rvalue = bitwise_xor(x, y);
  return assign(x, rvalue);
}

RpnElement Evaluator::access_member(RpnElement &x, const RpnElement &y) {
  if (!y.value.is_lvalue()) {
    throw_error("Object members can only be accessed with lvalues");
  }
  Value &obj = get_mut_value(x);
  if (obj.type != VarType::OBJ) {
    const std::string &msg = stringify(obj) + " is not an object";
    throw_error(msg);
  }
  const auto member_it = obj.member_values.find(y.value.reference_name);
  if (member_it == obj.member_values.end()) {
    std::string object_name = x.value.is_lvalue() ? " " + x.value.reference_name + " " : " ";
    const std::string &msg = "Object" + object_name + "has no member named " + y.value.reference_name;
    throw_error(msg);
  }
  Value &val = member_it->second;
  if (val.type == Utils::FUNC) {
    val.func_name = y.value.reference_name;
  }
  return {val};
}

RpnElement Evaluator::access_index(RpnElement &arr, const RpnElement &idx) {
  Value &array = get_mut_value(arr);
  if (array.type != VarType::ARR) {
    const std::string &msg = stringify(array) + " is not an array";
    throw_error(msg);
  }
  const Value &index = evaluate_expression(idx.op.index_rpn);
  if (index.type != VarType::INT) {
    const std::string &msg = "index expected to be an int, but " + stringify(index) + " found";
    throw_error(msg);
  }
  if (index.number_value < 0 || index.number_value >= array.array_values.size()) {
    const std::string &msg = "index [" + std::to_string(index.number_value) + "] out of range";
    throw_error(msg);
  }
  Value &res = array.array_values[index.number_value];
  return {res};
}

RpnElement Evaluator::compare_eq(const RpnElement &x, const RpnElement &y) {
  const Value &x_val = get_value(x);
  const Value &y_val = get_value(y);
  Value val;
  if (x_val.type == VarType::FLOAT || y_val.type == VarType::FLOAT) {
    val.type = VarType::BOOL;
    val.boolean_value = to_double(x_val) == to_double(y_val);
    return {val};
  } else if (x_val.type == VarType::INT && y_val.type == VarType::INT) {
    val.type = VarType::BOOL;
    val.boolean_value = x_val.number_value == y_val.number_value;
    return {val};
  } else if (x_val.type == VarType::STR && y_val.type == VarType::STR) {
    val.type = VarType::BOOL;
    val.boolean_value = x_val.string_value == y_val.string_value;
    return {val};
  } else if (x_val.type == VarType::BOOL && y_val.type == VarType::BOOL) {
    val.type = VarType::BOOL;
    val.boolean_value = x_val.boolean_value == y_val.boolean_value;
    return {val};
  }
  const std::string &msg = "Cannot compare " + stringify(x_val) + " to " + stringify(y_val);
  throw_error(msg);
  return {};
}

RpnElement Evaluator::compare_neq(const RpnElement &x, const RpnElement &y) {
  Value val;
  val.type = VarType::BOOL;
  val.boolean_value = !compare_eq(x, y).value.boolean_value;
  return {val};
}

RpnElement Evaluator::compare_gt(const RpnElement &x, const RpnElement &y) {
  const Value &x_val = get_value(x);
  const Value &y_val = get_value(y);
  Value val;
  if (x_val.type == VarType::FLOAT || y_val.type == VarType::FLOAT) {
    val.type = VarType::BOOL;
    val.boolean_value = to_double(x_val) > to_double(y_val);
    return {val};
  } else if (x_val.type == VarType::INT && y_val.type == VarType::INT) {
    val.type = VarType::BOOL;
    val.boolean_value = x_val.number_value > y_val.number_value;
    return {val};
  }
  const std::string &msg = "Cannot compare " + stringify(x_val) + " to " + stringify(y_val);
  throw_error(msg);
  return {};
}

RpnElement Evaluator::compare_lt(const RpnElement &x, const RpnElement &y) {
  return compare_gt(y, x);
}

RpnElement Evaluator::compare_gt_eq(const RpnElement &x, const RpnElement &y) {
  const RpnElement &gt = compare_gt(x, y);
  const RpnElement &eq = compare_eq(x, y);
  Value val;
  val.type = VarType::BOOL;
  val.boolean_value = gt.value.boolean_value || eq.value.boolean_value;
  return {val};
}

RpnElement Evaluator::compare_lt_eq(const RpnElement &x, const RpnElement &y) {
  return compare_gt_eq(y, x);
}

void Evaluator::register_class(const ClassStatement &_class) {
  const std::shared_ptr<Variable> v = get_reference_by_name(_class.class_name);
  if (v != nullptr) {
    stack.erase(_class.class_name);
  }
  auto &var = (stack[_class.class_name] = std::make_shared<Variable>());
  var->type = "class";
  var->val.type = VarType::CLASS;
  var->val.members = _class.members;
  var->val.class_name = _class.class_name;
}

void Evaluator::declare_variable(const Node &declaration) {
  const Declaration &decl = declaration.decl;
  const Value &var_val = evaluate_expression(decl.var_expr, decl.reference);
  const Utils::VarType &var_type = utils.var_lut.at(decl.var_type);
  Utils::VarType expr_type = var_val.type;
  if (decl.reference) {
    expr_type = get_heap_value(var_val.heap_reference).type;
  }
  if (var_type != expr_type) {
    const std::string &msg = "Cannot assign " + stringify(var_val) + " to a variable of type " + decl.var_type;
    throw_error(msg);
  }
  const std::shared_ptr<Variable> v = get_reference_by_name(decl.id);
  if (v != nullptr) {
    // to avoid memory leaks while redeclaring
    stack.erase(decl.id);
  }
  if (decl.allocated) {
    assert(decl.reference == false);
    const Chunk &chunk = VM.heap.allocate();
    auto &var = (stack[decl.id] = std::make_shared<Variable>());
    var->val.heap_reference = chunk.heap_reference;
    var->type = decl.var_type;
    var->constant = decl.constant;
    *chunk.data = var_val;
    if (var_val.type == Utils::OBJ) {
      // bind the reference to the object to 'this' in its methods
      std::vector<Value> args(1, var->val);
      if (native_bind == nullptr) {
        native_bind = VM.globals.at("bind");
      }
      native_bind->execute(args, current_line, VM);
    }
    return;
  }
  auto &var = (stack[decl.id] = std::make_shared<Variable>());
  var->type = decl.var_type;
  var->val = var_val;
  var->constant = decl.constant;
}

RpnElement Evaluator::construct_object(const RpnElement &call, const RpnElement &_class) {
  Value val;
  const Value &class_val = get_value(_class);
  int args_counter = 0;
  for (const auto &arg : call.op.func_call.arguments) {
    if (arg.size() != 0) {
      args_counter++;
    } else {
      throw_error("Illegal class invocation, missing members");
    }
  }
  std::size_t members_count = class_val.members.size();
  if (args_counter != members_count) {
    std::string &&msg = _class.value.reference_name + " has " + std::to_string(class_val.members.size());
    msg += " members, " + std::to_string(args_counter) + " given";
    throw_error(msg);
  }
  val.class_name = _class.value.reference_name;
  val.type = VarType::OBJ;
  val.members.reserve(members_count);
  int i = 0;
  for (const auto &node_list : call.op.func_call.arguments) {
    const FuncParam &member = class_val.members[i];
    Value &&arg_val = evaluate_expression(node_list, member.is_ref);
    Value real_val = arg_val;
    VarType arg_type = arg_val.type;
    if (arg_val.heap_reference != -1) {
      real_val = get_heap_value(arg_val.heap_reference);
      arg_type = real_val.type;
    }
    if (member.is_ref && arg_val.heap_reference == -1) {
      std::string num = std::to_string(i + 1);
      const std::string &msg = "Object argument " + num + " expected to be a reference, but value given";
      throw_error(msg);
    }
    if (arg_type != utils.var_lut.at(member.type_name)) {
      std::string num = std::to_string(i + 1);
      const std::string &msg = "Argument " + num + " expected to be " + member.type_name + ", but " + stringify(real_val) + " given";
      throw_error(msg);
    }
    arg_val.member_name = member.param_name;
    val.member_values.insert(std::make_pair(member.param_name, arg_val));
    i++;
  }
  return {val};
}

RpnElement Evaluator::execute_function(RpnElement &fn, const RpnElement &call) {
  assert(call.op.op_type == Operator::FUNC);
  auto global_it = VM.globals.find(fn.value.reference_name);
  if (fn.value.is_lvalue() && global_it != VM.globals.end()) {
    std::vector<Value> call_args;
    call_args.reserve(call.op.func_call.arguments.size());
    bool needs_ref = fn.value.reference_name == "bind";
    for (const auto &node_list : call.op.func_call.arguments) {
      if (node_list.size() == 0) break;
      call_args.push_back(evaluate_expression(node_list, needs_ref));
    }
    VM.trace.push(fn.value.reference_name, current_line, current_source);
    const Value &return_val = global_it->second->execute(call_args, current_line, VM);
    VM.trace.pop();
    return {return_val};
  }
  Value &fn_value = get_mut_value(fn);
  if (fn_value.type == VarType::CLASS) {
    return construct_object(call, fn);
  }
  if (fn_value.type == VarType::STR) {
    // string interpolation
    int args = 0;
    for (const auto &arg : call.op.func_call.arguments) {
      if (arg.size() != 0) {
        args++;
      } else if (args != 0) {
        throw_error("Illegal string interpolation, missing arguments");
      }
    }
    Value str = fn_value;
    if (args == 0) return {str};
    int argn = 1;
    for (const auto &arg : call.op.func_call.arguments) {
      Value arg_val = evaluate_expression(arg);
      std::string find = "@" + std::to_string(argn);
      str.string_value = std::regex_replace(str.string_value, std::regex(find), VM.stringify(arg_val));
      argn++;
    }
    return {str};
  }
  if (fn_value.type != VarType::FUNC) {
    const std::string &msg = stringify(fn_value) + " is not a function or a string";
    throw_error(msg);
  }
  if (fn_value.func.instructions.size() == 0) return {};
  int args_counter = 0;
  for (const auto &arg : call.op.func_call.arguments) {
    if (arg.size() != 0) {
      args_counter++;
    } else if (args_counter != 0) {
      throw_error("Illegal function invocation, missing arguments");
    }
  }
  if (args_counter != fn_value.func.params.size()) {
    std::string params_expected = std::to_string(fn_value.func.params.size());
    std::string params_given = std::to_string(args_counter);
    const std::string &msg = stringify(fn_value) + " expects " + params_expected + " argument(s), " + params_given + " given";
    throw_error(msg);
  }

  Evaluator func_evaluator(fn_value.func.instructions[0], VM, utils);
  func_evaluator.stack.reserve(100);
  func_evaluator.inside_func = true;
  func_evaluator.returns_ref = fn_value.func.ret_ref;

  if (fn_value.func.params.size() != 0) {
    int i = 0;
    for (const auto &node_list : call.op.func_call.arguments) {
      const Value &arg_val = evaluate_expression(node_list, fn_value.func.params[i].is_ref);
      if (fn_value.func.params[i].is_ref && arg_val.heap_reference == -1) {
        std::string num = std::to_string(i + 1);
        const std::string &msg = "Argument " + num + " expected to be a reference, but value given";
        throw_error(msg);
      }
      VarType arg_type = arg_val.type;
      if (arg_type != utils.var_lut.at(fn_value.func.params[i].type_name)) {
        Value real_val = arg_val;
        if (arg_val.heap_reference != -1) {
          real_val = get_heap_value(arg_val.heap_reference);
          arg_type = real_val.type;
        }
        std::string num = std::to_string(i + 1);
        const std::string &msg = "Argument " + num + " expected to be " + fn_value.func.params[i].type_name + ", but " + stringify(real_val) + " given";
        throw_error(msg);
      }
      auto &var = (func_evaluator.stack[fn_value.func.params[i].param_name] = std::make_shared<Variable>());
      var->type = fn_value.func.params[i].type_name;
      var->val = arg_val;
      i++;
    }
  }
  if (fn.value.is_lvalue()) {
    // push itself onto the new callstack
    func_evaluator.stack[fn.value.reference_name] = stack[fn.value.reference_name];
  }
  if (fn_value.this_ref != -1) {
    // push "this" onto the stack
    auto &var = (func_evaluator.stack["this"] = std::make_shared<Variable>());
    var->type = "obj"; // TODO: check if this is correct??
    var->val.heap_reference = fn_value.this_ref;
  }
  if (fn_value.func.captures) {
    // copy the current callstack to the new callstack
    for (const auto &pair : stack) {
      if (pair.first == "this") continue;
      if (fn.value.is_lvalue() && pair.first == fn.value.reference_name) continue;
      bool contains = false;
      for (const auto &p : fn_value.func.params) {
        if (p.param_name == pair.first) {
          contains = true;
          break;
        }
      }
      if (contains) continue;
      func_evaluator.stack[pair.first] = stack[pair.first];
    }
  }
  const std::string &fn_name = fn.value.is_lvalue() ? fn.value.reference_name : fn_value.func_name;
  VM.trace.push(fn_name, current_line, current_source);
  func_evaluator.start();
  if (fn_value.func.ret_ref) {
    if (func_evaluator.return_value.heap_reference == -1) {
      const std::string &msg = "function returns a reference, but " + stringify(func_evaluator.return_value) + " was returned";
      throw_error(msg);
      return {};
    }
    const Value &heap_val = get_heap_value(func_evaluator.return_value.heap_reference);
    if (heap_val.type != utils.var_lut.at(fn_value.func.ret_type)) {
      const std::string &msg = "function return type is ref " + fn_value.func.ret_type + ", but " + stringify(func_evaluator.return_value) + " was returned";
      throw_error(msg);
      return {};
    }
    VM.trace.pop();
    return {func_evaluator.return_value};
  } else {
    if (func_evaluator.return_value.type != utils.var_lut.at(fn_value.func.ret_type)) {
      const std::string &msg = "function return type is " + fn_value.func.ret_type + ", but " + stringify(func_evaluator.return_value) + " was returned";
      throw_error(msg);
      return {};
    }
    VM.trace.pop();
    return {func_evaluator.return_value};
  }
  return {};
}

void Evaluator::node_to_element(const Node &node, RpnStack &container) {
  assert(node.expr.is_paren() == false);
  if (node.expr.is_operation()) {
    if (node.expr.type == Expression::FUNC_CALL) {
      container.emplace_back(Operator(node.expr.func_call));
    } else if (node.expr.type == Expression::INDEX) {
      container.emplace_back(Operator(node.expr.index));
    } else {
      container.emplace_back(Operator(node.expr.op));
    }
    return;
  } else if (node.expr.type == Expression::BOOL_EXPR) {
    Value val;
    val.type = VarType::BOOL;
    val.boolean_value = node.expr.bool_literal;
    container.emplace_back(val);
  } else if (node.expr.type == Expression::STR_EXPR) {
    Value val;
    val.type = VarType::STR;
    val.string_value = node.expr.string_literal;
    container.emplace_back(val);
  } else if (node.expr.type == Expression::FLOAT_EXPR) {
    Value val;
    val.type = VarType::FLOAT;
    val.float_value = node.expr.float_literal;
    container.emplace_back(val);
  } else if (node.expr.type == Expression::NUM_EXPR) {
    Value val;
    val.type = VarType::INT;
    val.number_value = node.expr.number_literal;
    container.emplace_back(val);
  } else if (node.expr.type == Expression::IDENTIFIER_EXPR) {
    Value val;
    val.type = VarType::ID;
    val.reference_name = node.expr.id_name;
    container.emplace_back(val);
  } else if (node.expr.type == Expression::FUNC_EXPR) {
    container.emplace_back(Value(node.expr.func_expr));
  } else if (node.expr.type == Expression::ARRAY) {
    Value val;
    Value initial_size(Utils::INT);
    std::size_t elemenets_count = 0;
    if (node.expr.array_expressions.size() != 0 && node.expr.array_expressions[0].size() != 0) {
      elemenets_count = node.expr.array_expressions.size();
    }
    initial_size.number_value = elemenets_count;
    if (node.expr.array_size.size() > 0) {
      initial_size = evaluate_expression(node.expr.array_size);
      if (initial_size.type != Utils::INT) {
        throw_error("Number expected, but " + stringify(initial_size) + " found");
      }
      if (initial_size.number_value < 0) {
        throw_error("Array size cannot be negative");
      }
      if (initial_size.number_value < elemenets_count) {
        initial_size.number_value = elemenets_count;
      }
    }
    val.type = VarType::ARR;
    val.array_type = node.expr.array_type;
    if (initial_size.number_value != 0) {
      val.array_values.resize(initial_size.number_value);
    }
    const Utils::VarType &arr_type = utils.var_lut.at(node.expr.array_type);
    for (auto &v : val.array_values) v.type = arr_type;
    int i = 0;
    for (const auto &node_list : node.expr.array_expressions) {
      if (node_list.size() == 0) {
        if (i == 0) {
          break;
        } else {
          throw_error("Empty array element");
        }
      }
      Value &curr_el = val.array_values[i];
      curr_el = evaluate_expression(node_list, node.expr.array_holds_refs);
      if (node.expr.array_holds_refs && curr_el.heap_reference == -1) {
        throw_error("Array holds references, but null or value given");
      }
      if (node.expr.array_holds_refs) {
        if (arr_type != get_heap_value(curr_el.heap_reference).type) {
          const std::string &msg = "Cannot add " + stringify(curr_el) + " to an array of ref " + node.expr.array_type + "s";
          throw_error(msg);
        }
      } else if (curr_el.type != arr_type) {
        const std::string &msg = "Cannot add " + stringify(curr_el) + " to an array of " + node.expr.array_type + "s";
        throw_error(msg);
      }
      i++;
    }
    container.emplace_back(val);
  } else {
    throw_error("Unidentified expression type!\n");
  }
}

std::shared_ptr<Variable> Evaluator::get_reference_by_name(const std::string &name) {
  if (VM.globals.find(name) != VM.globals.end()) {
    throw_error("Trying to access a native function");
  }
  const auto el = stack.find(name);
  if (el == stack.end()) return nullptr;
  return el->second;
}

void Evaluator::set_member(const std::vector<std::string> &members, const NodeList &expression) {
  assert(members.size() > 1);
  const std::string &base = members[0];
  std::shared_ptr<Variable> var = get_reference_by_name(base);
  if (var == nullptr) {
    const std::string &msg = "'" + base + "' is not defined";
    throw_error(msg);
  }
  Value *val = var->val.heap_reference != -1 ? &get_heap_value(var->val.heap_reference) : &var->val;
  std::vector<Value *> references;
  references.reserve(members.size() + 1);
  references.push_back(val);
  int i = 0;
  std::string prev = members[0];
  for (const auto &member : members) {
    if (i++ == 0) continue;
    Value *temp = references.back();
    temp = temp->heap_reference != -1 ? &get_heap_value(temp->heap_reference) : temp;
    if (temp->type != VarType::OBJ) {
      throw_error(stringify(*temp) + "is not an object");
    }
    auto member_it = temp->member_values.find(member);
    if (member_it == temp->member_values.end()) {
      const std::string &msg = prev + " has no member '" + member + "'";
      throw_error(msg);
    }
    references.push_back(&member_it->second);
    prev = member;
  }
  const Value rvalue = evaluate_expression(expression);
  Value *fin = references.back();
  fin = fin->heap_reference != -1 ? &get_heap_value(fin->heap_reference) : fin;
  if (fin->type != rvalue.type) {
    const std::string &msg = "Cannot assign " + stringify(rvalue) + ", incorrect type";
    throw_error(msg);
  }
  *fin = rvalue;
}

void Evaluator::set_index(const Statement &stmt) {
  assert(stmt.indexes.size() > 0);
  assert(stmt.obj_members.size() == 1);
  assert(stmt.expressions.size() == 1);
  std::shared_ptr<Variable> arr = get_reference_by_name(stmt.obj_members[0]);
  if (arr == nullptr) {
    const std::string &msg = "'" + stmt.obj_members[0] + "' is not defined";
    throw_error(msg);
  }
  Value *val = arr->val.heap_reference != -1 ? &get_heap_value(arr->val.heap_reference) : &arr->val;
  std::vector<Value *> references;
  references.reserve(stmt.indexes.size() + 1);
  references.push_back(val);
  for (const auto &index : stmt.indexes) {
    Value *temp = references.back();
    temp = temp->heap_reference != -1 ? &get_heap_value(temp->heap_reference) : temp;
    if (temp->type != VarType::ARR) {
      throw_error(stringify(*temp) + "is not an array");
    }
    const Value index_val = evaluate_expression(index.expr.index);
    if (index_val.type != VarType::INT) {
      const std::string &msg = "Cannot access array with " + stringify(index_val);
      throw_error(msg);
    }
    if (index_val.number_value < 0 || index_val.number_value >= temp->array_values.size()) {
      const std::string &msg = "Index [" + std::to_string(index_val.number_value) + "] out of range";
      throw_error(msg);
    }
    references.push_back(&temp->array_values[index_val.number_value]);
  }
  const Value &rvalue = evaluate_expression(stmt.expressions[0]);
  Value *fin = references.back();
  fin = fin->heap_reference != -1 ? &get_heap_value(fin->heap_reference) : fin;
  if (fin->type != rvalue.type) {
    const std::string &msg = "Cannot assign " + stringify(rvalue) + ", incorrect type";
    throw_error(msg);
  }
  *fin = rvalue;
}

const Value &Evaluator::get_value(const RpnElement &el) {
  if (el.value.is_lvalue()) {
    if (el.value.member_name.size() != 0) {
      return el.value;
    }
    std::shared_ptr<Variable> var = get_reference_by_name(el.value.reference_name);
    if (var == nullptr) {
      const std::string &msg = "'" + el.value.reference_name + "' is not defined";
      throw_error(msg);
    }
    if (var->val.heap_reference > -1) {
      return get_heap_value(var->val.heap_reference);
    }
    return var->val;
  } else if (el.value.heap_reference != -1) {
    return get_heap_value(el.value.heap_reference);
  } else {
    return el.value;
  }
}

Value &Evaluator::get_mut_value(RpnElement &el) {
  if (el.value.is_lvalue()) {
    if (el.value.member_name.size() != 0) {
      return el.value;
    }
    std::shared_ptr<Variable> var = get_reference_by_name(el.value.reference_name);
    if (var == nullptr) {
      const std::string &msg = "'" + el.value.reference_name + "' is not defined";
      throw_error(msg);
    }
    if (var->val.heap_reference > -1) {
      return get_heap_value(var->val.heap_reference);
    }
    return var->val;
  } else if (el.value.heap_reference != -1) {
    return get_heap_value(el.value.heap_reference);
  } else {
    return el.value;
  }
}

Value &Evaluator::get_heap_value(std::int64_t ref) {
  if (ref < 0 || ref >= VM.heap.chunks.size()) {
    throw_error("dereferencing a value that is not on the heap");
  }
  Value *ptr = VM.heap.chunks[ref].data;
  if (ptr == nullptr) {
    throw_error("dereferencing a null pointer");
  }
  return *ptr;
}

void Evaluator::flatten_tree(RpnStack &res, const NodeList &expression_tree) {
  for (const auto &node : expression_tree) {
    if (node.expr.rpn_stack.size() != 0) {
      flatten_tree(res, node.expr.rpn_stack);
    }
    if (node.expr.type != Expression::RPN) {
      node_to_element(node, res);
    }
  }
}