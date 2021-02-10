#include "parser.hpp"
#include "AST.hpp"
#include "token.hpp"
#include "error-handler.hpp"

#include <vector>
#include <iostream>
#include <unordered_set>

typedef Expression::ExprType ExprType;
typedef Declaration::DeclType DeclType;
typedef Statement::StmtType StmtType;
typedef Node::NodeType NodeType;
typedef Token::TokenType TokenType;

void Parser::throw_error(const std::string &cause, std::uint32_t line) {
  ErrorHandler::throw_syntax_error(cause, line);
}

void Parser::fail_if_EOF(TokenType expected) {
  if (curr_token.type == Token::NONE) {
    throw_error("Reached end of file but " + Token::get_name(expected) + " expected", 0);
  }
}

void Parser::advance(void) {
  pos++;
  prev = tokens[pos - 1];
  if (pos < tokens_count) {
    curr_token = tokens[pos];
  } else {
    curr_token = Token();
    pos--;
  }
}

void Parser::retreat(void) {
  if (pos == 0) return;
  pos--;
  curr_token = tokens[pos];
  prev = pos > 0 ? tokens[pos - 1] : Token();
}

Token Parser::lookahead(int offset) {
  if (pos + offset < 0) return {};
  if (pos + offset >= tokens.size()) return {};
  return tokens[pos + offset];
}

int Parser::find_enclosing_brace(int start_pos, int braces) {
  int i = 0;
  int size = tokens.size();
  while (true) {
    if (size == i + start_pos) {
      std::string msg = "Invalid function declaration, no enclosing brace found";
      throw_error(msg, tokens[start_pos + i - 1].line);
    }
    if (tokens[start_pos + i].type == Token::LEFT_BRACE) {
      braces++;
    }
    if (tokens[start_pos + i].type == Token::RIGHT_BRACE) {
      braces--;
      if (braces == 0) {
        return i;
      }
    }
    i++;
  }
}

int Parser::find_enclosing_paren() {
  int start_pos = pos;
  int i = 0;
  int size = tokens.size();
  int lparen = 1;
  while (true) {
    if (size == i) {
      std::string msg = "Invalid expression, no enclosing parenthesis found";
      throw_error(msg, tokens[start_pos + i - 1].line);
    }
    if (tokens[start_pos + i].type == Token::LEFT_PAREN) {
      lparen++;
    }
    if (tokens[start_pos + i].type == Token::RIGHT_PAREN) {
      lparen--;
      if (lparen == 0) {
        return i;
      }
    }
    i++;
  }
}

int Parser::find_block_end(void) {
  return find_enclosing_brace(pos);
}

ParamList Parser::parse_func_params(bool is_class) {
  ParamList res;
  TokenType stop = Token::RIGHT_PAREN;
  std::unordered_set<std::string> param_names;
  while (true) {
    fail_if_EOF(Token::TYPE);
    bool is_ref = false;
    if (curr_token.type == Token::REF) {
      is_ref = true;
      advance(); // skip the ref
      fail_if_EOF(Token::TYPE);
    }
    if (curr_token.type != Token::TYPE) {
      std::string param_type = is_class ? "class" : "function";
      std::string msg = "Invalid " + param_type + " declaration, expected a type, but " + curr_token.get_name() + " found";
      throw_error(msg, curr_token.line);
    }
    std::string type = curr_token.value;
    advance(); // skip the type
    if (type == "void") {
      if (is_class) {
        std::string msg = "Invalid class declaration, cannot have void members";
        throw_error(msg, curr_token.line);
      }
      if (res.size() != 0) {
        std::string msg = "invalid function expression, illegal void placement";
        ErrorHandler::throw_syntax_error(msg);
      }
      if (curr_token.type != Token::RIGHT_PAREN) {
        std::string msg = "invalid function expression, expected ), but " + curr_token.get_name() + " found";
        ErrorHandler::throw_syntax_error(msg);
      }
      return res;
    }
    fail_if_EOF(Token::IDENTIFIER);
    if (curr_token.type != Token::IDENTIFIER) {
      std::string msg = "Invalid function declaration, expected an identifier, but " + curr_token.get_name() + " found";
      throw_error(msg, curr_token.line);
    }
    if (!param_names.insert(curr_token.value).second) {
      std::string n = is_class ? "class" : "function";
      std::string msg = "Invalid " + n + " declaration, duplicate parameter name '" + curr_token.value + "'";
      throw_error(msg, curr_token.line);
    }
    FuncParam param{type, curr_token.value};
    param.is_ref = is_ref;
    res.push_back(param);
    advance();
    fail_if_EOF(stop);
    if (curr_token.type == stop) break;
    advance(); // skip the sep
  }
  return res;
}

NodeListList Parser::get_many_expressions(TokenType sep, TokenType stop) {
  NodeListList res;
  while (true) {
    fail_if_EOF(stop);
    NodeList rpn = get_expression(sep, stop); // parse expression till either sep or stop
    res.push_back(rpn);
    if (curr_token.type == stop) break;
    advance();
    fail_if_EOF(stop);
  }
  return res;
}

Node Parser::parse_func_expr() {
  // function(arg1, arg2, ...) type { statement(s) };
  Node func = Node(Expression(ExprType::FUNC_EXPR));
  advance(); // skip the function
  if (curr_token.type == Token::OP_GT) {
    func.expr.func_expr.captures = true;
    advance(); // skip the >
  }
  if (curr_token.type != Token::LEFT_PAREN) {
    std::string msg = "invalid function declaration, expected '(', but " + curr_token.get_name() + " found";
    throw_error(msg, curr_token.line);
  }
  advance(); // skip the (
  func.expr.func_expr.params = parse_func_params();
  advance(); // skip the )
  bool returns_ref = curr_token.type == Token::REF;
  if (returns_ref) {
    advance(); // skip the ref
  }
  if (curr_token.type != Token::TYPE) {
    std::string msg = "invalid function declaration, expected a type, but " + curr_token.get_name() + " found";
    throw_error(msg, curr_token.line);
  }
  if (returns_ref && curr_token.value == "void") {
    std::string msg = "invalid function declaration, cannot return a reference to void";
    throw_error(msg, curr_token.line);
  }
  func.expr.func_expr.ret_type = curr_token.value;
  func.expr.func_expr.ret_ref = returns_ref;
  advance(); // skip the type
  if (curr_token.type != Token::LEFT_BRACE) {
    std::string msg = "invalid function declaration, expected '{', but " + curr_token.get_name() + " found";
    throw_error(msg, curr_token.line);
  }
  int func_end = find_block_end();
  TokenList func_start(tokens.begin() + pos, tokens.begin() + pos + func_end + 1); // create a subvector of tokens
  Parser func_parser(func_start, Token::NONE, "", utils);
  int end_pos = 0;
  func.expr.func_expr.instructions.push_back(func_parser.parse(&end_pos));
  pos += end_pos;
  advance();
  return func;
}

Node Parser::parse_class_stmt() {
  Node class_expr = Node(Statement(ClassStatement()));
  class_expr.stmt.line = curr_token.line;
  class_expr.stmt.source = curr_token.source;
  advance(); // skip the class
  if (curr_token.type != Token::IDENTIFIER) {
    std::string msg = "invalid class declaration, expected an identifier, but " + curr_token.get_name() + " found";
    throw_error(msg, curr_token.line);
  }
  class_expr.stmt.class_stmt.class_name = curr_token.value;
  advance(); // skip the identifier
  if (curr_token.type != Token::LEFT_PAREN) {
    std::string msg = "invalid class declaration, expected '(', but " + curr_token.get_name() + " found";
    throw_error(msg, curr_token.line);
  }
  advance(); // skip the (
  class_expr.stmt.class_stmt.members = parse_func_params(true);
  advance(); // skip the )
  if (curr_token.type != Token::SEMI_COLON) {
    std::string msg = "invalid class declaration, expected ';', but " + curr_token.get_name() + " found";
    throw_error(msg, curr_token.line);
  }
  advance(); // skip the ;
  return class_expr;
}

Node Parser::parse_array_expr() {
  advance(); // skip the array
  Node array((Expression(ExprType::ARRAY)));
  if (curr_token.type != Token::LEFT_PAREN) {
    std::string msg = "invalid array expression, expected '(', but " + curr_token.get_name() + " found";
    throw_error(msg, curr_token.line);
  }
  advance(); // skip the (
  array.expr.array_expressions = get_many_expressions(Token::COMMA, Token::RIGHT_PAREN);
  advance(); // skip the )
  if (curr_token.type == Token::LEFT_BRACKET) {
    advance(); // skip the [
    array.expr.array_size = get_expression(Token::RIGHT_BRACKET);
    advance(); // skip the ]
  }
  if (curr_token.type == Token::REF) {
    array.expr.array_holds_refs = true;
    advance(); // skip the ref
  }
  if (curr_token.type != Token::TYPE) {
    std::string msg = "invalid array expression, expected a type, but " + curr_token.get_name() + " found";
    throw_error(msg, curr_token.line);
  }
  if (curr_token.value == "void") {
    std::string msg = "invalid array expression, cannot hold void values";
    throw_error(msg, curr_token.line);
  }
  array.expr.array_type = curr_token.value;
  advance(); // skip the type
  return array;
}

Node Parser::get_expr_node() {
  fail_if_EOF(Token::GENERAL_EXPRESSION);
  if (curr_token.type == Token::FUNCTION) {
    return parse_func_expr();
  }
  if (curr_token.type == Token::ARRAY) {
    return parse_array_expr();
  }
  if (curr_token.type == Token::IDENTIFIER) {
    // it's an identifier expression
    Node id(Expression(curr_token.value, true));
    advance(); // skip the identifier
    return id;
  }
  if (curr_token.type == Token::LEFT_PAREN) {
    if (prev.type == Token::RIGHT_PAREN || prev.type == Token::IDENTIFIER
     || prev.type == Token::RIGHT_BRACKET || prev.type == Token::STRING_LITERAL) {
      // it's a function call
      // identifier(arg1, arg2...)
      FuncCall fc;
      Node call(fc);
      advance(); // skip the (
      call.expr.func_call.arguments = get_many_expressions(Token::COMMA, Token::RIGHT_PAREN); // (arg1, arg2...)
      advance(); // skip the )
      return call;
    }
  }
  if (curr_token.type == Token::LEFT_BRACKET) {
    // it's an indexing expresion 
    // expression[expression]
    advance(); // skip the [
    NodeList rpn = get_expression(Token::RIGHT_BRACKET);
    advance(); // skip the ]
    Expression e(rpn, true);
    Node index(e);
    return index;
  }
  if (curr_token.type == Token::STRING_LITERAL) {
    // string literal
    Node str_literal(Expression(curr_token.value));
    advance();
    return str_literal;
  }
  if (utils.op_unary(curr_token.type)) {
    std::string token_name = curr_token.get_name();
    TokenType token_type = curr_token.type;
    advance(); // skip the op
    fail_if_EOF(TokenType::GENERAL_EXPRESSION);
    Node oper(Expression(token_type, ExprType::UNARY_OP));
    return oper;
  }
  if (utils.op_binary(curr_token.type)) {
    std::string token_name = curr_token.get_name();
    TokenType token_type = curr_token.type;
    advance(); // skip the op
    fail_if_EOF(TokenType::GENERAL_EXPRESSION);
    Node oper(Expression(token_type, ExprType::BINARY_OP));
    return oper;
  }
  int base = (int)base_lut[(int)curr_token.type];
  if (base) {
    int is_neg = curr_token.value.c_str()[0] == '-';
    Node num_literal(Expression(strtoull(curr_token.value.c_str(), NULL, base), is_neg, true));
    advance(); // skip the number
    return num_literal;
  }
  if (curr_token.type == Token::FLOAT) {
    Node float_literal(Expression(strtod(curr_token.value.c_str(), NULL), true));
    advance(); // skip the float
    return float_literal;
  }
  if (curr_token.type == Token::TRUE || curr_token.type == Token::FALSE) {
    Node boolean(Expression(curr_token.type == Token::TRUE, 0.0f));
    advance(); // skip the boolean
    return boolean;
  }
  if (curr_token.type == Token::LEFT_PAREN) {
    advance(); // skip the (
    NodeList rpn = get_expression(Token::RIGHT_PAREN);
    advance(); // skip the )
    Expression e(rpn);
    Node rpn_expr(e);
    return rpn_expr;
  }
  std::string msg = "expected an expression, but " + curr_token.get_name() + " found";
  ErrorHandler::throw_syntax_error(msg, curr_token.line);
  return {};
}

static Node stack_peek(const NodeList &stack) {
  if (stack.size() == 0) {
    return {};
  }
  return stack.back();
}

NodeList Parser::get_expression(TokenType stop1, TokenType stop2) {
  NodeList queue;
  NodeList stack;
  while (curr_token.type != stop1 && curr_token.type != stop2) {
    Node tok = get_expr_node();
    if (tok.expr.is_evaluable()) {
      queue.push_back(tok);
    } else if (tok.expr.is_operation()) {
      Node top = stack_peek(stack);
      while (
        top.type != NodeType::UNKNOWN && top.expr.is_operation()
        &&
          (utils.get_precedence(top.expr) > utils.get_precedence(tok.expr)
          ||
            (utils.get_precedence(top.expr) == utils.get_precedence(tok.expr) 
              && !utils.right_assoc(tok)
            )
          )
        && 
          (top.expr.type != ExprType::LPAREN)
      ) {
        auto res = stack.back();
        stack.pop_back();
        queue.push_back(res);
        top = stack_peek(stack);
      }
      stack.push_back(tok);
    } else if (tok.expr.type == ExprType::LPAREN) {
      stack.push_back(tok);
    } else if (tok.expr.type == ExprType::RPAREN) {
      while (stack_peek(stack).expr.type != ExprType::LPAREN) {
        auto res = stack.back();
        stack.pop_back();
        queue.push_back(res);
      }
      if (stack_peek(stack).expr.type == ExprType::LPAREN) {
        stack.pop_back();
      } else {
        std::string msg = "no enclosing parenthesis";
        ErrorHandler::throw_syntax_error(msg, curr_token.line);
      }
    }
  }
  while (stack_peek(stack).type != NodeType::UNKNOWN) {
    auto res = stack.back();
    stack.pop_back();
    queue.push_back(res);
  }
  return queue;
}

NodeList Parser::get_many_statements(Node &node, TokenType stop) {
  NodeList res;
  while (true) {
    Node statement = get_statement(node, this->terminal);
    if (statement.type == NodeType::UNKNOWN) {
      break;
    }
    res.push_back(statement);
  }
  return res;
}

Node Parser::get_statement(Node &prev, TokenType stop) {
  if (curr_token.type == stop) {
    return prev;
  }
  if (curr_token.type == Token::CLASS) {
    return parse_class_stmt();
  }
  if (curr_token.type == Token::SET) {
    Node set((Statement(StmtType::SET)));
    set.stmt.line = curr_token.line;
    set.stmt.source = curr_token.source;
    advance(); // skip the $
    if (curr_token.type != Token::IDENTIFIER) {
      std::string msg = "invalid set statement. Expected an identifier, but " + curr_token.get_name() + " found";
      throw_error(msg, curr_token.line);
    }
    set.stmt.obj_members.push_back(curr_token.value);
    advance(); // skip the id
    while (true) {
      if (curr_token.type != Token::DOT) {
        std::string msg = "invalid set statement. Expected '.', but " + curr_token.get_name() + " found";
        throw_error(msg, curr_token.line);
      }
      advance(); // skip the dot
      if (curr_token.type != Token::IDENTIFIER) {
        std::string msg = "invalid set statement. Expected an identifier, but " + curr_token.get_name() + " found";
        throw_error(msg, curr_token.line);
      }
      set.stmt.obj_members.push_back(curr_token.value);
      advance(); // skip the id
      if (curr_token.type == Token::OP_ASSIGN) {
        break;
      }
    }
    advance(); // skip the =
    NodeList rpn = get_expression(Token::SEMI_COLON);
    set.stmt.expressions.push_back(rpn);
    advance(); // skip the ;
    return set;
  }
  if (curr_token.type == Token::SET_IDX) {
    Node set_idx((Statement(StmtType::SET_IDX)));
    set_idx.stmt.line = curr_token.line;
    set_idx.stmt.source = curr_token.source;
    advance(); // skip the #
    if (curr_token.type != Token::IDENTIFIER) {
      std::string msg = "invalid set index statement. Expected an identifier, but " + curr_token.get_name() + " found";
      throw_error(msg, curr_token.line);
    }
    set_idx.stmt.obj_members.push_back(curr_token.value);
    advance(); // skip the id
    while (true) {
      Node idx_expr = get_expr_node();
      if (idx_expr.expr.type != ExprType::INDEX) {
        std::string msg = "invalid set index statement, expected an index expression";
        throw_error(msg, curr_token.line);
      }
      set_idx.stmt.indexes.push_back(idx_expr);
      if (curr_token.type == Token::OP_ASSIGN) {
        break;
      }
    }
    advance(); // skip the =
    NodeList rpn = get_expression(Token::SEMI_COLON);
    set_idx.stmt.expressions.push_back(rpn);
    advance(); // skip the ;
    return set_idx;
  }
  if (curr_token.type == Token::LEFT_BRACE) {
    // { statement(s) }
    Node comp((Statement(StmtType::COMPOUND)));
    comp.stmt.line = curr_token.line;
    comp.stmt.source = curr_token.source;
    advance(); // skip the {
    int block_end = find_enclosing_brace(pos, 1);
    TokenList block_start(tokens.begin() + pos, tokens.begin() + pos + block_end + 1); // create a subvector of tokens
    Parser block_parser(block_start, Token::RIGHT_BRACE, "", utils);
    int end_pos = 0;
    comp.stmt.statements.push_back(block_parser.parse(&end_pos));
    pos += end_pos;
    advance(); // skip the }
    return comp;
  } else if (curr_token.type == Token::IF) {
    // if (expression) statement
    std::uint64_t line = curr_token.line;
    std::string *source = curr_token.source;
    advance(); // skip the if keyword
    if (curr_token.type != Token::LEFT_PAREN) {
      // invalid if statement
      std::string msg = "invalid if statement. Expected '(', but " + curr_token.get_name() + " found";
      throw_error(msg, curr_token.line);
    }
    Node if_stmt = Node((Statement(StmtType::IF)));
    if_stmt.stmt.line = line;
    if_stmt.stmt.source = source;
    advance(); // skip the (
    NodeList rpn = get_expression(Token::RIGHT_PAREN);
    if_stmt.stmt.expressions.push_back(rpn);
    advance(); // skip the )
    if_stmt.stmt.statements.push_back(get_statement(prev, stop));
    if (curr_token.type == Token::ELSE) {
      advance(); // skip the else
      if_stmt.stmt.statements.push_back(get_statement(prev, stop));
    }
    return if_stmt;
  } else if (curr_token.type == Token::WHILE) {
    // while (expression) statement
    std::uint64_t line = curr_token.line;
    std::string *source = curr_token.source;
    advance(); // skip the while keyword
    if (curr_token.type != Token::LEFT_PAREN) {
      // invalid while statement
      std::string msg = "invalid while statement. Expected '(', but " + curr_token.get_name() + " found";
      throw_error(msg, curr_token.line);
    }
    Node while_stmt = Node((Statement(StmtType::WHILE)));
    while_stmt.stmt.line = line;
    while_stmt.stmt.source = source;
    advance(); // skip the (
    NodeList rpn = get_expression(Token::RIGHT_PAREN);
    while_stmt.stmt.expressions.push_back(rpn);
    advance(); // skip the )
    while_stmt.stmt.statements.push_back(get_statement(prev, stop));
    return while_stmt;
  } else if (curr_token.type == Token::FOR) {
    // for (expression; expression; expression) statement
    std::uint64_t line = curr_token.line;
    std::string *source = curr_token.source;
    advance(); // skip the for keyword
    if (curr_token.type != Token::LEFT_PAREN) {
      // invalid for statement
      std::string msg = "invalid for statement. Expected '(', but " + curr_token.get_name() + " found";
      throw_error(msg, curr_token.line);
    }
    Node for_stmt = Node((Statement(StmtType::FOR)));
    for_stmt.stmt.line = line;
    for_stmt.stmt.source = source;
    advance(); // skip the (
    for_stmt.stmt.expressions = get_many_expressions(Token::SEMI_COLON, Token::RIGHT_PAREN);
    advance(); // skip the )
    for_stmt.stmt.statements.push_back(get_statement(prev, stop));
    return for_stmt;
  } else if (curr_token.type == Token::RETURN) {
    // return expression;
    Node return_stmt((Statement(StmtType::RETURN)));
    return_stmt.stmt.line = curr_token.line;
    return_stmt.stmt.source = curr_token.source;
    advance(); // skip the return
    NodeList rpn = get_expression(Token::SEMI_COLON);
    return_stmt.stmt.expressions.push_back(rpn);
    advance(); // skip the semicolon
    return return_stmt;
  } else if (curr_token.type == Token::BREAK) {
    std::uint64_t line = curr_token.line;
    std::string *source = curr_token.source;
    advance(); // skip the break
    if (curr_token.type != Token::SEMI_COLON) {
      std::string msg = "expected ';', but " + curr_token.get_name() + " found";
      ErrorHandler::throw_syntax_error(msg);
    }
    advance(); // skip the ;
    Node break_stmt = Statement((StmtType::BREAK));
    break_stmt.stmt.line = line;
    break_stmt.stmt.source = source;
    return break_stmt;
  } else if (curr_token.type == Token::CONTINUE) {
    std::uint64_t line = curr_token.line;
    std::string *source = curr_token.source;
    advance(); // skip the continue
    if (curr_token.type != Token::SEMI_COLON) {
      std::string msg = "expected ';', but " + curr_token.get_name() + " found";
      ErrorHandler::throw_syntax_error(msg);
    }
    advance(); // skip the ;
    Node continue_stmt = Statement((StmtType::CONTINUE));
    continue_stmt.stmt.line = line;
    continue_stmt.stmt.source = source;
    return continue_stmt;
  } else if (curr_token.type == Token::TYPE) {
    // type identifier = expression;
    if (curr_token.value == "void") {
      std::string msg = "invalid variable declaration. Cannot declare a void variable";
      throw_error(msg, curr_token.line);
    }
    std::uint64_t curr_line = curr_token.line;
    std::string *source = curr_token.source;
    bool allocated = this->prev.type == Token::ALLOC;
    bool constant = this->prev.type == Token::CONST;
    bool reference = this->prev.type == Token::REF;
    if (allocated && lookahead(-2).type == Token::CONST) {
      constant = true;
    }
    Node var_decl = Node((Declaration(DeclType::VAR_DECL)));
    var_decl.decl.var_type = curr_token.value;
    advance(); // skip the variable type
    if (curr_token.type != Token::IDENTIFIER) {
      std::string msg = "invalid variable declaration. Expected an identifier, but " + curr_token.get_name() + " found";
      throw_error(msg, curr_token.line);
    }
    var_decl.decl.id = curr_token.value; // set the declaration identifier to token value
    advance(); // skip the identifier
    if (curr_token.type != Token::OP_ASSIGN) {
      std::string msg = "invalid variable declaration. Expected '=', but " + curr_token.get_name() + " found";
      throw_error(msg, curr_token.line);
    }
    advance(); // skip the =
    var_decl.decl.var_expr = get_expression(Token::SEMI_COLON);
    var_decl.decl.allocated = allocated;
    var_decl.decl.constant = constant;
    var_decl.decl.reference = reference;
    Node decl_stmt((Statement(StmtType::DECL)));
    decl_stmt.stmt.declaration.push_back(var_decl);
    decl_stmt.stmt.line = curr_line;
    decl_stmt.stmt.source = source;
    advance(); // skip the semicolon
    return decl_stmt;
  } else if (curr_token.type == Token::ALLOC) {
    // alloc declaration
    advance(); // skip the alloc
    if (curr_token.type != Token::TYPE) {
      std::string msg = "invalid variable allocation. Expected a type, but " + curr_token.get_name() + " found";
      throw_error(msg, curr_token.line);
    }
    return get_statement(prev, stop);
  } else if (curr_token.type == Token::REF) {
    // ref declaration
    advance();
    if (curr_token.type != Token::TYPE) {
      std::string msg = "invalid variable reference. Expected a type, but " + curr_token.get_name() + " found";
      throw_error(msg, curr_token.line);
    }
    return get_statement(prev, stop);
  } else if (curr_token.type == Token::CONST) {
    // const declaration
    advance(); // skip the const
    if (curr_token.type != Token::TYPE && curr_token.type != Token::ALLOC && curr_token.type != Token::REF) {
      std::string msg = "invalid constant variable declaration. Expected a type, alloc or ref, but " + curr_token.get_name() + " found";
      throw_error(msg, curr_token.line);
    }
    return get_statement(prev, stop);
  } else if (curr_token.type == Token::SEMI_COLON && prev.type == NodeType::UNKNOWN) {
    // nop;
    Node nop_stmt((Statement(StmtType::NOP)));
    nop_stmt.stmt.line = curr_token.line;
    nop_stmt.stmt.source = curr_token.source;
    advance(); // skip the semicolon
    return nop_stmt;
  } else if (curr_token.type == this->terminal) {
    // end parsing
    return prev;
  } else {
    // expression;
    std::uint64_t line = curr_token.line;
    std::string *source = curr_token.source;
    NodeList expr = get_expression(Token::SEMI_COLON);
    Node expr_stmt((Statement(StmtType::EXPR)));
    expr_stmt.stmt.expressions.push_back(expr);
    expr_stmt.stmt.line = line;
    expr_stmt.stmt.source = source;
    advance(); // skip the semicolon
    return expr_stmt;
  }
  throw_error("unrecognized token " + curr_token.get_name(), curr_token.line);
  return prev;
}

Node Parser::parse(int *end_pos) {
  Node block;
  NodeList instructions = get_many_statements(block, this->terminal);
  block.add_children(instructions);
  if (end_pos != NULL) {
    *end_pos = pos;
  }
  return block;
}