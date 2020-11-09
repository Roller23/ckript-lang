#include "evaluator.hpp"
#include "utils.hpp"
#include "error-handler.hpp"
#include "ckript-vm.hpp"

#include <iostream>
#include <cassert>

#define FLAG_OK 0
#define FLAG_BREAK 1
#define FLAG_CONTINUE 2
#define FLAG_RETURN 3

#define REG(OP, FN) if (token.op.type == Token::OP) {result=FN(x, y);} else

#define BITWISE(OP, NAME)\
  Value val;\
  Value &x_val = get_value(x);\
  Value &y_val = get_value(y);\
  if (x_val.type == VarType::INT && y_val.type == VarType::INT) {\
    std::cout << stringify(x_val) << " " << #OP << " " << stringify(y_val) << "\n";\
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
  ErrorHandler::throw_runtime_error(cause, current_line);
}

void Evaluator::start() {
  for (auto &statement : AST.children) {
    int flag = execute_statement(statement);
    if (flag == FLAG_RETURN) {
      break;
    }
  }
  if (stream) return; // retain callstack
  // empty the callstack
  while (stack.size()) {
    Variable *var = stack.back();
    delete var;
    stack.pop_back();
  }
  if (return_value.type == VarType::UNKNOWN) {
    return_value.type = VarType::VOID;
  }
}

int Evaluator::execute_statement(Node &statement) {
  current_line = statement.stmt.line;
  if (statement.stmt.type == StmtType::NONE) return FLAG_OK;
  std::cout << "Executing a statement\n";
  if (statement.stmt.type == StmtType::EXPR) {
    if (statement.stmt.expressions.size() != 1) return FLAG_OK;
    Value result = evaluate_expression(statement.stmt.expressions.at(0));
    if (stream) {
      std::cout << "< " + stringify(result) << "\n";
    }
    return FLAG_OK;
  }
  if (statement.stmt.type == StmtType::CLASS) {
    register_class(statement.stmt.class_stmt);
    return FLAG_OK;
  }
  if (statement.stmt.type == StmtType::SET) {
    if (statement.stmt.expressions.size() == 0) return FLAG_OK; // might break something
    set_member(statement.stmt.obj_members, statement.stmt.expressions.at(0));
    return FLAG_OK;
  }
  if (statement.stmt.type == StmtType::SET_IDX) {
    set_index(statement.stmt);
    return FLAG_OK;
  }
  if (statement.stmt.type == StmtType::DECL) {
    if (statement.stmt.declaration.size() != 1) return FLAG_OK;
    declare_variable(statement.stmt.declaration.at(0));
  }
  if (statement.stmt.type == StmtType::COMPOUND) {
    if (statement.stmt.statements.size() == 0) return FLAG_OK;
    for (auto &stmt : statement.stmt.statements.at(0).children) {
      int flag = execute_statement(stmt);
      if (flag) return flag;
    }
  }
  if (statement.stmt.type == StmtType::BREAK) {
    if (!nested_loops) {
      throw_error("break statement outside of loops is illegal");
    }
    std::cout << "break loop\n";
    return FLAG_BREAK;
  }
  if (statement.stmt.type == StmtType::CONTINUE) {
    if (!nested_loops) {
      throw_error("continue statement outside of loops is illegal");
    }
    std::cout << "continue loop\n";
    return FLAG_CONTINUE;
  }
  if (statement.stmt.type == StmtType::RETURN) {
    if (!inside_func) {
      throw_error("return statement outside of functions is illegal");
    }
    if (statement.stmt.expressions.size() != 0 && statement.stmt.expressions.at(0).size() != 0) {
      NodeList return_expr = statement.stmt.expressions.at(0);
      return_value = evaluate_expression(return_expr, returns_ref);
    }
    std::cout << "return " + stringify(return_value) << "\n";
    return FLAG_RETURN;
  }
  if (statement.stmt.type == StmtType::WHILE) {
    if (statement.stmt.expressions.size() == 0) return FLAG_OK;
    if (statement.stmt.statements.size() == 0) return FLAG_OK; // might cause bugs
    nested_loops++;
    while (true) {
      NodeList cond = statement.stmt.expressions.at(0); // make a copy
      Value result = evaluate_expression(cond);
      if (result.type != VarType::BOOL) {
        std::string msg = "Expected a boolean value in while statement, found " + stringify(result);
        throw_error(msg);
      }
      if (!result.boolean_value) break;
      Node stmt = statement.stmt.statements.at(0); // make a copy
      int flag = execute_statement(stmt);
      if (flag == FLAG_BREAK) break;
      if (flag == FLAG_RETURN) return flag;
    }
    nested_loops--;
  }
  if (statement.stmt.type == StmtType::FOR) {
    if (statement.stmt.expressions.size() != 3) return FLAG_OK; // TO DO
    if (statement.stmt.statements.size() == 0) return FLAG_OK; // might cause bugs
    evaluate_expression(statement.stmt.expressions.at(0));
    nested_loops++;
    while (true) {
      NodeList cond = statement.stmt.expressions.at(1); // make a copy
      Value result = evaluate_expression(cond);
      if (result.type != VarType::BOOL) {
        std::string msg = "Expected a boolean value in while statement, found " + stringify(result);
        throw_error(msg);
      }
      if (!result.boolean_value) break;
      Node stmt = statement.stmt.statements.at(0); // make a copy
      int flag = execute_statement(stmt);
      if (flag == FLAG_BREAK) break;
      if (flag == FLAG_RETURN) return flag;
      NodeList post_expr = statement.stmt.expressions.at(2);
      evaluate_expression(post_expr);
    }
    nested_loops--;
  }
  if (statement.stmt.type == StmtType::IF) {
    if (statement.stmt.statements.size() == 0) return FLAG_OK; // might cause bugs
    if (statement.stmt.expressions.size() == 0) return FLAG_OK;
    Value result = evaluate_expression(statement.stmt.expressions.at(0));
    if (result.type != VarType::BOOL) {
      std::string msg = "Expected a boolean value in if statement, found " + stringify(result);
      throw_error(msg);
    }
    if (result.boolean_value) {
      int flag = execute_statement(statement.stmt.statements.at(0));
      if (flag) return flag;
    } else {
      if (statement.stmt.statements.size() == 2) {
        int flag = execute_statement(statement.stmt.statements.at(1));
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
            ErrorHandler::throw_syntax_error(msg);
          }
          RpnElement y = res_stack.back();
          res_stack.pop_back();
          RpnElement x = res_stack.back();
          res_stack.pop_back();
          RpnElement result;
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
          res_stack.push_back(result);
        } else if (utils.op_unary(token.op.type)) {
          if (res_stack.size() < 1) {
            std::string msg = "Operator " + Token::get_name(token.op.type) + " an operand"; 
            ErrorHandler::throw_syntax_error(msg);
          }
          RpnElement x = res_stack.back();
          res_stack.pop_back();
          RpnElement result;
          if (token.op.type == Token::OP_NOT) {
            result = logical_not(x);
          } else if (token.op.type == Token::OP_NEG) {
            result = bitwise_not(x);
          } else if (token.op.type == Token::DEL) {
            result = delete_value(x);
          } else {
            std::string msg = "Unknown unary operator " + Token::get_name(token.op.type);
            throw_error(msg);
          }
          res_stack.push_back(result);
        }
      } else if (token.op.op_type == Operator::FUNC) {
        RpnElement fn = res_stack.back();
        res_stack.pop_back();
        RpnElement result = execute_function(token, fn);
        res_stack.push_back(result);
      } else if (token.op.op_type == Operator::INDEX) {
        RpnElement arr = res_stack.back();
        res_stack.pop_back();
        RpnElement result = access_index(arr, token);
        res_stack.push_back(result);
      }
    } else {
      res_stack.push_back(token);
    }
  }
  Value &res_val = res_stack.at(0).value;
  if (get_ref) {
    if (res_val.is_lvalue()) {
      Variable *var = get_reference_by_name(res_val.reference_name);
      if (var == nullptr) {
        std::string msg = res_val.reference_name + " is not defined";
        throw_error(msg);
      }
      if (var->val.heap_reference != -1) {
        return var->val;
      }
    } else if (res_val.heap_reference != -1) {
      return res_val;
    }
  }
  if (res_val.is_lvalue() || res_val.heap_reference > -1) {
    RpnElement wrapper = res_val;
    res_val = get_value(wrapper);
  }
  std::cout << "Expression result = " << stringify(res_val) << "\n";
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
    std::cout << "!" << stringify(x_val) << "\n";
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
    std::cout << "~" << stringify(x_val) << "\n";
    val.type = VarType::INT;
    val.number_value = ~x_val.number_value;
    return {val};
  }
  throw_error("Cannot perform bitwise not on " + stringify(val) + "\n");
  return {};
}

RpnElement Evaluator::delete_value(RpnElement &x) {
  if (!x.value.is_lvalue()) {
    throw_error("Cannot delete an rvalue " + stringify(x.value) + "\n");
  }
  Variable *var = get_reference_by_name(x.value.reference_name);
  if (var == nullptr) {
    throw_error(x.value.reference_name + " is not defined");
  }
  if (var->val.heap_reference == -1) {
    throw_error(x.value.reference_name + " is not allocated on heap");
  }
  if (var->val.heap_reference >= VM.heap.chunks.size()) {
    throw_error("deleting a value that is not on the heap");
  }
  if (VM.heap.chunks.at(var->val.heap_reference).used == false) {
    throw_error("double delete");
  }
  std::cout << "deleting " << x.value.reference_name << "\n";
  VM.heap.free(var);
  Value val;
  val.type = VarType::VOID;
  return {val};
}

RpnElement Evaluator::perform_addition(RpnElement &x, RpnElement &y) {
  Value val;
  Value &x_val = get_value(x);
  Value &y_val = get_value(y);
  if (x_val.type == VarType::ARR) {
    if (y_val.type == utils.var_lut.at(x_val.array_type)) {
      // append to array
      x_val.array_values.push_back(y_val);
      return {x_val};
    } else {
      throw_error("Cannot append " + stringify(y_val) + " an array of " + x_val.array_type + "s");
    }
  }
  if (y_val.type == VarType::ARR) {
    if (x_val.type == utils.var_lut.at(y_val.array_type)) {
      // prepend to array
      y_val.array_values.insert(y_val.array_values.begin(), x_val);
      return {y_val};
    } else {
      throw_error("Cannot prepend " + stringify(x_val) + " an array of " + y_val.array_type + "s");
    }
  }
  if (x_val.type == VarType::STR || y_val.type == VarType::STR) {
    std::string str1 = stringify(x_val);
    std::string str2 = stringify(y_val);
    std::cout << "concating " << str1 << " to " << str2 << "\n";
    val.type = VarType::STR;
    val.string_value = str1 + str2;
    std::cout << "result = " + val.string_value << "\n";
    return {val};
  }
  if (x_val.type == VarType::INT && y_val.type == VarType::INT) {
    std::cout << "adding " << x_val.number_value << " to " << y_val.number_value << "\n";
    val.type = VarType::INT;
    val.number_value = x_val.number_value + y_val.number_value;
    std::cout << "result = " << val.number_value << "\n";
    return {val};
  }
  if (x_val.type == VarType::FLOAT || y_val.type == VarType::FLOAT) {
    double f1 = to_double(x_val);
    double f2 = to_double(y_val);
    val.type = VarType::FLOAT;
    val.float_value = f1 + f2;
    std::cout << "result = " << val.float_value << "\n";
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
    std::cout << "subtracting " << y_val.number_value << " from " << x_val.number_value << "\n";
    val.type = VarType::INT;
    val.number_value = x_val.number_value - y_val.number_value;
    std::cout << "result = " << val.number_value << "\n";
    return {val};
  }
  if (x_val.type == VarType::FLOAT || y_val.type == VarType::FLOAT) {
    double f1 = to_double(x_val);
    double f2 = to_double(y_val);
    std::cout << "subtracting " << f2 << " from " << f1 << "\n";
    val.type = VarType::FLOAT;
    val.float_value = f1 - f2;
    std::cout << "result = " << val.float_value << "\n";
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
    std::cout << "multiplying " << x_val.number_value << " by " << y_val.number_value << "\n";
    val.type = VarType::INT;
    val.number_value = x_val.number_value * y_val.number_value;
    std::cout << "result = " << val.number_value << "\n";
    return {val};
  }
  if (x_val.type == VarType::FLOAT || y_val.type == VarType::FLOAT) {
    double f1 = to_double(x_val);
    double f2 = to_double(y_val);
    std::cout << "multiplying " << f1 << " by " << f2 << "\n";
    val.type = VarType::FLOAT;
    val.float_value = f1 * f2;
    std::cout << "result = " << val.float_value << "\n";
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
    std::cout << "dividing " << x_val.number_value << " by " << y_val.number_value << "\n";
    val.type = VarType::INT;
    val.number_value = x_val.number_value / y_val.number_value;
    std::cout << "result = " << val.number_value << "\n";
    return {val};
  }
  if (x_val.type == VarType::FLOAT || y_val.type == VarType::FLOAT) {
    double f1 = to_double(x_val);
    double f2 = to_double(y_val);
    if (f2 == 0.0f) {
      throw_error("Cannot divide by zero");
    }
    std::cout << "dividing " << f1 << " by " << f2 << "\n";
    val.type = VarType::FLOAT;
    val.float_value = f1 / f2;
    std::cout << "result = " << val.float_value << "\n";
    return {val};
  }
  std::string msg = "Cannot perform subtraction on " + stringify(x_val) + " and " + stringify(y_val);
  throw_error(msg);
  return {};
}

RpnElement Evaluator::perform_modulo(RpnElement &x, RpnElement &y) {
  Value val;
  Value &x_val = get_value(x);
  Value &y_val = get_value(y);
  if (x_val.type == VarType::INT && y_val.type == VarType::INT) {
    std::cout << y_val.number_value << " % " << x_val.number_value << "\n";
    val.type = VarType::INT;
    val.number_value = x_val.number_value % y_val.number_value;
    std::cout << "result = " << val.number_value << "\n";
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
      // vector1.insert( vector1.end(), vector2.begin(), vect
      x_val.array_values.insert(x_val.array_values.end(), y_val.array_values.begin(), y_val.array_values.end());
      return {x_val};
    } else {
      std::string msg = "Cannot concatenate arrays of type " + x_val.array_type + " and " + y_val.array_type;
      throw_error(msg);
    }
  }
  Value val;
  if (x_val.type == VarType::INT && y_val.type == VarType::INT) {
    std::cout << stringify(x_val) << " ^ " << stringify(y_val) << "\n";
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
    std::cout << stringify(x_val) << " && " << stringify(y_val) << "\n";
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
    std::cout << stringify(x_val) << " || " << stringify(y_val) << "\n";
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
    std::string msg = "Cannot assign " + stringify(y_value) + " to " + x_value.reference_name;
    throw_error(msg);
  }
  std::string ref_name = x.value.reference_name;
  x_value = y_value;
  std::cout << "Assigned " + stringify(y_value) + " to " + ref_name + "\n";
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
  if (obj.member_values.find(y.value.reference_name) == obj.member_values.end()) {
    std::string msg = "Object has no member named " + y.value.reference_name;
    throw_error(msg);
  }
  Value &val = obj.member_values.at(y.value.reference_name);
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
  Value &res = array.array_values.at(index.number_value);
  return {res};
}

RpnElement Evaluator::compare_eq(RpnElement &x, RpnElement &y) {
  Value &x_val = get_value(x);
  Value &y_val = get_value(y);
  Value val;
  if (x_val.type == VarType::FLOAT || y_val.type == VarType::FLOAT) {
    double f1 = to_double(x_val);
    double f2 = to_double(y_val);
    std::cout << "is " << f1 << " == " << f2 << "\n";
    val.type = VarType::BOOL;
    val.boolean_value = f1 == f2;
    std::cout << "result = " << stringify(val) << "\n";
    return {val};
  }
  if (x_val.type == VarType::INT && y_val.type == VarType::INT) {
    std::cout << "is " << x_val.number_value << " == " << y_val.number_value << "\n";
    val.type = VarType::BOOL;
    val.boolean_value = x_val.number_value == y_val.number_value;
    std::cout << "result = " << stringify(val) << "\n";
    return {val};
  }
  if (x_val.type == VarType::STR && y_val.type == VarType::STR) {
    std::cout << "is " << x_val.string_value << " == " << y_val.string_value << "\n";
    val.type = VarType::BOOL;
    val.boolean_value = x_val.string_value == y_val.string_value;
    std::cout << "result = " << stringify(val) << "\n";
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
    std::cout << "is " << f1 << " > " << f2 << "\n";
    val.type = VarType::BOOL;
    val.boolean_value = f1 > f2;
    std::cout << "result = " << stringify(val) << "\n";
    return {val};
  }
  if (x_val.type == VarType::INT && y_val.type == VarType::INT) {
    std::cout << "is " << x_val.number_value << " > " << y_val.number_value << "\n";
    val.type = VarType::BOOL;
    val.boolean_value = x_val.number_value > y_val.number_value;
    std::cout << "result = " << stringify(val) << "\n";
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
  if (get_reference_by_name(_class.class_name) != nullptr) {
    std::string msg = _class.class_name + " is already defined";
    throw_error(msg);
  }
  std::cout << "Declaring a class " << _class.class_name << "\n";
  Variable *var = new Variable;
  var->id = _class.class_name;
  var->type = "class";
  var->val.type = VarType::CLASS;
  var->val.members = _class.members;
  stack.push_back(var);
}

void Evaluator::declare_variable(Node &declaration) {
  Declaration &decl = declaration.decl;
  if (get_reference_by_name(decl.id) != nullptr) {
    std::string msg = decl.id + " is already defined";
    throw_error(msg);
  }
  std::cout << "Declaring " + decl.var_type + " " + decl.id + "\n";
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
  std::cout << "Assigning " + stringify(var_val) + " to " + decl.id + "\n";
  if (decl.allocated) {
    assert(decl.reference == false);
    std::cout << "allocating " + decl.id + " on the heap\n";
    Chunk &chunk = VM.heap.allocate();
    Variable *var = new Variable;
    var->val.heap_reference = chunk.heap_reference;
    var->id = decl.id;
    var->type = decl.var_type;
    var->constant = decl.constant;
    *chunk.data = var_val;
    stack.push_back(var);
    return;
  }
  Variable *var = new Variable;
  var->id = decl.id;
  var->type = decl.var_type;
  var->val = var_val;
  var->constant = decl.constant;
  stack.push_back(var);
}

RpnElement Evaluator::construct_object(RpnElement &call, RpnElement &_class) {
  std::cout << "Constructing an object\n";
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
  if (args_counter != class_val.members.size()) {
    std::string msg = _class.value.reference_name + " has " + std::to_string(class_val.members.size());
    msg += " members, " + std::to_string(args_counter) + " given";
    throw_error(msg);
  }
  val.class_name = _class.value.reference_name;
  val.type = VarType::OBJ;
  int i = 0;
  std::vector<std::string> this_mapper;
  for (auto &node_list : call.op.func_call.arguments) {
    std::string num = std::to_string(i + 1);
    Value arg_val = evaluate_expression(node_list, class_val.members.at(i).is_ref);
    Value real_val = arg_val;
    VarType arg_type = arg_val.type;
    if (arg_val.heap_reference != -1) {
      real_val = get_heap_value(arg_val.heap_reference);
      arg_type = real_val.type;
    }
    if (class_val.members.at(i).is_ref && arg_val.heap_reference == -1) {
      std::string msg = "Object argument " + num + " expected to be a reference, but value given";
      throw_error(msg);
    }
    if (arg_type != utils.var_lut.at(class_val.members.at(i).type_name)) {
      std::string msg = "Argument " + num + " expected to be " + class_val.members.at(i).type_name + ", but " + stringify(real_val) + " given";
      throw_error(msg);
    }
    arg_val.is_member = true;
    arg_val.member_name = class_val.members.at(i).param_name;
    val.member_values.insert(std::make_pair(class_val.members.at(i).param_name, arg_val));
    if (arg_val.type == VarType::FUNC) {
      this_mapper.push_back(class_val.members.at(i).param_name);
    }
    i++;
  }
  for (auto &str : this_mapper) {
    // pass the object copy as "this" to object's methods
    val.member_values.at(str).func_this.push_back(val);
  }
  return {val};
}

RpnElement Evaluator::execute_function(RpnElement &call, RpnElement &fn) {
  assert(call.op.op_type == Operator::FUNC);
  if (fn.value.is_lvalue() && VM.globals.find(fn.value.reference_name) != VM.globals.end()) {
    std::vector<Value> call_args;
    call_args.reserve(call.op.func_call.arguments.size());
    for (auto &node_list : call.op.func_call.arguments) {
      if (node_list.size() == 0) break;
      call_args.push_back(evaluate_expression(node_list));
    }
    Value return_val = VM.globals.at(fn.value.reference_name)->execute(call_args, current_line);
    return {return_val};
  }
  Value &fn_value = get_value(fn);
  if (fn_value.type == VarType::CLASS) {
    return construct_object(call, fn);
  }
  if (fn_value.func.instructions.size() == 0) return {}; // might break something
  if (fn_value.type != VarType::FUNC) {
    std::string msg = stringify(fn_value) + " is not a function";
    throw_error(msg);
  }
  int args_counter = 0;
  for (auto &arg : call.op.func_call.arguments) {
    if (arg.size() != 0) {
      args_counter++;
    } else if (args_counter != 0) {
      std::string msg = "Illegal function invocation, missing arguments";
      throw_error(msg);
    }
  }
  if (args_counter != fn_value.func.params.size()) {
    std::string params_expected = std::to_string(fn_value.func.params.size());
    std::string params_given = std::to_string(args_counter);
    std::string msg = stringify(fn_value) + " expects " + params_expected + " argument(s), " + params_given + " given";
    throw_error(msg);
  }

  Node fn_AST = fn_value.func.instructions.at(0);
  Evaluator func_evaluator(fn_AST, VM, utils);
  func_evaluator.stack.reserve(100);
  func_evaluator.inside_func = true;
  func_evaluator.returns_ref = fn_value.func.ret_ref;

  if (fn_value.func.params.size() != 0) {
    int i = 0;
    for (auto &node_list : call.op.func_call.arguments) {
      std::string num = std::to_string(i + 1);
      Value arg_val = evaluate_expression(node_list, fn_value.func.params.at(i).is_ref);
      Value real_val = arg_val;
      VarType arg_type = arg_val.type;
      if (arg_val.heap_reference != -1) {
        real_val = get_heap_value(arg_val.heap_reference);
        arg_type = real_val.type;
      }
      if (fn_value.func.params.at(i).is_ref && arg_val.heap_reference == -1) {
        std::string msg = "Argument " + num + " expected to be a reference, but value given";
        throw_error(msg);
      }
      if (arg_type != utils.var_lut.at(fn_value.func.params.at(i).type_name)) {
        std::string msg = "Argument " + num + " expected to be " + fn_value.func.params.at(i).type_name + ", but " + stringify(real_val) + " given";
        throw_error(msg);
      }
      Variable *var = new Variable;
      var->id = fn_value.func.params.at(i).param_name;
      var->type = fn_value.func.params.at(i).type_name;
      var->val = arg_val;
      func_evaluator.stack.push_back(var);
      i++;
    } 
  }
  if (fn.value.is_lvalue()) {
    // push itself onto the callstack
    Variable *var = new Variable;
    var->id = fn.value.reference_name;
    var->type = VarType::FUNC;
    var->val = fn_value;
    func_evaluator.stack.push_back(var);
  }
  if (fn.value.func_this.size() != 0) {
    // push "this" onto the stack
    Variable *var = new Variable;
    var->id = "this";
    var->type = VarType::OBJ;
    var->val = fn.value.func_this.at(0);
    func_evaluator.stack.push_back(var);
  }
  func_evaluator.start();
  if (fn_value.func.ret_ref) {
    if (func_evaluator.return_value.heap_reference == -1) {
      std::string msg = "function returns a reference, but " + stringify(func_evaluator.return_value) + " was returned";
      throw_error(msg);
      return {};
    }
    return {func_evaluator.return_value};
  } else {
    if (func_evaluator.return_value.type != utils.var_lut.at(fn_value.func.ret_type)) {
      std::string msg = "function return type is " + fn_value.func.ret_type + ", but " + stringify(func_evaluator.return_value) + " was returned";
      throw_error(msg);
      return {};
    }
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
    val.type = VarType::ARR;
    val.array_type = node.expr.array_type;
    val.array_values.reserve(node.expr.array_expressions.size());
    int i = 0;
    for (auto &node_list : node.expr.array_expressions) {
      if (node_list.size() == 0) {
        if (i == 0) {
          break;
        } else {
          throw_error("Empty array element");
        }
      }
      Value array_element = evaluate_expression(node_list);
      if (array_element.type != utils.var_lut.at(node.expr.array_type)) {
        std::string msg = "Cannot add " + stringify(array_element) + " to an array of " + node.expr.array_type + "s";
        throw_error(msg);
      }
      val.array_values.push_back(array_element);
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
  for (auto &var : stack) {
    if (var->id == name) {
      return var;
    }
  }
  return nullptr;
}

void Evaluator::set_member(const std::vector<std::string> &members, NodeList &expression) {
  assert(members.size() > 1);
  std::string base = members.at(0);
  Variable *var = get_reference_by_name(base);
  if (var == nullptr) {
    std::string msg = base + " is not defined";
    throw_error(msg);
  }
  Value *val = var->val.heap_reference != -1 ? &get_heap_value(var->val.heap_reference) : &var->val;
  std::vector<Value *> references;
  references.reserve(members.size() + 1);
  references.push_back(val);
  int i = 0;
  std::string prev = members.at(0);
  for (auto &member : members) {
    if (i++ == 0) continue;
    Value *temp = references.back();
    temp = temp->heap_reference != -1 ? &get_heap_value(temp->heap_reference) : temp;
    if (temp->member_values.find(member) == temp->member_values.end()) {
      std::string msg = prev + " has no member " + member;
      throw_error(msg);
    }
    references.push_back(&temp->member_values.at(member));
    prev = member;
  }
  Value rvalue = evaluate_expression(expression);
  if (references.back()->type != rvalue.type) {
    std::string msg = "Cannot assign " + stringify(rvalue) + ", incorrect type";
    throw_error(msg);
  }
  *references.back() = rvalue;
}

void Evaluator::set_index(Statement &stmt) {
  assert(stmt.indexes.size() > 0);
  assert(stmt.obj_members.size() == 1);
  assert(stmt.expressions.size() == 1);
  Variable *arr = get_reference_by_name(stmt.obj_members.at(0));
  if (arr == nullptr) {
    std::string msg = stmt.obj_members.at(0) + " is not defined";
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
    references.push_back(&temp->array_values.at(index_val.number_value));
  }
  Value rvalue = evaluate_expression(stmt.expressions.at(0));
  if (references.back()->type != rvalue.type) {
    std::string msg = "Cannot assign " + stringify(rvalue) + ", incorrect type";
    throw_error(msg);
  }
  *references.back() = rvalue;
}

Value &Evaluator::get_value(RpnElement &el) {
  if (el.value.is_lvalue()) {
    if (el.value.is_member) {
      return el.value;
    }
    Variable *var = get_reference_by_name(el.value.reference_name);
    if (var == nullptr) {
      std::string msg = el.value.reference_name + " is not defined";
      throw_error(msg);
    }
    std::cout << "var heap reference " << var->val.heap_reference << "\n";
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
  Value *ptr = VM.heap.chunks.at(ref).data;
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
      node.expr.rpn_stack.clear();
    }
    if (node.expr.type != Expression::RPN) {
      res.push_back(node_to_element(node));
    }
  }
  expression_tree.clear();
}