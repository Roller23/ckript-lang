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

bool Parser::op_binary(Token::TokenType token) {
  return binary_tokens_lut[(unsigned int)token];
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
  if (pos + offset >= tokens.size()) {
    return {}; // empty token
  }
  return tokens.at(pos + offset);
}

int Parser::find_func_end_brace(TokenList &tokens, int start_pos) {
  int i = 0;
  int brackets = 0;
  int size = tokens.size();
  while (true) {
    if (size == i) {
      std::string msg = "Invalid function declaration, no enclosing brace found";
      throw_error(msg, tokens.at(start_pos + i - 1).line);
    }
    if (tokens.at(start_pos + i).type == Token::LEFT_BRACE) {
      brackets++;
    }
    if (tokens.at(start_pos + i).type == Token::RIGHT_BRACE) {
      brackets--;
      if (brackets == 0) {
        return i;
      }
    }
    i++;
  }
}

int Parser::find_func_end_semi(TokenList &tokens, int start_pos) {
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

ParamList Parser::parse_func_params() {
  ParamList res;
  TokenType sep = Token::COMMA;
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
    std::string msg = "invalid function declaration, expected '(', but " + curr_token.get_name() + "found";
    throw_error(msg, curr_token.line);
  }
  advance(); // skip the (
  func.expr.func_expr.params = parse_func_params();
  advance(); // skip the )
  if (curr_token.type != Token::TYPE) {
    std::string msg = "invalid function declaration, expected a type, but " + curr_token.get_name() + "found";
    throw_error(msg, curr_token.line);
  }
  func.expr.func_expr.ret_type = curr_token.value;
  advance(); // skip the type
  int func_end = 0;
  bool starts_with_brace = curr_token.type == Token::LEFT_BRACE;
  if (starts_with_brace) {
    func_end = find_func_end_brace(tokens, pos);
  } else {
    func_end = find_func_end_semi(tokens, pos);
  }
  TokenList func_start(tokens.begin() + pos, tokens.begin() + pos + func_end + 1); // create a subvector of tokens
  Parser func_parser(func_start, Token::NONE, "FUNC");
  int end_pos = 0;
  func.expr.func_expr.instructions.push_back(func_parser.parse(&end_pos));
  pos += end_pos;
  advance(); // skip the semicolon
  if (!starts_with_brace) {
    retreat();
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
    Node func = parse_func_expr();
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
  if (op_binary(curr_token.type)) {
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
    Node num_literal(Expression(strtoull(curr_token.value.c_str(), NULL, base)));
    advance();
    return get_expression(num_literal, stop1, stop2);
  }
  if (curr_token.type == Token::FLOAT) {
    std::cout << "found a float literal\n";
    Node float_literal(Expression(strtod(curr_token.value.c_str(), NULL), true));
    advance();
    return get_expression(float_literal, stop1, stop2);;
  }
  std::cout << "unknown expression token " << curr_token << "\n";
  return prev;
}

void Parser::get_many_statements(Node &node, TokenType stop) {
  std::cout << "consuming statements in a compound statement\n";
  Node stmt;
  while (true) {
    fail_if_EOF(stop);
    stmt = get_statement(node, stop);
    if (curr_token.type == stop) {
      break;
    }
  }
  node = stmt;
  advance(); // skip stop
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
    Node stmt(Statement(StmtType::COMPOUND));
    advance(); // skip the {
    get_many_statements(stmt, Token::RIGHT_BRACE);
    prev.add_children(stmt);
    return get_statement(prev, stop);
  } else if (curr_token.type == Token::IF) {
    std::cout << "Found if\n";
    // if (expression) statement
    advance(); // skip the if keyword
    if (curr_token.type != Token::LEFT_PAREN) {
      // invalid if statement
      std::string msg = "invalid if statement. Expected '(', but " + curr_token.get_name() + "found";
      throw_error(msg, curr_token.line);
    }
    Node if_stmt = Node(Statement(StmtType::IF));
    advance(); // skip the (
    Node expr_start(Expression(ExprType::NONE));
    Node expr = get_expression(expr_start, Token::RIGHT_PAREN);
    if_stmt.stmt.stmt_expr.push_back(expr.expr);
    advance(); // skip the )
    prev.add_children(if_stmt);
    return get_statement(prev, stop);
  } else if (curr_token.type == Token::WHILE) {
    std::cout << "Found while\n";
    // while (expression) statement
    advance(); // skip the while keyword
    if (curr_token.type != Token::LEFT_PAREN) {
      // invalid while statement
      std::string msg = "invalid while statement. Expected '(', but " + curr_token.get_name() + "found";
      throw_error(msg, curr_token.line);
    }
    Node while_stmt = Node(Statement(StmtType::WHILE));
    advance(); // skip the (
    Node expr_start(Expression(ExprType::NONE));
    Node expr = get_expression(expr_start, Token::RIGHT_PAREN);
    while_stmt.stmt.stmt_expr.push_back(expr.expr);
    advance(); // skip the )
    prev.add_children(while_stmt);
    return get_statement(prev, stop);
  } else if (curr_token.type == Token::FOR) {
    std::cout << "Found for\n";
    // for (expression; expression; expression) statement
    advance(); // skip the for keyword
    if (curr_token.type != Token::LEFT_PAREN) {
      // invalid for statement
      std::string msg = "invalid for statement. Expected '(', but " + curr_token.get_name() + "found";
      throw_error(msg, curr_token.line);
    }
    Node for_stmt = Node(Statement(StmtType::FOR));
    advance(); // skip the (
    Node expr_start(Expression(ExprType::NONE));
    NodeList expressions = get_many_expressions(expr_start, Token::SEMI_COLON, Token::RIGHT_PAREN);
    for (auto &expr : expressions) {
      for_stmt.stmt.stmt_exprs.push_back(expr.expr);
    }
    advance(); // skip the )
    prev.add_children(for_stmt);
    return get_statement(prev, stop);
  } else if (curr_token.type == Token::RETURN) {
    std::cout << "Found return\n";
    // return expression;
    advance(); // skip the return
    Node return_stmt(Statement(StmtType::RETURN));
    Node expr_start(Expression(ExprType::NONE));
    Node return_expr = get_expression(expr_start, Token::SEMI_COLON);
    return_stmt.stmt.stmt_expr.push_back(return_expr);
    prev.add_children(return_stmt);
    advance(); // skip the semicolon
    return get_statement(prev, stop);
  } else if (curr_token.type == Token::TYPE) {
    std::cout << "Found type\n";
    // type identifier = expression;
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
      std::string msg = "invalid variable declaration. Expected '=', but " + curr_token.get_name() + "found";
      throw_error(msg, curr_token.line);
    }
    advance(); // skip the =
    Node expr_start(Expression(ExprType::NONE));
    var_decl.decl.var_expr.push_back(get_expression(expr_start, Token::SEMI_COLON));
    advance(); // skip the semicolon
    prev.add_children(var_decl);
    return get_statement(prev, stop);
  } else if (curr_token.type == Token::SEMI_COLON) {
    std::cout << "no operation\n";
    // nop;
    advance(); // skip the semicolon
    Node nop(Expression(ExprType::NOP));
    prev.add_children(nop);
    return get_statement(prev, stop);
  } else if (curr_token.type == this->terminal) {
    std::cout << "Encountered terminal - " << Token::get_name(this->terminal) << "\n";
    // end parsing
    return prev;
  } else {
    std::cout << "found an expression\n";
    // expression;
    Node expr_start(Expression(ExprType::NONE));
    Node expr = get_expression(expr_start, Token::SEMI_COLON);
    prev.add_children(expr);
    advance(); // skip the semicolon
    return get_statement(prev, stop);
  }
  throw_error("unrecognized token " + curr_token.get_name(), curr_token.line);
  return prev;
}

Node Parser::parse(int *end_pos) {
  Node start(Statement(StmtType::COMPOUND), "Compound (" + parser_name + ")");
  Node program = get_statement(start, this->terminal);
  if (end_pos != NULL) {
    *end_pos = pos;
  }
  std::cout << "Abstract syntax tree:\n";
  program.print();
  std::cout << std::endl;
  return program;
}