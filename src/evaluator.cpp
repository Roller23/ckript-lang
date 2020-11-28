#include "evaluator.hpp"
#include "utils.hpp"
#include "error-handler.hpp"
#include "CVM.hpp"

#include <iostream>
#include <cassert>
#include <regex>

#define FLAG_OK 0
#define FLAG_BREAK 1
#define FLAG_CONTINUE 2
#define FLAG_RETURN 3

#define REG(OP, FN) if (token.op.type == Token::OP) {res_stack.push_back(FN(x, y));} else

#define BITWISE(OP, NAME)\
  Value val;\
  Value &x_val = get_value(x);\
  Value &y_val = get_value(y);\
  if (x_val.type == VarType::INT && y_val.type == VarType::INT) {\
    val.type = VarType::INT;\
    val.number_value = x_val.number_value OP y_val.number_value;\
    return {val};\
  }\
  std::string msg = "Cannot perform bitwise " NAME " on " + stringify(x_val) + " and " + stringify(y_val);\
  throw_error(msg);\
  return {};

typedef Statement::StmtType StmtType;
typedef Utils::VarType VarType;

void Evaluator::throw_error(const std::string &cause) {
  if (VM.trace.stack.size() == 0)
    return ErrorHandler::throw_runtime_error(cause, current_line);
  std::cout << "Runtime error: " << cause << " (line " << current_line << ")\n";
  std::vector<Value> args;
  native_stacktrace->execute(args, current_line, VM);
  std::exit(EXIT_FAILURE);
}

void Evaluator::start() {
  for (auto &statement : AST.children) {
    int flag = execute_statement(statement);
    if (flag == FLAG_RETURN) {
      break;
    }
  }
  if (stream) return; // retain callstack
  // deallocate variables on callstack
  for (auto &pair : stack) {
    delete pair.second;
  }
  if (return_value.type == VarType::UNKNOWN) {
    return_value.type = VarType::VOID;
  }
}

int Evaluator::execute_statement(Node &statement) {
  current_line = statement.stmt.line;
  if (statement.stmt.type == StmtType::NONE) return FLAG_OK;
  if (statement.stmt.type == StmtType::EXPR) {
    if (statement.stmt.expressions.size() != 1) return FLAG_OK;
    Value result = evaluate_expression(statement.stmt.expressions[0]);
    if (stream && result.type != VarType::VOID) {
      std::cout << "< ";
      std::vector<Value> v(1, result);
      native_println->execute(v, current_line, VM);
    }
    return FLAG_OK;
  }
  if (statement.stmt.type == StmtType::CLASS) {
    register_class(statement.stmt.class_stmt);
    return FLAG_OK;
  }
  if (statement.stmt.type == StmtType::SET) {
    if (statement.stmt.expressions.size() == 0) return FLAG_OK; // might break something
    set_member(statement.stmt.obj_members, statement.stmt.expressions[0]);
    return FLAG_OK;
  }
  if (statement.stmt.type == StmtType::SET_IDX) {
    set_index(statement.stmt);
    return FLAG_OK;
  }
  if (statement.stmt.type == StmtType::DECL) {
    if (statement.stmt.declaration.size() != 1) return FLAG_OK;
    declare_variable(statement.stmt.declaration[0]);
  }
  if (statement.stmt.type == StmtType::COMPOUND) {
    if (statement.stmt.statements.size() == 0) return FLAG_OK;
    for (auto &stmt : statement.stmt.statements[0].children) {
      int flag = execute_statement(stmt);
      if (flag) return flag;
    }
  }
  if (statement.stmt.type == StmtType::BREAK) {
    if (!nested_loops) {
      throw_error("break statement outside of loops is illegal");
    }
    return FLAG_BREAK;
  }
  if (statement.stmt.type == StmtType::CONTINUE) {
    if (!nested_loops) {
      throw_error("continue statement outside of loops is illegal");
    }
    return FLAG_CONTINUE;
  }
  if (statement.stmt.type == StmtType::RETURN) {
    if (!inside_func) {
      throw_error("return statement outside of functions is illegal");
    }
    if (statement.stmt.expressions.size() != 0 && statement.stmt.expressions[0].size() != 0) {
      NodeList return_expr = statement.stmt.expressions[0];
      return_value = evaluate_expression(return_expr, returns_ref);
    }
    return FLAG_RETURN;
  }
  if (statement.stmt.type == StmtType::WHILE) {
    assert(statement.stmt.expressions.size() != 0);
    if (statement.stmt.statements.size() == 0) return FLAG_OK; // might cause bugs
    if (statement.stmt.expressions[0].size() == 0) {
      throw_error("while expects an expression");
    }
    nested_loops++;
    while (true) {
      NodeList cond = statement.stmt.expressions[0]; // make a copy
      Value result = evaluate_expression(cond);
      if (result.type != VarType::BOOL) {
        std::string msg = "Expected a boolean value in while statement, found " + stringify(result);
        throw_error(msg);
      }
      if (!result.boolean_value) break;
      Node stmt = statement.stmt.statements[0]; // make a copy
      int flag = execute_statement(stmt);
      if (flag == FLAG_BREAK) break;
      if (flag == FLAG_RETURN) return flag;
    }
    nested_loops--;
  }
  if (statement.stmt.type == StmtType::FOR) {
    if (statement.stmt.expressions.size() != 3) {
      std::string given = std::to_string(statement.stmt.expressions.size());
      throw_error("For expects 3 expressions, " + given + " given");
    }
    if (statement.stmt.statements.size() == 0) return FLAG_OK; // might cause bugs
    if (statement.stmt.expressions[0].size() != 0) {
      evaluate_expression(statement.stmt.expressions[0]);
    }
    nested_loops++;
    while (true) {
      NodeList cond = statement.stmt.expressions[1]; // make a copy
      Value result;
      if (cond.size() == 0) {
        // empty conditions evaluate to true
        result.type = Utils::BOOL;
        result.boolean_value = true;
      } else {
        result = evaluate_expression(cond);
      }
      if (result.type != VarType::BOOL) {
        std::string msg = "Expected a boolean value in while statement, found " + stringify(result);
        throw_error(msg);
      }
      if (!result.boolean_value) break;
      Node stmt = statement.stmt.statements[0]; // make a copy
      int flag = execute_statement(stmt);
      if (flag == FLAG_BREAK) break;
      if (flag == FLAG_RETURN) return flag;
      NodeList increment_expr = statement.stmt.expressions[2];
      if (increment_expr.size() != 0) {
        evaluate_expression(increment_expr);
      }
    }
    nested_loops--;
  }
  if (statement.stmt.type == StmtType::IF) {
    if (statement.stmt.statements.size() == 0) return FLAG_OK; // might cause bugs
    assert(statement.stmt.expressions.size() != 0);
    if (statement.stmt.expressions[0].size() == 0) {
      throw_error("if expects an expression");
    }
    Value result = evaluate_expression(statement.stmt.expressions[0]);
    if (result.type != VarType::BOOL) {
      std::string msg = "Expected a boolean value in if statement, found " + stringify(result);
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
  }
  return FLAG_OK;
}

Value Evaluator::evaluate_expression(NodeList &expression_tree, bool get_ref) {
  RpnStack rpn_stack;
  flatten_tree(rpn_stack, expression_tree);
  RpnStack res_stack;
  assert(rpn_stack.size() != 0);
  res_stack.reserve(rpn_stack.size() * 3);
  for (auto &token : rpn_stack) {
    if (token.type == RpnElement::OPERATOR) {
      if (token.op.op_type == Operator::BASIC) {
        if (utils.op_binary(token.op.type)) {
          if (res_stack.size() < 2) {
            std::string msg = "Operator " + Token::get_name(token.op.type) + " expects two operands"; 
            throw_error(msg);
          }
          RpnElement y = res_stack.back();
          res_stack.pop_back();
          RpnElement x = res_stack.back();
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
            std::string msg = "Unknown binary operator " + Token::get_name(token.op.type);
            throw_error(msg);
          }
        } else if (utils.op_unary(token.op.type)) {
          if (res_stack.size() < 1) {
            std::string msg = "Operator " + Token::get_name(token.op.type) + "expects one operand"; 
            throw_error(msg);
          }
          RpnElement x = res_stack.back();
          res_stack.pop_back();
          if (token.op.type == Token::OP_NOT) {
            res_stack.push_back(logical_not(x));
          } else if (token.op.type == Token::OP_NEG) {
            res_stack.push_back(bitwise_not(x));
          } else if (token.op.type == Token::DEL) {
            res_stack.push_back(delete_value(x));
          } else {
            std::string msg = "Unknown unary operator " + Token::get_name(token.op.type);
            throw_error(msg);
          }
        }
      } else if (token.op.op_type == Operator::FUNC) {
        RpnElement fn = res_stack.back();
        res_stack.pop_back();
        res_stack.push_back(execute_function(token, fn));
      } else if (token.op.op_type == Operator::INDEX) {
        RpnElement arr = res_stack.back();
        res_stack.pop_back();
        res_stack.push_back(access_index(arr, token));
      }
    } else {
      res_stack.push_back(token);
    }
  }
  Value &res_val = res_stack[0].value;
  if (get_ref) {
    if (res_val.is_lvalue()) {
      Variable *var = get_reference_by_name(res_val.reference_name);
      if (var == nullptr) {
        std::string msg = "'" + res_val.reference_name + "' is not defined";
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
    res_val = get_value(wrapper);
  }
  return res_val;
}

std::string Evaluator::stringify(Value &val) {
  if (val.heap_reference != -1) {
    return "reference to " + stringify(get_heap_value(val.heap_reference));
  }
  if (val.type == VarType::STR) {
    return val.string_value;
  }
  if (val.type == VarType::BOOL) {
    return val.boolean_value ? "true" : "false";
  }
  if (val.type == VarType::FLOAT) {
    return std::to_string(val.float_value);
  }
  if (val.type == VarType::INT) {
    return std::to_string(val.number_value);
  }
  if (val.type == VarType::FUNC) {
    return "function";
  }
  if (val.type == VarType::CLASS) {
    return "class";
  }
  if (val.type == VarType::OBJ) {
    return "object";
  }
  if (val.type == VarType::ARR) {
    return "array";
  }
  if (val.type == VarType::VOID) {
    return "void";
  }
  if (val.type == VarType::UNKNOWN) {
    return "null";
  }
  if (val.type == VarType::ID) {
    return val.reference_name;
  }
  return "";
}

double Evaluator::to_double(Value &val) {
  if (val.type == VarType::FLOAT) {
    return val.float_value;
  }
  if (val.type == VarType::INT) {
    return (double)val.number_value;
  }
  throw_error("Cannot convert " + stringify(val) + " to double");
  return 0;
}

RpnElement Evaluator::logical_not(RpnElement &x) {
  Value &x_val = get_value(x);
  Value val;
  if (x_val.type == VarType::BOOL) {
    val.type = VarType::BOOL;
    val.boolean_value = !x_val.boolean_value;
    return {val};
  }
  throw_error("Cannot perform logical not on " + stringify(val) + "\n");
  return {};
}

RpnElement Evaluator::Evaluator::bitwise_not(RpnElement &x) {
  Value &x_val = get_value(x);
  Value val;
  if (x_val.type == VarType::INT) {
    val.type = VarType::INT;
    val.number_value = ~x_val.number_value;
    return {val};
  }
  throw_error("Cannot perform bitwise not on " + stringify(val) + "\n");
  return {};
}

RpnElement Evaluator::delete_value(RpnElement &x) {
  Value val = x.value;
  Variable *v = nullptr;
  if (val.is_lvalue()) {
    v = get_reference_by_name(val.reference_name);
    if (v == nullptr) {
      throw_error(val.reference_name + " is not defined");
    }
    val = v->val;
  }
  if (val.heap_reference == -1) {
    throw_error(x.value.reference_name + " is not allocated on heap");
  }
  if (val.heap_reference >= VM.heap.chunks.size()) {
    throw_error("deleting a value that is not on the heap");
  }
  if (VM.heap.chunks[val.heap_reference].used == false) {
    throw_error("double delete");
  }
  VM.heap.free(val.heap_reference);
  if (v != nullptr) {
    v->val.heap_reference = -1;
  }
  Value res;
  res.type = VarType::VOID;
  return {res};
}

RpnElement Evaluator::perform_addition(RpnElement &x, RpnElement &y) {
  Value val;
  Value &x_val = get_value(x);
  Value &y_val = get_value(y);
  if (x_val.type == VarType::ARR) {
    if (y_val.type == utils.var_lut.at(x_val.array_type)) {
      // append to array
      Value x_val_cpy = x_val;
      x_val_cpy.array_values.push_back(y_val);
      return {x_val_cpy};
    } else {
      throw_error("Cannot append " + stringify(y_val) + " an array of " + x_val.array_type + "s");
    }
  }
  if (y_val.type == VarType::ARR) {
    if (x_val.type == utils.var_lut.at(y_val.array_type)) {
      // prepend to array
      Value y_val_cpy = y_val;
      y_val_cpy.array_values.insert(y_val.array_values.begin(), x_val);
      return {y_val_cpy};
    } else {
      throw_error("Cannot prepend " + stringify(x_val) + " an array of " + y_val.array_type + "s");
    }
  }
  if (x_val.type == VarType::STR || y_val.type == VarType::STR) {
    std::string str1 = stringify(x_val);
    std::string str2 = stringify(y_val);
    val.type = VarType::STR;
    val.string_value = str1 + str2;
    return {val};
  }
  if (x_val.type == VarType::INT && y_val.type == VarType::INT) {
    val.type = VarType::INT;
    val.number_value = x_val.number_value + y_val.number_value;
    return {val};
  }
  if (x_val.type == VarType::FLOAT || y_val.type == VarType::FLOAT) {
    double f1 = to_double(x_val);
    double f2 = to_double(y_val);
    val.type = VarType::FLOAT;
    val.float_value = f1 + f2;
    return {val};
  }
  std::string msg = "Cannot perform addition on " + stringify(x_val) + " and " + stringify(y_val);
  throw_error(msg);
  return {};
}

RpnElement Evaluator::perform_subtraction(RpnElement &x, RpnElement &y) {
  Value val;
  Value &x_val = get_value(x);
  Value &y_val = get_value(y);
  if (x_val.type == VarType::INT && y_val.type == VarType::INT) {
    val.type = VarType::INT;
    val.number_value = x_val.number_value - y_val.number_value;
    return {val};
  }
  if (x_val.type == VarType::ARR && y_val.type == VarType::INT) {
    // remove from array
    Value x_val_cpy = x_val;
    if (y_val.number_value < 0 || y_val.number_value >= x_val_cpy.array_values.size()) {
      std::string msg = "cannot remove index [" + std::to_string(y_val.number_value) + "] (out of range)";
      throw_error(msg);
    }
    x_val_cpy.array_values.erase(x_val_cpy.array_values.begin() + y_val.number_value);
    return {x_val_cpy};
  }
  if (x_val.type == VarType::FLOAT || y_val.type == VarType::FLOAT) {
    double f1 = to_double(x_val);
    double f2 = to_double(y_val);
    val.type = VarType::FLOAT;
    val.float_value = f1 - f2;
    return {val};
  }
  std::string msg = "Cannot perform subtraction on " + stringify(x_val) + " and " + stringify(y_val);
  throw_error(msg);
  return {};
}

RpnElement Evaluator::perform_multiplication(RpnElement &x, RpnElement &y) {
  Value val;
  Value &x_val = get_value(x);
  Value &y_val = get_value(y);
  if (x_val.type == VarType::INT && y_val.type == VarType::INT) {
    val.type = VarType::INT;
    val.number_value = x_val.number_value * y_val.number_value;
    return {val};
  }
  if (x_val.type == VarType::FLOAT || y_val.type == VarType::FLOAT) {
    double f1 = to_double(x_val);
    double f2 = to_double(y_val);
    val.type = VarType::FLOAT;
    val.float_value = f1 * f2;
    return {val};
  }
  std::string msg = "Cannot perform multiplication on " + stringify(x_val) + " and " + stringify(y_val);
  throw_error(msg);
  return {};
}

RpnElement Evaluator::perform_division(RpnElement &x, RpnElement &y) {
  Value val;
  Value &x_val = get_value(x);
  Value &y_val = get_value(y);
  if (x_val.type == VarType::INT && y_val.type == VarType::INT) {
    if (y_val.number_value == 0) {
      throw_error("Cannot divide by zero");
    }
    val.type = VarType::INT;
    val.number_value = x_val.number_value / y_val.number_value;
    return {val};
  }
  if (x_val.type == VarType::FLOAT || y_val.type == VarType::FLOAT) {
    double f1 = to_double(x_val);
    double f2 = to_double(y_val);
    if (f2 == 0.0f) {
      throw_error("Cannot divide by zero");
    }
    val.type = VarType::FLOAT;
    val.float_value = f1 / f2;
    return {val};
  }
  std::string msg = "Cannot perform division on " + stringify(x_val) + " and " + stringify(y_val);
  throw_error(msg);
  return {};
}

RpnElement Evaluator::perform_modulo(RpnElement &x, RpnElement &y) {
  Value val;
  Value &x_val = get_value(x);
  Value &y_val = get_value(y);
  if (x_val.type == VarType::INT && y_val.type == VarType::INT) {
    if (y_val.number_value == 0) {
      throw_error("Cannot divide by 0");
    }
    val.type = VarType::INT;
    val.number_value = x_val.number_value % y_val.number_value;
    return {val};
  }
  std::string msg = "Cannot perform modulo on " + stringify(x_val) + " and " + stringify(y_val);
  throw_error(msg);
  return {};
}

RpnElement Evaluator::bitwise_and(RpnElement &x, RpnElement &y) {
  BITWISE(&, "and")
}

RpnElement Evaluator::bitwise_or(RpnElement &x, RpnElement &y) {
  BITWISE(|, "or")
}

RpnElement Evaluator::bitwise_xor(RpnElement &x, RpnElement &y) {
  Value &x_val = get_value(x);
  Value &y_val = get_value(y);
  if (x_val.type == VarType::ARR && y_val.type == VarType::ARR) {
    // concat arrays
    if (x_val.array_type == y_val.array_type) {
      Value x_val_cpy = x_val;
      x_val_cpy.array_values.insert(x_val.array_values.end(), y_val.array_values.begin(), y_val.array_values.end());
      return {x_val_cpy};
    } else {
      std::string msg = "Cannot concatenate arrays of type " + x_val.array_type + " and " + y_val.array_type;
      throw_error(msg);
    }
  }
  Value val;
  if (x_val.type == VarType::INT && y_val.type == VarType::INT) {
    val.type = VarType::INT;
    val.number_value = x_val.number_value ^ y_val.number_value;
    return {val};
  }
  std::string msg = "Cannot perform bitwise xor on " + stringify(x_val) + " and " + stringify(y_val);
  throw_error(msg);
  return {};
}

RpnElement Evaluator::shift_left(RpnElement &x, RpnElement &y) {
  BITWISE(<<, "left shift")
}

RpnElement Evaluator::shift_right(RpnElement &x, RpnElement &y) {
  BITWISE(>>, "right shift")
}

RpnElement Evaluator::logical_and(RpnElement &x, RpnElement &y) {
  Value val;
  Value &x_val = get_value(x);
  Value &y_val = get_value(y);
  if (x_val.type == VarType::BOOL && y_val.type == VarType::BOOL) {
    val.type = VarType::BOOL;
    val.boolean_value = x_val.boolean_value && y_val.boolean_value;
    return {val};
  }
  std::string msg = "Cannot perform logical and on " + stringify(x_val) + " and " + stringify(y_val);
  throw_error(msg);
  return {};
}

RpnElement Evaluator::logical_or(RpnElement &x, RpnElement &y) {
  Value val;
  Value &x_val = get_value(x);
  Value &y_val = get_value(y);
  if (x_val.type == VarType::BOOL && y_val.type == VarType::BOOL) {
    val.type = VarType::BOOL;
    val.boolean_value = x_val.boolean_value || y_val.boolean_value;
    return {val};
  }
  std::string msg = "Cannot perform logical or on " + stringify(x_val) + " and " + stringify(y_val);
  throw_error(msg);
  return {};
}

RpnElement Evaluator::assign(RpnElement &x, RpnElement &y) {
  if (!x.value.is_lvalue()) {
    std::string msg = "Cannot assign to an rvalue";
    throw_error(msg);
  }
  Variable *var = get_reference_by_name(x.value.reference_name);
  if (var == nullptr) {
    std::string msg = x.value.reference_name + " is not defined";
    throw_error(msg);
  }
  if (var->constant) {
    std::string msg = "Cannot reassign a constant variable (" + x.value.reference_name + ")";
    throw_error(msg);
  }
  Value &x_value = get_value(x);
  Value y_value = get_value(y);
  if (x_value.type == VarType::UNKNOWN) {
    std::string msg = x_value.reference_name + " doesn't point to anything on the heap";
    throw_error(msg);
  }
  if (x_value.type != y_value.type) {
    std::string msg = "Cannot assign " + stringify(y_value) + " to " + x.value.reference_name;
    throw_error(msg);
  }
  std::string ref_name = x.value.reference_name;
  x_value = y_value;
  return {x_value};
}

RpnElement Evaluator::plus_assign(RpnElement &x, RpnElement &y) {
  RpnElement rvalue = perform_addition(x, y);
  return assign(x, rvalue);
}

RpnElement Evaluator::minus_assign(RpnElement &x, RpnElement &y) {
  RpnElement rvalue = perform_subtraction(x, y);
  return assign(x, rvalue);
}

RpnElement Evaluator::mul_assign(RpnElement &x, RpnElement &y) {
  RpnElement rvalue = perform_multiplication(x, y);
  return assign(x, rvalue);
}

RpnElement Evaluator::div_assign(RpnElement &x, RpnElement &y) {
  RpnElement rvalue = perform_division(x, y);
  return assign(x, rvalue);
}

RpnElement Evaluator::mod_assign(RpnElement &x, RpnElement &y) {
  RpnElement rvalue = perform_modulo(x, y);
  return assign(x, rvalue);
}

RpnElement Evaluator::lshift_assign(RpnElement &x, RpnElement &y) {
  RpnElement rvalue = shift_left(x, y);
  return assign(x, rvalue);
}

RpnElement Evaluator::rshift_assign(RpnElement &x, RpnElement &y) {
  RpnElement rvalue = shift_right(x, y);
  return assign(x, rvalue);
}

RpnElement Evaluator::and_assign(RpnElement &x, RpnElement &y) {
  RpnElement rvalue = bitwise_and(x, y);
  return assign(x, rvalue);
}

RpnElement Evaluator::or_assign(RpnElement &x, RpnElement &y) {
  RpnElement rvalue = bitwise_or(x, y);
  return assign(x, rvalue);
}

RpnElement Evaluator::xor_assign(RpnElement &x, RpnElement &y) {
  RpnElement rvalue = bitwise_xor(x, y);
  return assign(x, rvalue);
}

RpnElement Evaluator::access_member(RpnElement &x, RpnElement &y) {
  if (!y.value.is_lvalue()) {
    throw_error("Object members can only be accessed with lvalues");
  }
  Value &obj = get_value(x);
  if (obj.type != VarType::OBJ) {
    std::string msg = stringify(obj) + " is not an object";
    throw_error(msg);
  }
  auto member_it = obj.member_values.find(y.value.reference_name);
  if (member_it == obj.member_values.end()) {
    std::string object_name = x.value.is_lvalue() ? " " + x.value.reference_name + " " : " ";
    std::string msg = "Object" + object_name + "has no member named " + y.value.reference_name;
    throw_error(msg);
  }
  Value &val = member_it->second;
  if (val.type == Utils::FUNC) {
    val.func_name = y.value.reference_name;
  }
  return {val};
}

RpnElement Evaluator::access_index(RpnElement &arr, RpnElement &idx) {
  Value &array = get_value(arr);
  if (array.type != VarType::ARR) {
    std::string msg = stringify(array) + " is not an array";
    throw_error(msg);
  }
  Value index = evaluate_expression(idx.op.index_rpn);
  if (index.type != VarType::INT) {
    std::string msg = "index expected to be an int, but " + stringify(index) + " found";
    throw_error(msg);
  }
  if (index.number_value < 0 || index.number_value >= array.array_values.size()) {
    std::string msg = "index [" + std::to_string(index.number_value) + "] out of range";
    throw_error(msg);
  }
  Value &res = array.array_values[index.number_value];
  return {res};
}

RpnElement Evaluator::compare_eq(RpnElement &x, RpnElement &y) {
  Value &x_val = get_value(x);
  Value &y_val = get_value(y);
  Value val;
  if (x_val.type == VarType::FLOAT || y_val.type == VarType::FLOAT) {
    double f1 = to_double(x_val);
    double f2 = to_double(y_val);
    val.type = VarType::BOOL;
    val.boolean_value = f1 == f2;
    return {val};
  }
  if (x_val.type == VarType::INT && y_val.type == VarType::INT) {
    val.type = VarType::BOOL;
    val.boolean_value = x_val.number_value == y_val.number_value;
    return {val};
  }
  if (x_val.type == VarType::STR && y_val.type == VarType::STR) {
    val.type = VarType::BOOL;
    val.boolean_value = x_val.string_value == y_val.string_value;
    return {val};
  }
  if (x_val.type == VarType::BOOL && y_val.type == VarType::BOOL) {
    val.type = VarType::BOOL;
    val.boolean_value = x_val.boolean_value == y_val.boolean_value;
    return {val};
  }
  std::string msg = "Cannot compare " + stringify(x_val) + " to " + stringify(y_val);
  throw_error(msg);
  return {};
}

RpnElement Evaluator::compare_neq(RpnElement &x, RpnElement &y) {
  RpnElement eq = compare_eq(x, y);
  Value val;
  val.type = VarType::BOOL;
  val.boolean_value = !eq.value.boolean_value;
  return {val};
}

RpnElement Evaluator::compare_gt(RpnElement &x, RpnElement &y) {
  Value &x_val = get_value(x);
  Value &y_val = get_value(y);
  Value val;
  if (x_val.type == VarType::FLOAT || y_val.type == VarType::FLOAT) {
    double f1 = to_double(x_val);
    double f2 = to_double(y_val);
    val.type = VarType::BOOL;
    val.boolean_value = f1 > f2;
    return {val};
  }
  if (x_val.type == VarType::INT && y_val.type == VarType::INT) {
    val.type = VarType::BOOL;
    val.boolean_value = x_val.number_value > y_val.number_value;
    return {val};
  }
  std::string msg = "Cannot compare " + stringify(x_val) + " to " + stringify(y_val);
  throw_error(msg);
  return {};
}

RpnElement Evaluator::compare_lt(RpnElement &x, RpnElement &y) {
  return compare_gt(y, x);
}

RpnElement Evaluator::compare_gt_eq(RpnElement &x, RpnElement &y) {
  RpnElement gt = compare_gt(x, y);
  RpnElement eq = compare_eq(x, y);
  Value val;
  val.type = VarType::BOOL;
  val.boolean_value = gt.value.boolean_value || eq.value.boolean_value;
  return {val};
}

RpnElement Evaluator::compare_lt_eq(RpnElement &x, RpnElement &y) {
  return compare_gt_eq(y, x);
}

void Evaluator::register_class(ClassStatement &_class) {
  Variable *v = get_reference_by_name(_class.class_name);
  if (v != nullptr) {
    delete v;
  }
  Variable *var = new Variable;
  var->type = "class";
  var->val.type = VarType::CLASS;
  var->val.members = _class.members;
  var->val.class_name = _class.class_name;
  stack[_class.class_name] = var;
}

void Evaluator::declare_variable(Node &declaration) {
  Declaration &decl = declaration.decl;
  Value var_val = evaluate_expression(decl.var_expr, decl.reference);
  Utils::VarType var_type = utils.var_lut.at(decl.var_type);
  Utils::VarType expr_type = var_val.type;
  if (decl.reference) {
    expr_type = get_heap_value(var_val.heap_reference).type;
  }
  if (var_type != expr_type) {
    std::string msg = "Cannot assign " + stringify(var_val) + " to a variable of type " + decl.var_type;
    throw_error(msg);
  }
  Variable *v = get_reference_by_name(decl.id);
  if (v != nullptr) {
    // to avoid memory leaks while redeclaring
    delete v;
  }
  if (decl.allocated) {
    assert(decl.reference == false);
    Chunk &chunk = VM.heap.allocate();
    Variable *var = new Variable;
    var->val.heap_reference = chunk.heap_reference;
    var->type = decl.var_type;
    var->constant = decl.constant;
    *chunk.data = var_val;
    stack[decl.id] = var;
    if (var_val.type == Utils::OBJ) {
      // bind the reference to the object to 'this' in its methods
      std::vector<Value> args(1, var->val);
      native_bind->execute(args, current_line, VM);
    }
    return;
  }
  Variable *var = new Variable;
  var->type = decl.var_type;
  var->val = var_val;
  var->constant = decl.constant;
  stack[decl.id] = var;
}

RpnElement Evaluator::construct_object(RpnElement &call, RpnElement &_class) {
  Value val;
  Value &class_val = get_value(_class);
  int args_counter = 0;
  for (auto &arg : call.op.func_call.arguments) {
    if (arg.size() != 0) {
      args_counter++;
    } else {
      std::string msg = "Illegal object invocation, missing members";
      throw_error(msg);
    }
  }
  std::size_t members_count = class_val.members.size();
  if (args_counter != members_count) {
    std::string msg = _class.value.reference_name + " has " + std::to_string(class_val.members.size());
    msg += " members, " + std::to_string(args_counter) + " given";
    throw_error(msg);
  }
  val.class_name = _class.value.reference_name;
  val.type = VarType::OBJ;
  val.members.reserve(members_count);
  int i = 0;
  for (auto &node_list : call.op.func_call.arguments) {
    std::string num = std::to_string(i + 1);
    FuncParam &member = class_val.members[i];
    Value arg_val = evaluate_expression(node_list, member.is_ref);
    Value real_val = arg_val;
    VarType arg_type = arg_val.type;
    if (arg_val.heap_reference != -1) {
      real_val = get_heap_value(arg_val.heap_reference);
      arg_type = real_val.type;
    }
    if (member.is_ref && arg_val.heap_reference == -1) {
      std::string msg = "Object argument " + num + " expected to be a reference, but value given";
      throw_error(msg);
    }
    if (arg_type != utils.var_lut.at(member.type_name)) {
      std::string msg = "Argument " + num + " expected to be " + member.type_name + ", but " + stringify(real_val) + " given";
      throw_error(msg);
    }
    arg_val.member_name = member.param_name;
    val.member_values.insert(std::make_pair(member.param_name, arg_val));
    i++;
  }
  return {val};
}

RpnElement Evaluator::execute_function(RpnElement &call, RpnElement &fn) {
  assert(call.op.op_type == Operator::FUNC);
  auto global_it = VM.globals.find(fn.value.reference_name);
  if (fn.value.is_lvalue() && global_it != VM.globals.end()) {
    std::vector<Value> call_args;
    call_args.reserve(call.op.func_call.arguments.size());
    bool needs_ref = fn.value.reference_name == "bind";
    for (auto &node_list : call.op.func_call.arguments) {
      if (node_list.size() == 0) break;
      call_args.push_back(evaluate_expression(node_list, needs_ref));
    }
    VM.trace.push(fn.value.reference_name, current_line);
    Value return_val = global_it->second->execute(call_args, current_line, VM);
    VM.trace.pop();
    return {return_val};
  }
  Value &fn_value = get_value(fn);
  if (fn_value.type == VarType::CLASS) {
    return construct_object(call, fn);
  }
  if (fn_value.type == VarType::STR) {
    // string interpolation
    int args = 0;
    for (auto &arg : call.op.func_call.arguments) {
      if (arg.size() != 0) {
        args++;
      } else if (args != 0) {
        throw_error("Illegal string interpolation, missing arguments");
      }
    }
    Value str = fn_value;
    if (args == 0) return {str};
    int argn = 1;
    for (auto &arg : call.op.func_call.arguments) {
      Value arg_val = evaluate_expression(arg);
      std::string find = "@" + std::to_string(argn);
      str.string_value = std::regex_replace(str.string_value, std::regex(find), VM.stringify(arg_val));
      argn++;
    }
    return {str};
  }
  if (fn_value.type != VarType::FUNC) {
    std::string msg = stringify(fn_value) + " is not a function";
    throw_error(msg);
  }
  if (fn_value.func.instructions.size() == 0) return {};
  int args_counter = 0;
  for (auto &arg : call.op.func_call.arguments) {
    if (arg.size() != 0) {
      args_counter++;
    } else if (args_counter != 0) {
      throw_error("Illegal function invocation, missing arguments");
    }
  }
  if (args_counter != fn_value.func.params.size()) {
    std::string params_expected = std::to_string(fn_value.func.params.size());
    std::string params_given = std::to_string(args_counter);
    std::string msg = stringify(fn_value) + " expects " + params_expected + " argument(s), " + params_given + " given";
    throw_error(msg);
  }

  Node fn_AST = fn_value.func.instructions[0];
  Evaluator func_evaluator(fn_AST, VM, utils);
  func_evaluator.stack.reserve(100);
  func_evaluator.inside_func = true;
  func_evaluator.returns_ref = fn_value.func.ret_ref;

  if (fn_value.func.params.size() != 0) {
    int i = 0;
    for (auto &node_list : call.op.func_call.arguments) {
      std::string num = std::to_string(i + 1);
      Value arg_val = evaluate_expression(node_list, fn_value.func.params[i].is_ref);
      Value real_val = arg_val;
      VarType arg_type = arg_val.type;
      if (arg_val.heap_reference != -1) {
        real_val = get_heap_value(arg_val.heap_reference);
        arg_type = real_val.type;
      }
      if (fn_value.func.params[i].is_ref && arg_val.heap_reference == -1) {
        std::string msg = "Argument " + num + " expected to be a reference, but value given";
        throw_error(msg);
      }
      if (arg_type != utils.var_lut.at(fn_value.func.params[i].type_name)) {
        std::string msg = "Argument " + num + " expected to be " + fn_value.func.params[i].type_name + ", but " + stringify(real_val) + " given";
        throw_error(msg);
      }
      Variable *var = new Variable;
      var->type = fn_value.func.params[i].type_name;
      var->val = arg_val;
      func_evaluator.stack[fn_value.func.params[i].param_name] = var;
      i++;
    }
  }
  if (fn.value.is_lvalue()) {
    // push itself onto the callstack
    Variable *var = new Variable;
    var->type = VarType::FUNC;
    var->val = fn_value;
    func_evaluator.stack[fn.value.reference_name] = var;
  }
  if (fn_value.this_ref != -1) {
    // push "this" onto the stack
    Variable *var = new Variable;
    var->type = VarType::OBJ;
    var->val.heap_reference = fn_value.this_ref;
    func_evaluator.stack["this"] = var;
  }
  if (fn_value.func.captures) {
    // copy current callstack on the new callstack
    for (auto &pair : stack) {
      if (pair.first == "this") continue;
      if (fn.value.is_lvalue() && pair.first == fn.value.reference_name) continue;
      bool contains = false;
      for (auto &p : fn_value.func.params) {
        if (p.param_name == pair.first) {
          contains = true;
          break;
        }
      }
      if (contains) continue;
      Variable *copy = new Variable;
      *copy = *pair.second;
      func_evaluator.stack[pair.first] = copy;
    }
  }
  const std::string &fn_name = fn.value.is_lvalue() ? fn.value.reference_name : fn_value.func_name;
  VM.trace.push(fn_name, current_line);
  func_evaluator.start();
  if (fn_value.func.ret_ref) {
    if (func_evaluator.return_value.heap_reference == -1) {
      std::string msg = "function returns a reference, but " + stringify(func_evaluator.return_value) + " was returned";
      throw_error(msg);
      return {};
    }
    Value heap_val = get_heap_value(func_evaluator.return_value.heap_reference);
    if (heap_val.type != utils.var_lut.at(fn_value.func.ret_type)) {
      std::string msg = "function return type is ref " + fn_value.func.ret_type + ", but " + stringify(func_evaluator.return_value) + " was returned";
      throw_error(msg);
      return {};
    }
    VM.trace.pop();
    return {func_evaluator.return_value};
  } else {
    if (func_evaluator.return_value.type != utils.var_lut.at(fn_value.func.ret_type)) {
      std::string msg = "function return type is " + fn_value.func.ret_type + ", but " + stringify(func_evaluator.return_value) + " was returned";
      throw_error(msg);
      return {};
    }
    VM.trace.pop();
    return {func_evaluator.return_value};
  }
  return {};
}

RpnElement Evaluator::node_to_element(Node &node) {
  assert(node.expr.is_paren() == false);
  if (node.expr.is_operation()) {
    if (node.expr.type == Expression::FUNC_CALL) {
      return {Operator(node.expr.func_call)};
    }
    if (node.expr.type == Expression::INDEX) {
      return {Operator(node.expr.index)};
    }
    return {Operator(node.expr.op)};
  }
  Value val;
  if (node.expr.type == Expression::BOOL_EXPR) {
    val.type = VarType::BOOL;
    val.boolean_value = node.expr.bool_literal;
    return {val};
  }
  if (node.expr.type == Expression::STR_EXPR) {
    val.type = VarType::STR;
    val.string_value = node.expr.string_literal;
    return {val};
  }
  if (node.expr.type == Expression::FLOAT_EXPR) {
    val.type = VarType::FLOAT;
    val.float_value = node.expr.float_literal;
    return {val};
  }
  if (node.expr.type == Expression::NUM_EXPR) {
    val.type = VarType::INT;
    val.number_value = node.expr.number_literal;
    return {val};
  }
  if (node.expr.type == Expression::IDENTIFIER_EXPR) {
    val.type = VarType::ID;
    val.reference_name = node.expr.id_name;
    return {val};
  }
  if (node.expr.type == Expression::FUNC_EXPR) {
    val.type = VarType::FUNC;
    val.func = node.expr.func_expr;
    return {val};
  }
  if (node.expr.type == Expression::ARRAY) {
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
    Utils::VarType arr_type = utils.var_lut.at(node.expr.array_type);
    for (auto &v : val.array_values) v.type = arr_type;
    int i = 0;
    for (auto &node_list : node.expr.array_expressions) {
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
          std::string msg = "Cannot add " + stringify(curr_el) + " to an array of ref " + node.expr.array_type + "s";
          throw_error(msg);
        }
      } else if (curr_el.type != arr_type) {
        std::string msg = "Cannot add " + stringify(curr_el) + " to an array of " + node.expr.array_type + "s";
        throw_error(msg);
      }
      i++;
    }
    return {val};
  }
  throw_error("Unidentified expression type!\n");
  return {};
}

Variable *Evaluator::get_reference_by_name(const std::string &name) {
  if (VM.globals.find(name) != VM.globals.end()) {
    throw_error("Trying to access a native function");
  }
  auto el = stack.find(name);
  if (el == stack.end()) return nullptr;
  return el->second;
}

void Evaluator::set_member(const std::vector<std::string> &members, NodeList &expression) {
  assert(members.size() > 1);
  std::string base = members[0];
  Variable *var = get_reference_by_name(base);
  if (var == nullptr) {
    std::string msg = "'" + base + "' is not defined";
    throw_error(msg);
  }
  Value *val = var->val.heap_reference != -1 ? &get_heap_value(var->val.heap_reference) : &var->val;
  std::vector<Value *> references;
  references.reserve(members.size() + 1);
  references.push_back(val);
  int i = 0;
  std::string prev = members[0];
  for (auto &member : members) {
    if (i++ == 0) continue;
    Value *temp = references.back();
    temp = temp->heap_reference != -1 ? &get_heap_value(temp->heap_reference) : temp;
    auto member_it = temp->member_values.find(member);
    if (member_it == temp->member_values.end()) {
      std::string msg = prev + " has no member '" + member + "'";
      throw_error(msg);
    }
    references.push_back(&member_it->second);
    prev = member;
  }
  Value rvalue = evaluate_expression(expression);
  Value *fin = references.back();
  fin = fin->heap_reference != -1 ? &get_heap_value(fin->heap_reference) : fin;
  if (fin->type != rvalue.type) {
    std::string msg = "Cannot assign " + stringify(rvalue) + ", incorrect type";
    throw_error(msg);
  }
  *fin = rvalue;
}

void Evaluator::set_index(Statement &stmt) {
  assert(stmt.indexes.size() > 0);
  assert(stmt.obj_members.size() == 1);
  assert(stmt.expressions.size() == 1);
  Variable *arr = get_reference_by_name(stmt.obj_members[0]);
  if (arr == nullptr) {
    std::string msg = "'" + stmt.obj_members[0] + "' is not defined";
    throw_error(msg);
  }
  Value *val = arr->val.heap_reference != -1 ? &get_heap_value(arr->val.heap_reference) : &arr->val;
  std::vector<Value *> references;
  references.reserve(stmt.indexes.size() + 1);
  references.push_back(val);
  for (auto &index : stmt.indexes) {
    Value *temp = references.back();
    temp = temp->heap_reference != -1 ? &get_heap_value(temp->heap_reference) : temp;
    Value index_val = evaluate_expression(index.expr.index);
    if (index_val.type != VarType::INT) {
      std::string msg = "Cannot access array with " + stringify(index_val);
      throw_error(msg);
    }
    if (index_val.number_value < 0 || index_val.number_value >= temp->array_values.size()) {
      std::string msg = "Index [" + std::to_string(index_val.number_value) + "] out of range";
      throw_error(msg);
    }
    references.push_back(&temp->array_values[index_val.number_value]);
  }
  Value rvalue = evaluate_expression(stmt.expressions[0]);
  Value *fin = references.back();
  fin = fin->heap_reference != -1 ? &get_heap_value(fin->heap_reference) : fin;
  if (fin->type != rvalue.type) {
    std::string msg = "Cannot assign " + stringify(rvalue) + ", incorrect type";
    throw_error(msg);
  }
  *fin = rvalue;
}

Value &Evaluator::get_value(RpnElement &el) {
  if (el.value.is_lvalue()) {
    if (el.value.member_name.size() != 0) {
      return el.value;
    }
    Variable *var = get_reference_by_name(el.value.reference_name);
    if (var == nullptr) {
      std::string msg = "'" + el.value.reference_name + "' is not defined";
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
    std::string msg = "dereferencing a value that is not on the heap";
    throw_error(msg);
  }
  Value *ptr = VM.heap.chunks[ref].data;
  if (ptr == nullptr) {
    std::string msg = "dereferencing a null pointer";
    throw_error(msg);
  }
  return *ptr;
}

void Evaluator::flatten_tree(RpnStack &res, NodeList &expression_tree) {
  for (auto &node : expression_tree) {
    if (node.expr.rpn_stack.size() != 0) {
      flatten_tree(res, node.expr.rpn_stack);
    }
    if (node.expr.type != Expression::RPN) {
      res.push_back(node_to_element(node));
    }
  }
}