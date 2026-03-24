#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "common/location.h"

namespace pascal_s2c {

// 基础类型枚举。
// 使用位置：
// 1) Parser 在解析类型关键字时写入（如 integer/real）。
// 2) SemanticAnalyzer 将其映射到 TypeInfo 做类型检查。
// 3) CCodeGenerator 按此决定 C 侧类型名（int/float/bool/char）。
// 存储信息：Pascal 标量类型与内部 void/string 占位类型。
enum class BasicTypeKind {
    Integer,
    Real,
    Boolean,
    Char,
    String,
    Void
};

// AST 基类。
// 使用位置：
// 1) 所有 AST 节点的公共父类。
// 2) Parser 在构建节点时填写 loc，供错误定位与调试。
// 存储信息：源码位置信息（行/列）。
struct Node {
    SourceLocation loc{};
    virtual ~Node() = default;
};

// 类型节点基类。
// 使用位置：
// 1) VarDeclNode 的 type 以多态方式挂接具体类型。
// 2) SemanticAnalyzer/CodeGenerator 通过 dynamic_cast 区分标量或数组。
// 存储信息：继承 Node 的位置信息，无额外字段。
struct TypeNode : Node {
    ~TypeNode() override = default;
};

// 标量类型节点，例如 integer/real/boolean/char。
// 使用位置：
// 1) Parser 解析 basic type 时创建。
// 2) SemanticAnalyzer 读取 kind 生成 TypeInfo。
// 存储信息：标量类型 kind。
struct ScalarTypeNode : TypeNode {
    BasicTypeKind kind = BasicTypeKind::Void;
};

// 数组单维边界。
// 使用位置：
// 1) Parser 解析 array[lb..ub] 时填充。
// 2) CodeGenerator 计算 C 数组长度与下标修正（i - lb）。
// 存储信息：维度下界 lower、上界 upper。
struct ArrayBound {
    int lower = 0;
    int upper = 0;
};

// 数组类型节点。
// 使用位置：
// 1) Parser 解析 array[...] of <basic_type> 时创建。
// 2) SemanticAnalyzer 读取 dims 与 elementType 形成数组 TypeInfo。
// 3) CodeGenerator 用 dims 计算声明长度和访问偏移。
// 存储信息：
// - dims: 每一维的边界列表
// - elementType: 元素基础类型（当前实现为标量类型）
struct ArrayTypeNode : TypeNode {
    std::vector<ArrayBound> dims;
    std::unique_ptr<ScalarTypeNode> elementType;
};

// 表达式节点基类。
// 使用位置：
// 1) 所有右值/条件/调用参数等表达式的公共父类。
// 2) SemanticAnalyzer 在 expressionTypes 中按 Expr* 记录推断类型。
// 存储信息：仅继承位置。
struct Expr : Node {
    ~Expr() override = default;
};

// 声明节点基类。
// 使用位置：常量声明、变量声明、子程序声明统一抽象。
// 存储信息：仅继承位置。
struct Decl : Node {
    ~Decl() override = default;
};

// 常量声明节点：const a = <expr>。
// 使用位置：
// 1) Parser 在 const 区构建。
// 2) SemanticAnalyzer 对 value 做类型推断并写入符号表。
// 3) CodeGenerator 输出 const 声明。
// 存储信息：常量名 name、初始化表达式 value。
struct ConstDeclNode : Decl {
    std::string name;
    std::unique_ptr<Expr> value;
};

// 变量声明节点：a, b: <type>。
// 使用位置：
// 1) Parser 在 var 区构建。
// 2) SemanticAnalyzer 为 names 中每个名字建 Symbol。
// 3) CodeGenerator 输出变量定义（含数组后缀）。
// 存储信息：变量名列表 names、声明类型 type。
struct VarDeclNode : Decl {
    std::vector<std::string> names;
    std::unique_ptr<TypeNode> type;
};

// 参数传递方式。
// 使用位置：
// 1) Parser 识别 "var" 前缀时写入 Var。
// 2) SemanticAnalyzer 生成参数符号与调用签名。
// 3) CodeGenerator 决定指针参数与按地址传参。
// 存储信息：值传递(Value) / 引用传递(Var)。
enum class ParamPassMode {
    Value,
    Var
};

// 形参组节点：可一次声明多个同类型参数。
// 示例：var x, y: integer
// 使用位置：
// 1) Parser 在函数/过程头构建。
// 2) SemanticAnalyzer 展开 names 写入参数符号。
// 3) CodeGenerator 生成函数签名参数列表。
// 存储信息：参数名列表 names、基础类型 type、传参方式 passMode。
struct ParamDeclNode : Node {
    std::vector<std::string> names;
    BasicTypeKind type = BasicTypeKind::Void;
    ParamPassMode passMode = ParamPassMode::Value;
};

// 语句节点基类。
// 使用位置：复合语句、控制流、赋值、IO、调用等统一抽象。
// 存储信息：仅继承位置。
struct Stmt : Node {
    ~Stmt() override = default;
};

// 复合语句节点：begin ... end。
// 使用位置：
// 1) Parser 作为 block 主体及嵌套语句块。
// 2) SemanticAnalyzer 递归分析子语句。
// 3) CodeGenerator 输出大括号代码块。
// 存储信息：语句序列 statements。
struct CompoundStmtNode : Stmt {
    std::vector<std::unique_ptr<Stmt>> statements;
};

// break 语句节点。
// 使用位置：Parser 构建；CodeGenerator 输出 break;。
// 存储信息：仅位置信息。
struct BreakStmtNode : Stmt {};

// 赋值语句节点：target := value。
// 使用位置：
// 1) Parser 由标识符引导语句消歧后构建。
// 2) SemanticAnalyzer 校验左右值类型兼容与左值合法性。
// 3) CodeGenerator 输出赋值语句。
// 存储信息：左值表达式 target、右值表达式 value。
struct AssignStmtNode : Stmt {
    std::unique_ptr<Expr> target;
    std::unique_ptr<Expr> value;
};

// 调用语句节点：procedure 调用或无返回值调用语句。
// 使用位置：
// 1) Parser 解析 foo(...) 或裸 foo。
// 2) SemanticAnalyzer 做可调用符号和实参检查。
// 3) CodeGenerator 输出调用语句。
// 存储信息：被调名称 name、实参 args。
struct CallStmtNode : Stmt {
    std::string name;
    std::vector<std::unique_ptr<Expr>> args;
};

// if 语句节点。
// 使用位置：Parser 构建控制流；SemanticAnalyzer 分析条件与分支；
// CodeGenerator 输出 if/else。
// 存储信息：条件 condition、then 分支、可选 else 分支。
struct IfStmtNode : Stmt {
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> thenBranch;
    std::unique_ptr<Stmt> elseBranch;
};

// while 语句节点。
// 使用位置：Parser/Analyzer/CodeGenerator 全流程。
// 存储信息：循环条件 condition、循环体 body。
struct WhileStmtNode : Stmt {
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> body;
};

// for-to 语句节点（当前实现只支持 to）。
// 使用位置：
// 1) Parser 解析 for i := start to stop do body。
// 2) SemanticAnalyzer 校验循环变量为整数。
// 3) CodeGenerator 输出 C for 循环。
// 存储信息：循环变量名 varName、起点 start、终点 stop、循环体 body。
struct ForStmtNode : Stmt {
    std::string varName;
    std::unique_ptr<Expr> start;
    std::unique_ptr<Expr> stop;
    std::unique_ptr<Stmt> body;
};

// read 语句节点。
// 使用位置：
// 1) Parser 解析 read(a, arr[i])。
// 2) SemanticAnalyzer 检查目标必须是可写左值。
// 3) CodeGenerator 按目标类型输出 scanf。
// 存储信息：输入目标列表 targets。
struct ReadStmtNode : Stmt {
    std::vector<std::unique_ptr<Expr>> targets;
};

// write 语句节点。
// 使用位置：
// 1) Parser 解析 write(expr1, expr2, ...)。
// 2) SemanticAnalyzer 推断每个表达式类型。
// 3) CodeGenerator 拼接 printf 格式串和参数列表。
// 存储信息：输出表达式列表 values。
struct WriteStmtNode : Stmt {
    std::vector<std::unique_ptr<Expr>> values;
};

// 二元运算符枚举。
// 使用位置：
// 1) Parser 将 token 映射为 BinaryOp。
// 2) SemanticAnalyzer 按运算符做类型检查。
// 3) CodeGenerator 输出对应 C 运算符及必要转换（如 RealDiv）。
// 存储信息：算术、关系、逻辑运算类别。
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

// 一元运算符枚举。
// 使用位置：Parser 构建一元表达式；CodeGenerator 输出前缀运算。
// 存储信息：正号、负号、逻辑非。
enum class UnaryOp {
    Plus,
    Minus,
    Not
};

// 二元表达式节点。
// 示例：a + b、x < y、a div b。
// 使用位置：
// 1) Parser 在加法/乘法/关系层构建。
// 2) SemanticAnalyzer 推断表达式结果类型。
// 3) CodeGenerator 生成运算表达式并处理优先级括号。
// 存储信息：运算符 op、左操作数 lhs、右操作数 rhs。
struct BinaryExprNode : Expr {
    BinaryOp op = BinaryOp::Add;
    std::unique_ptr<Expr> lhs;
    std::unique_ptr<Expr> rhs;
};

// 一元表达式节点。
// 示例：-x、not flag。
// 使用位置：Parser 构建；Analyzer/CodeGenerator 读取。
// 存储信息：运算符 op、操作数 operand。
struct UnaryExprNode : Expr {
    UnaryOp op = UnaryOp::Plus;
    std::unique_ptr<Expr> operand;
};

// 调用表达式节点（有返回值语境中的调用）。
// 示例：f(a, b) 出现在赋值右值或更大表达式中。
// 使用位置：
// 1) Parser 在表达式语境解析调用。
// 2) SemanticAnalyzer 检查可调用性、参数匹配和返回类型。
// 3) CodeGenerator 输出函数调用表达式。
// 存储信息：被调名称 name、实参 args。
struct CallExprNode : Expr {
    std::string name;
    std::vector<std::unique_ptr<Expr>> args;
};

// 变量引用表达式节点。
// 示例：a
// 使用位置：
// 1) Parser 构建最基础标识符表达式。
// 2) SemanticAnalyzer 绑定到具体 Symbol。
// 3) CodeGenerator 输出变量名或 var 参数解引用。
// 存储信息：变量名 name。
struct VarExprNode : Expr {
    std::string name;
};

// 数组索引表达式节点。
// 示例：arr[i]、mat[i, j]
// 使用位置：
// 1) Parser 解析下标访问。
// 2) SemanticAnalyzer 检查维度数与索引表达式类型。
// 3) CodeGenerator 按 lower bound 做下标修正。
// 存储信息：数组名 baseName、各维索引表达式 indices。
struct IndexExprNode : Expr {
    std::string baseName;
    std::vector<std::unique_ptr<Expr>> indices;
};

// 字面量种类。
// 使用位置：Parser 为 LiteralExprNode 赋 kind；
// SemanticAnalyzer/CodeGenerator 根据 kind 决定类型与输出形式。
// 存储信息：整型、实型、布尔、字符、字符串字面量分类。
enum class LiteralKind {
    Int,
    Real,
    Bool,
    Char,
    String
};

// 字面量表达式节点。
// 示例：123、3.14、'a'、'hello'、true
// 使用位置：
// 1) Parser 从 token 构建。
// 2) SemanticAnalyzer 由 kind 推断类型。
// 3) CodeGenerator 按 rawText 还原并做必要转义处理。
// 存储信息：字面量类别 kind、原始文本 rawText。
struct LiteralExprNode : Expr {
    LiteralKind kind = LiteralKind::Int;
    std::string rawText;
};

// 前向声明，解决子程序与 block 的循环依赖。
struct BlockNode;

// 子程序声明基类（function/procedure 共享结构）。
// 使用位置：
// 1) Parser 先解析头再解析子 block。
// 2) SemanticAnalyzer 用于建立调用签名和局部作用域。
// 3) CodeGenerator 输出函数/过程定义。
// 存储信息：名称 name、形参列表 params、函数体 block。
struct SubprogramDeclNode : Decl {
    std::string name;
    std::vector<std::unique_ptr<ParamDeclNode>> params;
    std::unique_ptr<BlockNode> block;
    ~SubprogramDeclNode() override = default;
};

// 函数声明节点（有返回类型）。
// 使用位置：SemanticAnalyzer/CodeGenerator 读取 returnType。
// 存储信息：在公共子程序信息基础上增加 returnType。
struct FunctionDeclNode : SubprogramDeclNode {
    BasicTypeKind returnType = BasicTypeKind::Void;
};

// 过程声明节点（无返回值）。
// 使用位置：与 FunctionDeclNode 并列，区分调用语义与代码生成签名。
// 存储信息：复用 SubprogramDeclNode 字段。
struct ProcedureDeclNode : SubprogramDeclNode {};

// block 节点：一个作用域内的完整内容。
// 使用位置：
// 1) Program 主体和每个子程序主体都对应一个 BlockNode。
// 2) SemanticAnalyzer 以 block 为粒度建立作用域并声明符号。
// 3) CodeGenerator 按 block 输出声明和语句。
// 存储信息：常量声明、变量声明、子程序声明、复合语句主体。
struct BlockNode : Node {
    std::vector<std::unique_ptr<ConstDeclNode>> constDecls;
    std::vector<std::unique_ptr<VarDeclNode>> varDecls;
    std::vector<std::unique_ptr<SubprogramDeclNode>> subprograms;
    std::unique_ptr<CompoundStmtNode> body;
};

// 程序根节点。
// 使用位置：
// 1) Parser::parse 返回 ProgramNode。
// 2) Compiler 流水线各阶段（语义/降级/代码生成）的统一输入。
// 存储信息：程序名 name、顶层 block。
struct ProgramNode : Node {
    std::string name;
    std::unique_ptr<BlockNode> block;
};

// ProgramNode 智能指针别名，表示 AST 根所有权。
using ProgramPtr = std::unique_ptr<ProgramNode>;

// 构造占位 Program 的辅助接口（用于测试或错误回退场景）。
ProgramPtr makePlaceholderProgram(std::string name);

}  // namespace pascal_s2c
