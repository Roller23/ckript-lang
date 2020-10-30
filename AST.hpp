#if !defined(__AST_)
#define __AST_

#include <vector>
#include <string>

class Node;

typedef std::vector<Node> Nodes;

class Node {
  public:
    typedef enum node_type {
      EXPR, STMT, DECL
    } NodeType;
    Nodes children;
    NodeType type;
    std::string name;
    Node(NodeType _type, std::string _name = "") : type(_type), name(_name) {};
};

class Expression : Node {
  public:
    typedef enum expr_type {
      BINARY_OP, FUNC_CALL, NUM_EXPR, STR_EXPR, IDENTIFIER_EXPR, BOOL_EXPR, NOP
    } ExprType;
    ExprType type;
    Expression(ExprType _type) : type(_type), Node(NodeType::EXPR) {};
};

class Statement : Node {
  public:
    typedef enum stmt_type {
      IF, RETURN, WHILE, FOR, COMPOUND
    } StmtType;
    StmtType type;
    Statement(StmtType _type) : type(_type), Node(NodeType::STMT) {};
};

class Declaration : Node {
  public:
    typedef enum decl_type {
      FUNC_DECL, VAR_DECL
    } DeclType;
    DeclType type;
    Declaration(DeclType _type) : type(_type), Node(NodeType::DECL) {};
};

#endif // __AST_