#if !defined(__AST_)
#define __AST_

#include <vector>
#include <string>
#include <cstdint>
#include "token.hpp"

class Node;
class FuncParam;
class Expression;

typedef std::vector<Node> NodeList;
typedef std::vector<NodeList> NodeListList;
typedef std::vector<FuncParam> ParamList;
typedef std::vector<Expression> ExpressionList;

class FuncParam {
  public:
    std::string type_name = "int";
    std::string param_name = "";
    bool is_ref = false;
    FuncParam(const std::string &_type, const std::string &_name) 
      : type_name(_type), param_name(_name) {};
};

class FuncCall {
  public:
    NodeListList arguments;
};

class FuncExpression {
  public:
    ParamList params;
    std::string ret_type = "void";
    bool ret_ref = false;
    bool captures = false;
    NodeList instructions;
};

class ClassStatement {
  public:
    ParamList members;
    std::string class_name = "";
};

class Expression {
  public:
    typedef enum expr_type {
      BINARY_OP, UNARY_OP, FUNC_CALL, FUNC_EXPR, NUM_EXPR, FLOAT_EXPR, STR_EXPR,
      IDENTIFIER_EXPR, BOOL_EXPR, NOP, RPN, LPAREN, RPAREN, INDEX, ARRAY, NONE
    } ExprType;
    ExprType type;
    NodeList rpn_stack;
    FuncExpression func_expr;
    FuncCall func_call;
    NodeList index;
    NodeListList array_expressions;
    std::string array_type;
    bool array_holds_refs = false;
    NodeList array_size;
    bool is_negative = false;
    std::int64_t number_literal = 0;
    std::string string_literal = "";
    std::string id_name = "";
    Token::TokenType op;
    double float_literal = 0.0f;
    bool bool_literal = false;
    bool is_operation() const;
    bool is_evaluable();
    bool is_paren() const;
    Expression(void) : type(NONE) {};
    Expression(Token::TokenType _paren) : type(_paren == '(' ? LPAREN : RPAREN) {};
    Expression(Token::TokenType _op, ExprType _type) : type(_type), op(_op) {};
    Expression(const NodeList &rpn) : type(RPN), rpn_stack(rpn) {};
    Expression(const NodeList &rpn, bool is_index) : type(INDEX), index(rpn), op(Token::LEFT_BRACKET) {}
    Expression(const bool boolean, const double lol) : type(BOOL_EXPR), bool_literal(boolean) {};
    Expression(ExprType _type) : type(_type) {};
    Expression(const FuncExpression &fn) : type(FUNC_EXPR), func_expr(fn) {}
    Expression(const FuncCall &call) : type(FUNC_CALL), func_call(call) {}
    Expression(const std::string &literal) : type(STR_EXPR), string_literal(literal) {}
    Expression(const std::string &_id, bool identifier) : type(IDENTIFIER_EXPR), id_name(_id) {} 
    Expression(const std::uint64_t literal, bool neg, bool is_int) : 
      type(NUM_EXPR),
      number_literal(literal),
      is_negative(neg) {}
    Expression(const double literal, bool is_double) : type(FLOAT_EXPR), float_literal(literal) {};
    void print(int nest = 0);
};

class Statement {
  public:
    typedef enum stmt_type {
      IF, RETURN, WHILE, FOR, COMPOUND, EXPR, UNKNOWN, NOP, DECL, CLASS, BREAK, CONTINUE, SET, SET_IDX, NONE
    } StmtType;
    StmtType type;
    NodeListList expressions;
    NodeList declaration;
    NodeList statements;
    NodeList indexes;
    ClassStatement class_stmt;
    std::vector<std::string> obj_members;
    std::uint64_t line = 0;
    std::string *source = nullptr;
    Statement(void) : type(NONE) {}
    Statement(StmtType _type) : type(_type) {}
    Statement(const ClassStatement &_class) : type(CLASS), class_stmt(_class) {};
    void print(int nest = 0);
};

class Declaration {
  public:
    typedef enum decl_type {
      VAR_DECL, NONE
    } DeclType;
    std::string var_type = "";
    std::string id = "";
    bool constant = false;
    bool allocated = false;
    bool reference = false;
    NodeList var_expr;
    DeclType type;
    Declaration(void) : type(DeclType::NONE) {};
    Declaration(DeclType _type) : type(_type) {};
    void print(int nest = 0);
};

class Node {
  public:
    typedef enum node_type {
      EXPR, STMT, DECL, UNKNOWN
    } NodeType;
    Expression expr;
    Statement stmt;
    Declaration decl;
    NodeList children;
    NodeType type;
    Node(void) : type(NodeType::UNKNOWN) {};
    Node(const Expression &e) :
      type(NodeType::EXPR), expr(e) {};
    Node(const Statement &s) :
      type(NodeType::STMT), stmt(s) {};
    Node(const Declaration &d) :
      type(NodeType::DECL), decl(d) {};
    Node(const FuncCall &c) : type(NodeType::EXPR), expr(Expression(c)) {};
    void add_children(Node &node);
    void add_children(NodeList &nodes);
    virtual void print(int nest = 0);
    static void print_nesting(int nest);
};

#endif // __AST_