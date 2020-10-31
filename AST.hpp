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
typedef std::vector<FuncParam> ParamList;
typedef std::vector<Expression> ExpressionList;

class FuncParam {
  public:
    std::string type_name;
    std::string param_name;
    FuncParam(const std::string &_type, const std::string &_name) : type_name(_type), param_name(_name) {};
};

class FuncCall {
  public:
    std::string name;
    ExpressionList arguments;
    FuncCall(const std::string &_name = "") : name(_name) {}
};

class FuncExpression {
  public:
    typedef enum func_type {
      THREAD, FUNC, NONE
    } FuncType;
    FuncType type;
    ParamList params;
    std::string ret_type;
    Node *instructions;
    FuncExpression(void) : type(FuncType::NONE) {};
    FuncExpression(FuncType _type) : type(_type) {};
};

class Expression {
  public:
    typedef enum expr_type {
      BINARY_OP, FUNC_CALL, FUNC_EXPR, NUM_EXPR, FLOAT_EXPR, STR_EXPR, IDENTIFIER_EXPR, BOOL_EXPR, NOP, NONE
    } ExprType;
    ExprType type;
    TokenList tokens;
    FuncExpression func_expr;
    FuncCall func_call;
    std::uint64_t number_literal = 0;
    std::string string_literal = "";
    double float_literal = 0;
    bool bool_literal = false;
    Expression(void) : type(ExprType::NONE) {};
    Expression(ExprType _type) : type(_type) {};
    Expression(const FuncExpression &fn) : type(FUNC_EXPR), func_expr(fn) {}
    Expression(const FuncCall &call) : type(FUNC_CALL), func_call(call) {}
    Expression(const std::string &literal) : type(STR_EXPR), string_literal(literal) {}
    Expression(const std::uint64_t literal) : type(NUM_EXPR), number_literal(literal) {}
    Expression(const double literal, bool is_double) : type(FLOAT_EXPR), float_literal(literal) {}
    void print(const std::string &name, int nest = 0);
};

class Statement {
  public:
    typedef enum stmt_type {
      IF, RETURN, WHILE, FOR, COMPOUND, EXPR, UNKNOWN, NONE
    } StmtType;
    StmtType type;
    Expression stmt_expr;
    ExpressionList stmt_exprs;
    Statement(void) : type(StmtType::NONE) {};
    Statement(StmtType _type) : type(_type) {};
    void print(const std::string &name, int nest = 0);
};

class Declaration {
  public:
    typedef enum decl_type {
      VAR_DECL, NONE
    } DeclType;
    std::string var_type = "";
    std::string id = "";
    Node *var_expr; // to avoid forward declaration
    DeclType type;
    Declaration(void) : type(DeclType::NONE) {};
    Declaration(DeclType _type) : type(_type) {};
    void print(const std::string &name, int nest = 0);
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
    std::string name;
    Node(void) : type(NodeType::UNKNOWN), name("") {};
    Node(Expression e, std::string _name = "") : type(NodeType::EXPR), expr(e), name(_name) {};
    Node(Statement s, std::string _name = "") : type(NodeType::STMT), stmt(s), name(_name) {};
    Node(Declaration d, std::string _name = ""): type(NodeType::DECL), decl(d), name(_name) {};
    Node(FuncCall c) : type(NodeType::EXPR), expr(Expression(c)) {};
    void add_children(Node &node);
    void add_children(NodeList &nodes);
    virtual void print(const std::string &name, int nest = 0);
    static void print_nesting(int nest);
};

#endif // __AST_