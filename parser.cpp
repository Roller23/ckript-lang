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

void Parser::fail_if_EOF(TokenType expected) {
  if (curr_token.type == Token::NONE) {
    ErrorHandler::thow_syntax_error("Reached end of file but " + Token::get_name(expected) + " expected");
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
      ErrorHandler::thow_syntax_error(msg);
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
      ErrorHandler::thow_syntax_error(msg);
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
      ErrorHandler::thow_syntax_error(msg);
    }
    std::string type = curr_token.value;
    advance();
    fail_if_EOF(Token::IDENTIFIER);
    if (curr_token.type != Token::IDENTIFIER) {
      std::string msg = "Invalid function declaration, expected an identifier, but " + curr_token.get_name() + " found";
      ErrorHandler::thow_syntax_error(msg);
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

NodeList Parser::get_many_expressions(TokenType sep, TokenType stop) {
  NodeList res;
  while (true) {
    fail_if_EOF(stop);
    Node expr = get_expression(sep, stop); // parse expression till either sep or stop
    if (curr_token.type == stop) {
      res.push_back(expr);
      break;
    }
    res.push_back(expr);
    advance();
    fail_if_EOF(stop);
  }
  return res;
}

Node Parser::parse_func_expr() {
  // function(arg1, arg2, ...) type statement(s);
  Node func = Node(Expression(ExprType::FUNC_EXPR), "function def");
  func.expr.func_expr.type = curr_token.type == Token::THREAD ? FuncType::THREAD : FuncType::FUNC;
  advance(); // skip the function/thread
  if (curr_token.type != Token::LEFT_PAREN) {
    std::string msg = "invalid function declaration, expected '(', but " + curr_token.get_name() + "found";
    ErrorHandler::thow_syntax_error(msg);
  }
  advance(); // skip the (
  func.expr.func_expr.params = parse_func_params();
  advance(); // skip the )
  if (curr_token.type != Token::TYPE) {
    std::string msg = "invalid function declaration, expected a type, but " + curr_token.get_name() + "found";
    ErrorHandler::thow_syntax_error(msg);
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
  func.expr.func_expr.instructions = new Node;
  int end_pos = 0;
  *func.expr.func_expr.instructions = func_parser.parse(&end_pos);
  pos += end_pos;
  advance(); // skip the semicolon
  if (!starts_with_brace) {
    retreat();
  }
  return func;
}

Node Parser::get_expression(TokenType stop1, TokenType stop2) {
  std::cout << "(" + parser_name + ") - ";
  fail_if_EOF(Token::GENERAL_EXPRESSION);
  std::cout << "consuming expression\n";
  Node res(Expression(ExprType::BOOL_EXPR), "Expression");
  while (curr_token.type != stop1 && curr_token.type != stop2) {
    if (curr_token.type == Token::FUNCTION || curr_token.type == Token::THREAD) {
      Node func = parse_func_expr();
      if (curr_token.type == stop1 || curr_token.type == stop2) {
        return func;
        break;
      }
    }
    res.expr.tokens.push_back(curr_token);
    std::cout << "(" + parser_name + ") - ";
    std::cout << "pushing " << curr_token << "\n";
    advance();
    fail_if_EOF(stop2 == Token::NONE ? stop1 : stop2);
  }
  return res;
}

NodeList Parser::get_many_statements(Node &node, TokenType stop) {
  NodeList res;
  std::cout << "consuming statements in a compound statement\n";
  while (true) {
    fail_if_EOF(stop);
    Node stmt = get_statement(node, stop);
    if (curr_token.type != stop) {
      res.push_back(stmt); 
    } else {
      break;
    }
  }
  advance(); // skip stop
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
    Node stmt(Statement(StmtType::COMPOUND), "Compund");
    advance(); // skip the {
    NodeList inner_statements = get_many_statements(stmt, Token::RIGHT_BRACE);
    stmt.add_children(inner_statements);
    // std::cout << "compound count " << inner_statements.size() << "\n";
    prev.add_children(stmt);
    return get_statement(prev, stop);
  } else if (curr_token.type == Token::IF) {
    std::cout << "Found if\n";
    // if (expression) statement
    advance(); // skip the if keyword
    if (curr_token.type != Token::LEFT_PAREN) {
      // invalid if statement
      std::string msg = "invalid if statement. Expected '(', but " + curr_token.get_name() + "found";
      ErrorHandler::thow_syntax_error(msg);
    }
    Node if_stmt = Node(Statement(StmtType::IF), "IF");
    advance(); // skip the (
    Node expr = get_expression(Token::RIGHT_PAREN);
    if_stmt.stmt.stmt_expr = expr.expr;
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
      ErrorHandler::thow_syntax_error(msg);
    }
    Node while_stmt = Node(Statement(StmtType::WHILE), "WHILE");
    advance(); // skip the (
    Node expr = get_expression(Token::RIGHT_PAREN);
    while_stmt.stmt.stmt_expr = expr.expr;
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
      ErrorHandler::thow_syntax_error(msg);
    }
    Node for_stmt = Node(Statement(StmtType::FOR), "FOR");
    advance(); // skip the (
    NodeList expressions = get_many_expressions(Token::SEMI_COLON, Token::RIGHT_PAREN);
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
    Node return_stmt(Statement(StmtType::RETURN), "RETURN");
    Node return_expr = get_expression(Token::SEMI_COLON);
    return_stmt.stmt.stmt_expr = return_expr.expr;
    prev.add_children(return_stmt);
    advance(); // skip the semicolon
    return get_statement(prev, stop);
  } else if (curr_token.type == Token::TYPE) {
    std::cout << "Found type\n";
    // type identifier = expression;
    Node var_decl = Node(Declaration(DeclType::VAR_DECL), "VAR");
    var_decl.decl.var_type = curr_token.value;
    advance(); // skip the variable type
    if (curr_token.type != Token::IDENTIFIER) {
      std::string msg = "invalid variable declaration. Expected an identifier, but " + curr_token.get_name() + " found";
      ErrorHandler::thow_syntax_error(msg);
    }
    var_decl.decl.id = curr_token.value; // set the declaration identifier to token value
    advance(); // skip the identifier
    if (curr_token.type != Token::OP_ASSIGN) {
      std::string msg = "invalid variable declaration. Expected '=', but " + curr_token.get_name() + "found";
      ErrorHandler::thow_syntax_error(msg);
    }
    advance(); // skip the =
    var_decl.decl.var_expr = new Node; // to do, find a workaround for this
    *var_decl.decl.var_expr = get_expression(Token::SEMI_COLON);
    advance(); // skip the semicolon
    prev.add_children(var_decl);
    return get_statement(prev, stop);
  } else if (curr_token.type == Token::SEMI_COLON) {
    std::cout << "no operation\n";
    // nop;
    advance(); // skip the semicolon
    Node nop(Expression(ExprType::NOP), "NOP");
    prev.add_children(nop);
    return prev;
  } else if (curr_token.type == this->terminal) {
    std::cout << "Encountered terminal - " << Token::get_name(this->terminal) << "\n";
    // end parsing
    return prev;
  } else {
    std::cout << "found an expression\n";
    // expression;
    Node expr = get_expression(Token::SEMI_COLON);
    prev.add_children(expr);
    advance(); // skip the semicolon
    return get_statement(prev, stop);
  }
  ErrorHandler::thow_syntax_error("unrecognized token " + curr_token.get_name());
  return prev;
}

Node Parser::parse(int *end_pos) {
  Node start(Statement(StmtType::COMPOUND), "Compound (" + parser_name + ")");
  Node program = get_statement(start, this->terminal);
  if (end_pos != NULL) {
    *end_pos = pos;
  }
  std::cout << "AST:\n";
  program.print(program.name);
  return program;
}