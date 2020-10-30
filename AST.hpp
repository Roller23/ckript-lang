#if !defined(__AST_)
#define __AST_

#include <vector>
#include <string>
#include "token.hpp"

class Expression {
  public:
    typedef enum expr_type {
      BINARY_OP, FUNC_CALL, NUM_EXPR, STR_EXPR, IDENTIFIER_EXPR, BOOL_EXPR, NOP, NONE
    } ExprType;
    ExprType type;
    TokenList tokens;
    Expression(void) : type(ExprType::NONE) {};
    Expression(ExprType _type) : type(_type) {};
    void print(const std::string &name, int nest = 0);
};

class Statement {
  public:
    typedef enum stmt_type {
      IF, RETURN, WHILE, FOR, COMPOUND, EXPR, UNKNOWN, NONE
    } StmtType;
    StmtType type;
    Expression stmt_expr;
    Statement(void) : type(StmtType::NONE) {};
    Statement(StmtType _type) : type(_type) {};
    void print(const std::string &name, int nest = 0);
};

class Declaration {
  public:
    typedef enum decl_type {
      FUNC_DECL, VAR_DECL, NONE
    } DeclType;
    DeclType type;
    Declaration(void) : type(DeclType::NONE) {};
    Declaration(DeclType _type) : type(_type) {};
    void print(const std::string &name, int nest = 0);
};

class Node;

typedef std::vector<Node> NodeList;

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
    Node(Expression e, std::string _name = "") : type(NodeType::EXPR), expr(e), name(_name) {};
    Node(Statement s, std::string _name = "") : type(NodeType::STMT), stmt(s), name(_name) {};
    Node(Declaration d, std::string _name = ""): type(NodeType::DECL), decl(d), name(_name) {};
    void add_children(Node &node);
    void add_children(NodeList &nodes);
    virtual void print(const std::string &name, int nest = 0);
    static void print_nesting(int nest);
};

#endif // __AST_