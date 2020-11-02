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

bool Parser::op_binary() {
  return op_precedence[curr_token.type] && !op_unary();
}

bool Parser::op_unary() {
  return curr_token.type == TokenType::OP_NOT || curr_token.type == TokenType::OP_NEG;
}

char Parser::get_precedence(Token::TokenType token) {
  if (op_unary()) {
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

NodeList Parser::get_many_expressions(Node &prev, TokenType sep, TokenType stop) {
  NodeList res;
  while (true) {
    fail_if_EOF(stop);
    Node expr = get_expression(prev, sep, stop); // parse expression till either sep or stop
    res.push_back(expr);
    if (curr_token.type == stop) {
      break;
    }
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
  if (!starts_with_brace) {
    // retreat();
  }
  return func;
}

Node Parser::get_expression(Node &prev, TokenType stop1, TokenType stop2) {
  std::cout << "(" + parser_name + ") - ";
  fail_if_EOF(Token::GENERAL_EXPRESSION);
  if (curr_token.type == stop1 || curr_token.type == stop2) {
    std::cout << "Expression stopped\n";
    return prev;
  }
  std::cout << "consuming expression\n";
  if (curr_token.type == Token::FUNCTION || curr_token.type == Token::THREAD) {
    std::cout << "found function expression\n";
    Node func = parse_func_expr();
    if (curr_token.type == stop1 || curr_token.type == stop2) {
      std::cout << "Expression stopped (fn)\n";
      return func;
    }
    return get_expression(func, stop1, stop2);
  }
  if (curr_token.type == Token::IDENTIFIER) {
    if (lookahead().type == Token::LEFT_PAREN) {
      std::cout << "found a fn call\n";
      // it's a function call
      // identifier(arg1, arg2...)
      Node call(FuncCall(curr_token.value));
      advance(); // skip the identifier
      advance(); // skip the (
      std::cout << "parsing arguments\n";
      Node args_node;
      NodeList args = get_many_expressions(args_node, Token::COMMA, Token::RIGHT_PAREN); // (arg1, arg2...)
      for (auto &arg : args) {
        call.expr.func_call.arguments.push_back(arg.expr);
      }
      advance(); // skip the )
      return get_expression(call, stop1, stop2);
    } else {
      std::cout << "found an identifier expression\n";
      // it's an identifier expression
      Node id(Expression(curr_token.value, true));
      advance(); // skip the identifier
      return get_expression(id, stop1, stop2);
    }
  }
  if (curr_token.type == Token::STRING_LITERAL) {
    // string literal
    std::cout << "found a string literal\n";
    Node str_literal(Expression(curr_token.value));
    advance();
    return get_expression(str_literal, stop1, stop2);
  }
  if (curr_token.type == Token::UNDEF) {
    // undef literal
    std::cout << "found undef\n";
    Node undef(Expression(curr_token.value));
    advance();
    return get_expression(undef, stop1, stop2);
  }
  if (op_binary()) {
    std::string token_name = curr_token.get_name();
    TokenType token_type = curr_token.type;
    if (prev.type != NodeType::EXPR) {
      std::string msg = "left side of " + token_name + " must be an expression, " + this->prev.get_name() + " found";
      throw_error(msg, this->prev.line);
    }
    advance(); // skip the op
    Node expr_start(Expression(ExprType::NONE));
    Node next = get_expression(expr_start, stop1, stop2);
    if (next.type != NodeType::EXPR) {
      std::string msg = "right side of " + token_name + " must be an expression, " + curr_token.get_name() + " found";
      throw_error(msg, curr_token.line);
    }
    Node op(Expression(prev, token_type, next));
    return get_expression(op, stop1, stop2);
  }
  int base = (int)base_lut[(int)curr_token.type];
  if (base) {
    std::cout << "found a number literal " + curr_token.value + "\n";
    int is_neg = curr_token.value.c_str()[0] == '-';
    Node num_literal(Expression(strtoull(curr_token.value.c_str(), NULL, base), is_neg, true));
    advance();
    return get_expression(num_literal, stop1, stop2);
  }
  if (curr_token.type == Token::FLOAT) {
    std::cout << "found a float literal\n";
    Node float_literal(Expression(strtod(curr_token.value.c_str(), NULL), true));
    advance();
    return get_expression(float_literal, stop1, stop2);
  }
  if (curr_token.type == Token::TRUE || curr_token.type == Token::FALSE) {
    std::cout << "found a boolean literal\n";
    Node boolean(Expression(curr_token.type == Token::TRUE, 0.0f));
    advance();
    return get_expression(boolean, stop1, stop2);
  }
  std::cout << "unknown expression token " << curr_token << "\n";
  return prev;
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
    std::cout << "------- PUSHED STATEMENT --------\n";
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
    Node expr_start(Expression(ExprType::NONE));
    Node expr = get_expression(expr_start, Token::RIGHT_PAREN);
    if_stmt.stmt.expressions.push_back(expr);
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
    Node expr_start(Expression(ExprType::NONE));
    Node expr = get_expression(expr_start, Token::RIGHT_PAREN);
    while_stmt.stmt.expressions.push_back(expr);
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
    Node expr_start(Expression(ExprType::NONE));
    NodeList expressions = get_many_expressions(expr_start, Token::SEMI_COLON, Token::RIGHT_PAREN);
    for (auto &expr : expressions) {
      for_stmt.stmt.expressions.push_back(expr);
    }
    advance(); // skip the )
    prev.add_children(for_stmt);
    return get_statement(for_stmt, stop);
  } else if (curr_token.type == Token::RETURN) {
    std::cout << "Found return\n";
    // return expression;
    advance(); // skip the return
    Node return_stmt(Statement(StmtType::RETURN));
    Node expr_start(Expression(ExprType::NONE));
    Node return_expr = get_expression(expr_start, Token::SEMI_COLON);
    return_stmt.stmt.expressions.push_back(return_expr);
    advance(); // skip the semicolon
    return return_stmt;
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
    Node expr_start(Expression(ExprType::NONE));
    var_decl.decl.var_expr.push_back(get_expression(expr_start, Token::SEMI_COLON));
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
    Node expr_start(Expression(ExprType::NONE));
    Node expr = get_expression(expr_start, Token::SEMI_COLON);
    Node expr_stmt(Statement(StmtType::EXPR));
    expr_stmt.stmt.expressions.push_back(expr);
    advance(); // skip the semicolon
    return expr_stmt;
  }
  throw_error("unrecognized token " + curr_token.get_name(), curr_token.line);
  return prev;
}

Node Parser::parse(int *end_pos) {
  Node Main;
  NodeList instructions = get_many_statements(Main, this->terminal);
  Main.add_children(instructions);
  if (end_pos != NULL) {
    *end_pos = pos;
  }
  return Main;
}