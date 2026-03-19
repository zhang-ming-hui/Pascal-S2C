#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "common/location.h"

namespace pascal_s2c {

enum class BasicTypeKind {
    Integer,
    Real,
    Boolean,
    Char,
    Void
};

struct Node {
    SourceLocation loc{};
    virtual ~Node() = default;
};

struct TypeNode : Node {
    ~TypeNode() override = default;
};

struct ScalarTypeNode : TypeNode {
    BasicTypeKind kind = BasicTypeKind::Void;
};

struct ArrayBound {
    int lower = 0;
    int upper = 0;
};

struct ArrayTypeNode : TypeNode {
    std::vector<ArrayBound> dims;
    std::unique_ptr<ScalarTypeNode> elementType;
};

struct Expr : Node {
    ~Expr() override = default;
};

struct Decl : Node {
    ~Decl() override = default;
};

struct ConstDeclNode : Decl {
    std::string name;
    std::unique_ptr<Expr> value;
};

struct VarDeclNode : Decl {
    std::vector<std::string> names;
    std::unique_ptr<TypeNode> type;
};

enum class ParamPassMode {
    Value,
    Var
};

struct ParamDeclNode : Node {
    std::vector<std::string> names;
    BasicTypeKind type = BasicTypeKind::Void;
    ParamPassMode passMode = ParamPassMode::Value;
};

struct Stmt : Node {
    ~Stmt() override = default;
};

struct CompoundStmtNode : Stmt {
    std::vector<std::unique_ptr<Stmt>> statements;
};

struct AssignStmtNode : Stmt {
    std::unique_ptr<Expr> target;
    std::unique_ptr<Expr> value;
};

struct CallStmtNode : Stmt {
    std::string name;
    std::vector<std::unique_ptr<Expr>> args;
};

struct IfStmtNode : Stmt {
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> thenBranch;
    std::unique_ptr<Stmt> elseBranch;
};

struct WhileStmtNode : Stmt {
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> body;
};

struct ForStmtNode : Stmt {
    std::string varName;
    std::unique_ptr<Expr> start;
    std::unique_ptr<Expr> stop;
    std::unique_ptr<Stmt> body;
};

struct ReadStmtNode : Stmt {
    std::vector<std::unique_ptr<Expr>> targets;
};

struct WriteStmtNode : Stmt {
    std::vector<std::unique_ptr<Expr>> values;
};

enum class BinaryOp {
    Add,
    Sub,
    Mul,
    RealDiv,
    IntDiv,
    Mod,
    Eq,
    Ne,
    Lt,
    Le,
    Gt,
    Ge,
    And,
    Or
};

enum class UnaryOp {
    Plus,
    Minus,
    Not
};

struct BinaryExprNode : Expr {
    BinaryOp op = BinaryOp::Add;
    std::unique_ptr<Expr> lhs;
    std::unique_ptr<Expr> rhs;
};

struct UnaryExprNode : Expr {
    UnaryOp op = UnaryOp::Plus;
    std::unique_ptr<Expr> operand;
};

struct CallExprNode : Expr {
    std::string name;
    std::vector<std::unique_ptr<Expr>> args;
};

struct VarExprNode : Expr {
    std::string name;
};

struct IndexExprNode : Expr {
    std::string baseName;
    std::vector<std::unique_ptr<Expr>> indices;
};

enum class LiteralKind {
    Int,
    Real,
    Bool,
    Char
};

struct LiteralExprNode : Expr {
    LiteralKind kind = LiteralKind::Int;
    std::string rawText;
};

struct BlockNode;

struct SubprogramDeclNode : Decl {
    std::string name;
    std::vector<std::unique_ptr<ParamDeclNode>> params;
    std::unique_ptr<BlockNode> block;
    ~SubprogramDeclNode() override = default;
};

struct FunctionDeclNode : SubprogramDeclNode {
    BasicTypeKind returnType = BasicTypeKind::Void;
};

struct ProcedureDeclNode : SubprogramDeclNode {};

struct BlockNode : Node {
    std::vector<std::unique_ptr<ConstDeclNode>> constDecls;
    std::vector<std::unique_ptr<VarDeclNode>> varDecls;
    std::vector<std::unique_ptr<SubprogramDeclNode>> subprograms;
    std::unique_ptr<CompoundStmtNode> body;
};

struct ProgramNode : Node {
    std::string name;
    std::unique_ptr<BlockNode> block;
};

using ProgramPtr = std::unique_ptr<ProgramNode>;

ProgramPtr makePlaceholderProgram(std::string name);

}  // namespace pascal_s2c
