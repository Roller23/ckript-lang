#include "parser.hpp"
#include "AST.hpp"
#include "token.hpp"
#include "error-handler.hpp"

#include <vector>
#include <iostream>

typedef Expression::ExprType ExprType;
typedef Declaration::DeclType DeclType;
typedef Statement::StmtType StmtType;
typedef Node::NodeType NodeType;
typedef FuncExpression::FuncType FuncType;
typedef Token::TokenType TokenType;

bool Parser::op_binary(TokenType token) {
  return op_precedence[token] && !op_unary(token);
}

bool Parser::op_unary(TokenType token) {
  return token == TokenType::OP_NOT || token == TokenType::OP_NEG || token == TokenType::DEL;
}

bool Parser::right_assoc(const Node &n) {
  auto precedence = get_precedence(n.expr.op);
  return precedence == 12 || precedence == 1; // REMEMBER TO CHANGE IF YOU EVER CHANGE PRECEDENCE
}

char Parser::get_precedence(Token::TokenType token) {
  if (op_unary(token)) {
    return 12;
  }
  return op_precedence[token];
}

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
  prev = tokens.at(pos - 1);
  if (pos < tokens_count) {
    curr_token = tokens.at(pos);
  } else {
    std::cout << "Reached EOF\n";
    curr_token = Token(Token::NONE);
    pos--;
  }
}

void Parser::retreat(void) {
  if (pos == 0) return;
  pos--;
  curr_token = tokens.at(pos);
  prev = pos > 0 ? tokens.at(pos - 1) : Token(Token::NONE);
}

Token Parser::lookahead(int offset) {
  if (pos + offset < 0) return {};
  if (pos + offset >= tokens.size()) {
    return {}; // empty token
  }
  return tokens.at(pos + offset);
}

int Parser::find_enclosing_brace(int start_pos, int braces) {
  int i = 0;
  int size = tokens.size();
  while (true) {
    if (size == i) {
      std::string msg = "Invalid function declaration, no enclosing brace found";
      throw_error(msg, tokens.at(start_pos + i - 1).line);
    }
    if (tokens.at(start_pos + i).type == Token::LEFT_BRACE) {
      braces++;
    }
    if (tokens.at(start_pos + i).type == Token::RIGHT_BRACE) {
      braces--;
      if (braces == 0) {
        return i;
      }
    }
    i++;
  }
}

int Parser::find_enclosing_semi(int start_pos) {
  int i = 0;
  int size = tokens.size();
  while (true) {
    if (size == i) {
      std::string msg = "Invalid function declaration, no semicolon found";
      throw_error(msg, tokens.at(start_pos + i - 1).line);
    }
    if (tokens.at(start_pos + i).type == Token::SEMI_COLON) {
      return i;
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
      throw_error(msg, tokens.at(start_pos + i - 1).line);
    }
    if (tokens.at(start_pos + i).type == Token::LEFT_PAREN) {
      lparen++;
    }
    if (tokens.at(start_pos + i).type == Token::RIGHT_PAREN) {
      lparen--;
      if (lparen == 0) {
        return i;
      }
    }
    i++;
  }
}

int Parser::find_block_end(void) {
  if (curr_token.type == Token::LEFT_BRACE) {
    return find_enclosing_brace(pos);
  }
  return find_enclosing_semi(pos);
}

ParamList Parser::parse_func_params() {
  ParamList res;
  TokenType stop = Token::RIGHT_PAREN;
  while (true) {
    fail_if_EOF(Token::TYPE);
    if (curr_token.type != Token::TYPE) {
      std::string msg = "Invalid function declaration, expected a type, but " + curr_token.get_name() + " found";
      throw_error(msg, curr_token.line);
    }
    std::string type = curr_token.value;
    advance();
    fail_if_EOF(Token::IDENTIFIER);
    if (curr_token.type != Token::IDENTIFIER) {
      std::string msg = "Invalid function declaration, expected an identifier, but " + curr_token.get_name() + " found";
      throw_error(msg, curr_token.line);
    }
    res.push_back({type, curr_token.value});
    advance();
    fail_if_EOF(stop);
    if (curr_token.type == stop) {
      break;
    }
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
  // function(arg1, arg2, ...) type statement(s);
  Node func = Node(Expression(ExprType::FUNC_EXPR));
  func.expr.func_expr.type = curr_token.type == Token::THREAD ? FuncType::THREAD : FuncType::FUNC;
  advance(); // skip the function/thread
  if (curr_token.type != Token::LEFT_PAREN) {
    std::string msg = "invalid function declaration, expected '(', but " + curr_token.get_name() + " found";
    throw_error(msg, curr_token.line);
  }
  advance(); // skip the (
  func.expr.func_expr.params = parse_func_params();
  advance(); // skip the )
  if (curr_token.type != Token::TYPE) {
    std::string msg = "invalid function declaration, expected a type, but " + curr_token.get_name() + " found";
    throw_error(msg, curr_token.line);
  }
  func.expr.func_expr.ret_type = curr_token.value;
  advance(); // skip the type
  bool starts_with_brace = curr_token.type == Token::LEFT_BRACE;
  int func_end = find_block_end();
  TokenList func_start(tokens.begin() + pos, tokens.begin() + pos + func_end + 1); // create a subvector of tokens
  Parser func_parser(func_start, Token::NONE, "FUNC");
  int end_pos = 0;
  func.expr.func_expr.instructions.push_back(func_parser.parse(&end_pos));
  pos += end_pos;
  advance(); // skip the semicolon
  return func;
}

Node Parser::get_expr_node() {
  std::cout << "(" + parser_name + ") - ";
  fail_if_EOF(Token::GENERAL_EXPRESSION);
  std::cout << "consuming expression\n";
  if (curr_token.type == Token::FUNCTION || curr_token.type == Token::THREAD) {
    std::cout << "found function expression\n";
    return parse_func_expr();
  }
  if (curr_token.type == Token::IDENTIFIER) {
    std::cout << "found an identifier expression\n";
    // it's an identifier expression
    Node id(Expression(curr_token.value, true));
    advance(); // skip the identifier
    return id;
  }
  if (curr_token.type == Token::LEFT_PAREN) {
    if (prev.type == Token::RIGHT_PAREN || prev.type == Token::IDENTIFIER) {
      std::cout << "found a fn call\n";
      // it's a function call
      // identifier(arg1, arg2...)
      FuncCall fc;
      Node call(fc);
      advance(); // skip the (
      std::cout << "parsing arguments\n";
      call.expr.func_call.arguments = get_many_expressions(Token::COMMA, Token::RIGHT_PAREN); // (arg1, arg2...)
      advance(); // skip the )
      return call;
    }
  }
  if (curr_token.type == Token::LEFT_BRACKET) {
    // it's an indexing expresion 
    // expression[expression]
    std::cout << "found an index\n";
    advance(); // skip the [
    NodeList rpn = get_expression(Token::RIGHT_BRACKET);
    advance(); // skip the ]
    Expression e(rpn, true);
    Node index(e);
    return index;
  }
  if (curr_token.type == Token::STRING_LITERAL) {
    // string literal
    std::cout << "found a string literal\n";
    Node str_literal(Expression(curr_token.value));
    advance();
    return str_literal;
  }
  if (curr_token.type == Token::UNDEF) {
    // undef literal
    std::cout << "found undef\n";
    Node undef(Expression(curr_token.value));
    advance();
    return undef;
  }
  if (op_unary(curr_token.type)) {
    std::cout << "Found an unary operator " << curr_token << "\n";
    std::string token_name = curr_token.get_name();
    TokenType token_type = curr_token.type;
    advance(); // skip the op
    fail_if_EOF(TokenType::GENERAL_EXPRESSION);
    Node oper(Expression(token_type, ExprType::UNARY_OP));
    return oper;
  }
  if (op_binary(curr_token.type)) {
    std::cout << "Found a binary operator " << curr_token << "\n";
    std::string token_name = curr_token.get_name();
    TokenType token_type = curr_token.type;
    advance(); // skip the op
    fail_if_EOF(TokenType::GENERAL_EXPRESSION);
    Node oper(Expression(token_type, ExprType::BINARY_OP));
    return oper;
  }
  int base = (int)base_lut[(int)curr_token.type];
  if (base) {
    std::cout << "found a number literal " + curr_token.value + "\n";
    int is_neg = curr_token.value.c_str()[0] == '-';
    Node num_literal(Expression(strtoull(curr_token.value.c_str(), NULL, base), is_neg, true));
    advance(); // skip the number
    return num_literal;
  }
  if (curr_token.type == Token::FLOAT) {
    std::cout << "found a float literal\n";
    Node float_literal(Expression(strtod(curr_token.value.c_str(), NULL), true));
    advance(); // skip the float
    return float_literal;
  }
  if (curr_token.type == Token::TRUE || curr_token.type == Token::FALSE) {
    std::cout << "found a boolean literal\n";
    Node boolean(Expression(curr_token.type == Token::TRUE, 0.0f));
    advance(); // skip the boolean
    return boolean;
  }
  if (curr_token.type == Token::LEFT_PAREN) {
    std::cout << "found left paren, recursion!\n";
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
          (get_precedence(top.expr.op) > get_precedence(tok.expr.op)
          ||
            (get_precedence(top.expr.op) == get_precedence(tok.expr.op) && !right_assoc(tok))
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
  std::cout << "\nQUEUE START\n";
  for (auto &q : queue) {
    std::cout << " ";
    q.print();
    std::cout << " ";
  }
  std::cout << "\nQUEUE END\n";
  return queue;
}

NodeList Parser::get_many_statements(Node &node, TokenType stop) {
  std::cout << "consuming multiple statements\n";
  NodeList res;
  while (true) {
    Node statement = get_statement(node, this->terminal);
    if (statement.type == NodeType::UNKNOWN) {
      break;
    }
    res.push_back(statement);
    std::cout << "\n------- PUSHED STATEMENT --------\n";
    std::cout << "type " << statement.type << ", ";
    statement.print();
    std::cout << "\n-------       END        --------\n";
  }
  return res;
}

Node Parser::get_statement(Node &prev, TokenType stop) {
  std::cout << "(" + parser_name + ") - ";
  if (curr_token.type == stop) {
    std::cout << "Encountered stop - " << Token::get_name(stop) << "\n";
    return prev;
  }
  if (curr_token.type == Token::LEFT_BRACE) {
    std::cout << "Found {\n";
    // { statement(s) }
    advance(); // skip the {
    Node comp(Statement(StmtType::COMPOUND));
    int block_end = find_enclosing_brace(pos, 1);
    TokenList block_start(tokens.begin() + pos, tokens.begin() + pos + block_end + 1); // create a subvector of tokens
    Parser block_parser(block_start, Token::RIGHT_BRACE, "BLOCK");
    int end_pos = 0;
    comp.stmt.statements.push_back(block_parser.parse(&end_pos));
    pos += end_pos;
    advance(); // skip the }
    return comp;
  } else if (curr_token.type == Token::IF) {
    std::cout << "Found if\n";
    // if (expression) statement
    advance(); // skip the if keyword
    if (curr_token.type != Token::LEFT_PAREN) {
      // invalid if statement
      std::string msg = "invalid if statement. Expected '(', but " + curr_token.get_name() + " found";
      throw_error(msg, curr_token.line);
    }
    Node if_stmt = Node(Statement(StmtType::IF));
    advance(); // skip the (
    NodeList rpn = get_expression(Token::RIGHT_PAREN);
    if_stmt.stmt.expressions.push_back(rpn);
    advance(); // skip the )
    if_stmt.stmt.statements.push_back(get_statement(prev, stop)); // get the statement block
    return if_stmt;
  } else if (curr_token.type == Token::WHILE) {
    std::cout << "Found while\n";
    // while (expression) statement
    advance(); // skip the while keyword
    if (curr_token.type != Token::LEFT_PAREN) {
      // invalid while statement
      std::string msg = "invalid while statement. Expected '(', but " + curr_token.get_name() + " found";
      throw_error(msg, curr_token.line);
    }
    Node while_stmt = Node(Statement(StmtType::WHILE));
    advance(); // skip the (
    NodeList rpn = get_expression(Token::RIGHT_PAREN);
    while_stmt.stmt.expressions.push_back(rpn);
    advance(); // skip the )
    while_stmt.stmt.statements.push_back(get_statement(prev, stop));
    return while_stmt;
  } else if (curr_token.type == Token::FOR) {
    std::cout << "Found for\n";
    // for (expression; expression; expression) statement
    advance(); // skip the for keyword
    if (curr_token.type != Token::LEFT_PAREN) {
      // invalid for statement
      std::string msg = "invalid for statement. Expected '(', but " + curr_token.get_name() + " found";
      throw_error(msg, curr_token.line);
    }
    Node for_stmt = Node(Statement(StmtType::FOR));
    advance(); // skip the (
    for_stmt.stmt.expressions = get_many_expressions(Token::SEMI_COLON, Token::RIGHT_PAREN);
    advance(); // skip the )
    for_stmt.stmt.statements.push_back(get_statement(prev, stop));
    return for_stmt;
  } else if (curr_token.type == Token::RETURN) {
    std::cout << "Found return\n";
    // return expression;
    advance(); // skip the return
    Node return_stmt(Statement(StmtType::RETURN));
    NodeList rpn = get_expression(Token::SEMI_COLON);
    return_stmt.stmt.expressions.push_back(rpn);
    advance(); // skip the semicolon
    return return_stmt;
  } else if (curr_token.type == Token::BREAK) {
    advance(); // skip the break
    if (curr_token.type != Token::SEMI_COLON) {
      std::string msg = "expected ';', but " + curr_token.get_name() + " found";
      ErrorHandler::throw_syntax_error(msg);
    }
    advance(); // skip the ;
    return Statement(StmtType::BREAK);
  } else if (curr_token.type == Token::TYPE) {
    std::cout << "Found type\n";
    // type identifier = expression;
    bool allocated = this->prev.type == Token::ALLOC;
    bool constant = this->prev.type == Token::CONST;
    if (allocated && lookahead(-2).type == Token::CONST) {
      constant = true;
    }
    Node var_decl = Node(Declaration(DeclType::VAR_DECL));
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
    Node decl_stmt(Statement(StmtType::DECL));
    decl_stmt.stmt.declaration.push_back(var_decl);
    advance(); // skip the semicolon
    return decl_stmt;
  } else if (curr_token.type == Token::ALLOC) {
    // alloc declaration
    std::cout << "Found alloc\n";
    advance(); // skip the alloc
    if (curr_token.type != Token::TYPE) {
      std::string msg = "invalid variable allocation. Expected a type, but " + curr_token.get_name() + " found";
      throw_error(msg, curr_token.line);
    }
    return get_statement(prev, stop);
  } else if (curr_token.type == Token::CONST) {
    // const declaration
    std::cout << "found const\n";
    advance(); // skip the const
    if (curr_token.type != Token::TYPE && curr_token.type != Token::ALLOC) {
      std::string msg = "invalid constant variable declaration. Expected a type or alloc, but " + curr_token.get_name() + " found";
      throw_error(msg, curr_token.line);
    }
    return get_statement(prev, stop);
  } else if (curr_token.type == Token::SEMI_COLON && prev.type == NodeType::UNKNOWN) {
    std::cout << "no operation\n";
    // nop;
    Node nop_stmt(Statement(StmtType::NOP));
    advance(); // skip the semicolon
    return nop_stmt;
  } else if (curr_token.type == this->terminal) {
    std::cout << "Encountered terminal - " << Token::get_name(this->terminal) << "\n";
    // end parsing
    return prev;
  } else {
    std::cout << "found an expression\n";
    // expression;
    NodeList expr = get_expression(Token::SEMI_COLON);
    Node expr_stmt(Statement(StmtType::EXPR));
    expr_stmt.stmt.expressions.push_back(expr);
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