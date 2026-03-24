#include "parser/parser.h"

#include <cstdlib>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "common/error.h"

namespace pascal_s2c {

namespace {

class ParserImpl {
public:
    explicit ParserImpl(const TokenList& tokens) : tokens_(tokens) {}

    // 解析入口：Program -> program 头 + block + 结尾句点。
    // 输入：完整 token 序列。
    // 输出：ProgramNode 根节点。
    // 示例：program main; var a: integer; begin a := 3; end.
    ProgramPtr parseProgram() {
        // program <标识符> [(...)] ; <block> .
        const Token programToken = expect(TokenKind::Program, "expected 'program'");
        const Token name = expect(TokenKind::Identifier, "expected program name");

        if (match(TokenKind::LParen)) {
            if (!check(TokenKind::RParen)) {
                parseProgramHeaderElement();
                while (match(TokenKind::Comma)) {
                    parseProgramHeaderElement();
                }
            }
            expect(TokenKind::RParen, "expected ')' after program header");
        }

        expect(TokenKind::Semicolon, "expected ';' after program header");

        auto program = std::make_unique<ProgramNode>();
        program->loc = programToken.location;
        program->name = name.lexeme;
        program->block = parseBlock();

        expect(TokenKind::Dot, "expected '.' at end of program");
        expect(TokenKind::EndOfFile, "expected end of file");
        return program;
    }

private:
    // 返回当前游标指向的 token；越界时兜底返回 EOF token。
    // 示例：index_ 指向 "begin" 时，current().kind == TokenKind::Begin。
    const Token& current() const {
        if (index_ < tokens_.size()) {
            return tokens_[index_];
        }
        return tokens_.back();
    }

    // 返回上一个已消费 token。
    // 示例：刚匹配完 Identifier("main") 后，previous().lexeme == "main"。
    const Token& previous() const {
        return tokens_[index_ - 1];
    }

    // 判断是否到达输入末尾。
    // 示例：current().kind 为 EndOfFile 时返回 true。
    bool isAtEnd() const {
        return current().kind == TokenKind::EndOfFile;
    }

    // 仅查看当前 token 是否为指定 kind，不前进游标。
    // 示例：check(TokenKind::Semicolon) 用于判断语句结束符。
    bool check(TokenKind kind) const {
        return current().kind == kind;
    }

    // 若当前 token 匹配 kind，则消费并返回 true；否则不消费并返回 false。
    // 示例：match(TokenKind::Else) 用于可选 else 分支。
    bool match(TokenKind kind) {
        if (!check(kind)) {
            return false;
        }
        ++index_;
        return true;
    }
    // 强制消费指定 token；不匹配则抛出定位准确的语法错误。
    // 示例：expect(TokenKind::Semicolon, "...") 要求声明行必须以分号结尾。
    Token expect(TokenKind kind, const std::string& message) {
        if (!check(kind)) {
            throw CompilerError("parser", message, current().location);
        }
        return tokens_[index_++];
    }

    // 解析 program 头中的一个元素：identifier 或 identifier[expr,...]。
    // 示例：program gcd(arg[2]); 中的 arg[2]。
    void parseProgramHeaderElement() {
        expect(TokenKind::Identifier, "expected identifier in program header");
        if (match(TokenKind::LBracket)) {
            if (!check(TokenKind::RBracket)) {
                parseExpression();
                while (match(TokenKind::Comma)) {
                    parseExpression();
                }
            }
            expect(TokenKind::RBracket, "expected ']' in program header");
        }
    }

    // 解析 block：const -> var -> subprogram* -> compound statement。
    // 示例：
    //   const a = 1;
    //   var x: integer;
    //   begin x := a; end
    std::unique_ptr<BlockNode> parseBlock() {
        auto block = std::make_unique<BlockNode>();
        block->loc = current().location;

        // 声明区顺序遵循 Pascal 的 block 布局。
        parseConstDeclarations(*block);
        if (match(TokenKind::Type)) {
            throw CompilerError("parser", "type declarations are not implemented yet", previous().location);
        }
        parseVarDeclarations(*block);

        while (check(TokenKind::Function) || check(TokenKind::Procedure)) {
            block->subprograms.push_back(parseSubprogram());
            expect(TokenKind::Semicolon, "expected ';' after subprogram declaration");
        }

        block->body = parseCompoundStatement();
        return block;
    }

    // 解析常量声明区。
    // 示例：const a = 10; b = -2;
    // 结果：block.constDecls 追加两个 ConstDeclNode。
    void parseConstDeclarations(BlockNode& block) {
        if (!match(TokenKind::Const)) {
            return;
        }

        while (check(TokenKind::Identifier)) {
            auto decl = std::make_unique<ConstDeclNode>();
            decl->loc = current().location;
            decl->name = expect(TokenKind::Identifier, "expected constant name").lexeme;
            expect(TokenKind::Equal, "expected '=' in const declaration");
            decl->value = parseExpression();
            block.constDecls.push_back(std::move(decl));
            expect(TokenKind::Semicolon, "expected ';' after const declaration");
        }
    }

    // 解析变量声明区。
    // 示例：var a, b: integer; x: real;
    // 结果：block.varDecls 追加对应的 VarDeclNode。
    void parseVarDeclarations(BlockNode& block) {
        if (!match(TokenKind::Var)) {
            return;
        }

        while (check(TokenKind::Identifier)) {
            block.varDecls.push_back(parseVarDeclaration());
            expect(TokenKind::Semicolon, "expected ';' after variable declaration");
        }
    }

    // 解析单条变量声明：idlist : type。
    // 示例：a, b: integer
    std::unique_ptr<VarDeclNode> parseVarDeclaration() {
        auto decl = std::make_unique<VarDeclNode>();
        decl->loc = current().location;
        decl->names = parseIdentifierList();
        expect(TokenKind::Colon, "expected ':' after identifier list");
        decl->type = parseType();
        return decl;
    }

    // 解析标识符列表。
    // 示例：a, b, c -> ["a", "b", "c"]
    std::vector<std::string> parseIdentifierList() {
        std::vector<std::string> names;
        names.push_back(expect(TokenKind::Identifier, "expected identifier").lexeme);
        while (match(TokenKind::Comma)) {
            names.push_back(expect(TokenKind::Identifier, "expected identifier after ','").lexeme);
        }
        return names;
    }

    // 解析类型：basic type 或 array[bound,...] of basic type。
    // 示例：array[1..10, -2..2] of integer
    std::unique_ptr<TypeNode> parseType() {
        if (isBasicType(current().kind)) {
            return parseScalarType();
        }

        if (match(TokenKind::Array)) {
            auto type = std::make_unique<ArrayTypeNode>();
            type->loc = previous().location;
            expect(TokenKind::LBracket, "expected '[' after 'array'");
            type->dims = parseArrayBounds();
            expect(TokenKind::RBracket, "expected ']' after array bounds");
            expect(TokenKind::Of, "expected 'of' after array bounds");
            type->elementType = parseScalarTypeNode();
            return type;
        }

        throw CompilerError("parser", "expected type", current().location);
    }

    // 解析数组维度列表。
    // 示例：1..10, -2..2 -> 两个 ArrayBound。
    std::vector<ArrayBound> parseArrayBounds() {
        std::vector<ArrayBound> bounds;
        bounds.push_back(parseArrayBound());
        while (match(TokenKind::Comma)) {
            bounds.push_back(parseArrayBound());
        }
        return bounds;
    }

    // 解析单个数组边界 lower..upper。
    // 示例：-3..7 -> {lower=-3, upper=7}
    ArrayBound parseArrayBound() {
        ArrayBound bound;
        bound.lower = parseSignedIntegerLiteral();
        expect(TokenKind::Range, "expected '..' in array bound");
        bound.upper = parseSignedIntegerLiteral();
        return bound;
    }

    // 解析可选符号的整数字面量。
    // 示例：+12 -> 12，-5 -> -5，8 -> 8
    int parseSignedIntegerLiteral() {
        int sign = 1;
        if (match(TokenKind::Plus)) {
            sign = 1;
        } else if (match(TokenKind::Minus)) {
            sign = -1;
        }

        Token number = expect(TokenKind::IntegerLiteral, "expected integer literal");
        return sign * std::stoi(number.lexeme);
    }

    // 解析标量类型包装接口。
    // 示例：integer / real / boolean / char
    std::unique_ptr<TypeNode> parseScalarType() {
        return parseScalarTypeNode();
    }

    // 解析标量类型并映射为 BasicTypeKind。
    // 示例：token 为 TokenKind::Real 时，返回 BasicTypeKind::Real。
    std::unique_ptr<ScalarTypeNode> parseScalarTypeNode() {
        auto type = std::make_unique<ScalarTypeNode>();
        type->loc = current().location;

        switch (current().kind) {
        case TokenKind::Integer:
            type->kind = BasicTypeKind::Integer;
            break;
        case TokenKind::Real:
            type->kind = BasicTypeKind::Real;
            break;
        case TokenKind::Boolean:
            type->kind = BasicTypeKind::Boolean;
            break;
        case TokenKind::Char:
            type->kind = BasicTypeKind::Char;
            break;
        default:
            throw CompilerError("parser", "expected basic type", current().location);
        }

        ++index_;
        return type;
    }

    // 解析子程序声明（function 或 procedure）。
    // 示例：
    //   function add(a, b: integer): integer; begin ... end
    //   procedure swap(var x, y: integer); begin ... end
    std::unique_ptr<SubprogramDeclNode> parseSubprogram() {
        if (match(TokenKind::Function)) {
            const Token name = expect(TokenKind::Identifier, "expected function name");
            auto function = std::make_unique<FunctionDeclNode>();
            function->loc = previous().location;
            function->name = name.lexeme;
            function->params = parseFormalParameters();
            expect(TokenKind::Colon, "expected ':' before function return type");
            function->returnType = parseBasicTypeKind();
            expect(TokenKind::Semicolon, "expected ';' after function header");
            function->block = parseBlock();
            return function;
        }

        expect(TokenKind::Procedure, "expected 'procedure'");
        const Token name = expect(TokenKind::Identifier, "expected procedure name");
        auto procedure = std::make_unique<ProcedureDeclNode>();
        procedure->loc = previous().location;
        procedure->name = name.lexeme;
        procedure->params = parseFormalParameters();
        expect(TokenKind::Semicolon, "expected ';' after procedure header");
        procedure->block = parseBlock();
        return procedure;
    }

    // 解析基础类型关键字并返回枚举。
    // 示例：integer -> BasicTypeKind::Integer
    BasicTypeKind parseBasicTypeKind() {
        switch (current().kind) {
        case TokenKind::Integer:
            ++index_;
            return BasicTypeKind::Integer;
        case TokenKind::Real:
            ++index_;
            return BasicTypeKind::Real;
        case TokenKind::Boolean:
            ++index_;
            return BasicTypeKind::Boolean;
        case TokenKind::Char:
            ++index_;
            return BasicTypeKind::Char;
        default:
            throw CompilerError("parser", "expected basic type", current().location);
        }
    }

    // 解析形参列表，允许为空。
    // 示例：
    //   ()
    //   (a, b: integer; var x: real)
    std::vector<std::unique_ptr<ParamDeclNode>> parseFormalParameters() {
        std::vector<std::unique_ptr<ParamDeclNode>> params;
        if (!match(TokenKind::LParen)) {
            return params;
        }

        if (!check(TokenKind::RParen)) {
            params.push_back(parseParameterGroup());
            while (match(TokenKind::Semicolon)) {
                params.push_back(parseParameterGroup());
            }
        }

        expect(TokenKind::RParen, "expected ')' after parameter list");
        return params;
    }

    // 解析一个参数组：可选 var + idlist + : + basic type。
    // 示例：var x, y: integer
    std::unique_ptr<ParamDeclNode> parseParameterGroup() {
        auto param = std::make_unique<ParamDeclNode>();
        param->loc = current().location;
        if (match(TokenKind::Var)) {
            param->passMode = ParamPassMode::Var;
        }
        param->names = parseIdentifierList();
        expect(TokenKind::Colon, "expected ':' in parameter declaration");
        param->type = parseBasicTypeKind();
        return param;
    }

    // 解析复合语句 begin ... end。
    // 示例：begin a := 1; write(a); end
    std::unique_ptr<CompoundStmtNode> parseCompoundStatement() {
        const Token begin = expect(TokenKind::Begin, "expected 'begin'");
        auto stmt = std::make_unique<CompoundStmtNode>();
        stmt->loc = begin.location;

        while (!check(TokenKind::End)) {
            if (match(TokenKind::Semicolon)) {
                continue;
            }
            stmt->statements.push_back(parseStatement());
            if (!check(TokenKind::End)) {
                expect(TokenKind::Semicolon, "expected ';' between statements");
            }
        }

        expect(TokenKind::End, "expected 'end'");
        return stmt;
    }

    // 解析单条语句分发入口。
    // 示例：
    //   if a = 1 then write(a)
    //   while i < n do i := i + 1
    //   x := 3
    std::unique_ptr<Stmt> parseStatement() {
        switch (current().kind) {
        case TokenKind::Begin:
            return parseCompoundStatement();
        case TokenKind::If:
            return parseIfStatement();
        case TokenKind::While:
            return parseWhileStatement();
        case TokenKind::For:
            return parseForStatement();
        case TokenKind::Break:
            return parseBreakStatement();
        case TokenKind::Read:
            return parseReadStatement();
        case TokenKind::Write:
            return parseWriteStatement();
        case TokenKind::Identifier:
            return parseIdentifierLedStatement();
        case TokenKind::Semicolon:
        case TokenKind::End:
        case TokenKind::Else:
        case TokenKind::Until: {
            auto empty = std::make_unique<CompoundStmtNode>();
            empty->loc = current().location;
            return empty;
        }
        default:
            throw CompilerError("parser", "unexpected token in statement", current().location);
        }
    }
    // 解析标识符开头语句并进行消歧。
    // 示例：
    //   foo(1, 2)    -> CallStmtNode
    //   a := 3       -> AssignStmtNode(VarExpr)
    //   arr[i] := x  -> AssignStmtNode(IndexExpr)
    //   foo          -> 无参 CallStmtNode
    std::unique_ptr<Stmt> parseIdentifierLedStatement() {
        const Token name = expect(TokenKind::Identifier, "expected identifier");

        // Pascal 中“标识符开头语句”存在二义性。
        // 这里用前瞻按顺序消歧：调用 ->（带下标/不带下标）赋值 -> 无参过程调用。
        if (match(TokenKind::LParen)) {
            auto stmt = std::make_unique<CallStmtNode>();
            stmt->loc = name.location;
            stmt->name = name.lexeme;
            if (!check(TokenKind::RParen)) {
                stmt->args = parseExpressionList();
            }
            expect(TokenKind::RParen, "expected ')' after argument list");
            return stmt;
        }

        std::vector<std::unique_ptr<Expr>> indices;
        if (match(TokenKind::LBracket)) {
            indices = parseExpressionList();
            expect(TokenKind::RBracket, "expected ']' after index list");
        }

        if (match(TokenKind::Assign)) {
            auto stmt = std::make_unique<AssignStmtNode>();
            stmt->loc = name.location;
            if (indices.empty()) {
                auto target = std::make_unique<VarExprNode>();
                target->loc = name.location;
                target->name = name.lexeme;
                stmt->target = std::move(target);
            } else {
                auto target = std::make_unique<IndexExprNode>();
                target->loc = name.location;
                target->baseName = name.lexeme;
                target->indices = std::move(indices);
                stmt->target = std::move(target);
            }
            stmt->value = parseExpression();
            return stmt;
        }

        if (!indices.empty()) {
            throw CompilerError("parser", "indexed expression cannot be used as a statement here", name.location);
        }

        auto stmt = std::make_unique<CallStmtNode>();
        stmt->loc = name.location;
        stmt->name = name.lexeme;
        return stmt;
    }

    // 解析 if 语句（可带 else）。
    // 示例：if a = 1 then b := 2 else b := 3
    std::unique_ptr<Stmt> parseIfStatement() {
        const Token keyword = expect(TokenKind::If, "expected 'if'");
        auto stmt = std::make_unique<IfStmtNode>();
        stmt->loc = keyword.location;
        stmt->condition = parseExpression();
        expect(TokenKind::Then, "expected 'then'");
        stmt->thenBranch = parseStatement();
        if (match(TokenKind::Else)) {
            stmt->elseBranch = parseStatement();
        }
        return stmt;
    }

    // 解析 while 语句。
    // 示例：while i < 10 do i := i + 1
    std::unique_ptr<Stmt> parseWhileStatement() {
        const Token keyword = expect(TokenKind::While, "expected 'while'");
        auto stmt = std::make_unique<WhileStmtNode>();
        stmt->loc = keyword.location;
        stmt->condition = parseExpression();
        expect(TokenKind::Do, "expected 'do'");
        stmt->body = parseStatement();
        return stmt;
    }

    // 解析 break 语句。
    // 示例：if x = 0 then break
    std::unique_ptr<Stmt> parseBreakStatement() {
        const Token keyword = expect(TokenKind::Break, "expected 'break'");
        auto stmt = std::make_unique<BreakStmtNode>();
        stmt->loc = keyword.location;
        return stmt;
    }

    // 解析 for-to 循环（当前不支持 downto）。
    // 示例：for i := 1 to n do write(i)
    std::unique_ptr<Stmt> parseForStatement() {
        const Token keyword = expect(TokenKind::For, "expected 'for'");
        auto stmt = std::make_unique<ForStmtNode>();
        stmt->loc = keyword.location;
        stmt->varName = expect(TokenKind::Identifier, "expected loop variable").lexeme;
        expect(TokenKind::Assign, "expected ':=' in for statement");
        stmt->start = parseExpression();
        if (!match(TokenKind::To)) {
            if (match(TokenKind::Downto)) {
                throw CompilerError("parser", "downto is not implemented yet", previous().location);
            }
            throw CompilerError("parser", "expected 'to' in for statement", current().location);
        }
        stmt->stop = parseExpression();
        expect(TokenKind::Do, "expected 'do' in for statement");
        stmt->body = parseStatement();
        return stmt;
    }

    // 解析 read 语句，参数必须是可赋值目标。
    // 示例：read(a, arr[i])
    std::unique_ptr<Stmt> parseReadStatement() {
        const Token keyword = expect(TokenKind::Read, "expected 'read'");
        auto stmt = std::make_unique<ReadStmtNode>();
        stmt->loc = keyword.location;
        expect(TokenKind::LParen, "expected '(' after read");
        if (!check(TokenKind::RParen)) {
            stmt->targets.push_back(parseLValue());
            while (match(TokenKind::Comma)) {
                stmt->targets.push_back(parseLValue());
            }
        }
        expect(TokenKind::RParen, "expected ')' after read arguments");
        return stmt;
    }

    // 解析 write 语句，参数为表达式列表。
    // 示例：write(a, a + 1, 'x')
    std::unique_ptr<Stmt> parseWriteStatement() {
        const Token keyword = expect(TokenKind::Write, "expected 'write'");
        auto stmt = std::make_unique<WriteStmtNode>();
        stmt->loc = keyword.location;
        expect(TokenKind::LParen, "expected '(' after write");
        if (!check(TokenKind::RParen)) {
            stmt->values = parseExpressionList();
        }
        expect(TokenKind::RParen, "expected ')' after write arguments");
        return stmt;
    }

    // 解析逗号分隔表达式列表。
    // 示例：a, b + 1, func(x)
    std::vector<std::unique_ptr<Expr>> parseExpressionList() {
        std::vector<std::unique_ptr<Expr>> values;
        values.push_back(parseExpression());
        while (match(TokenKind::Comma)) {
            values.push_back(parseExpression());
        }
        return values;
    }

    // 解析左值：变量或数组下标访问。
    // 示例：a / arr[i, j]
    std::unique_ptr<Expr> parseLValue() {
        const Token name = expect(TokenKind::Identifier, "expected identifier");
        if (match(TokenKind::LBracket)) {
            auto expr = std::make_unique<IndexExprNode>();
            expr->loc = name.location;
            expr->baseName = name.lexeme;
            expr->indices = parseExpressionList();
            expect(TokenKind::RBracket, "expected ']' after index list");
            return expr;
        }

        auto expr = std::make_unique<VarExprNode>();
        expr->loc = name.location;
        expr->name = name.lexeme;
        return expr;
    }

    // 解析表达式顶层：additive [relop additive]。
    // 示例：a + b * 2 < c
    std::unique_ptr<Expr> parseExpression() {
        // 关系运算符优先级低于加法层与乘法层。
        auto left = parseAdditiveExpression();

        if (isRelationalOperator(current().kind)) {
            const Token op = current();
            ++index_;
            auto expr = std::make_unique<BinaryExprNode>();
            expr->loc = op.location;
            expr->op = toBinaryOp(op.kind);
            expr->lhs = std::move(left);
            expr->rhs = parseAdditiveExpression();
            return expr;
        }

        return left;
    }

    // 解析加法层（左结合）：(+ | - | or)。
    // 示例：a - b + c
    std::unique_ptr<Expr> parseAdditiveExpression() {
        // +、-、or 的左结合链。
        auto left = parseMultiplicativeExpression();

        while (check(TokenKind::Plus) || check(TokenKind::Minus) || check(TokenKind::Or)) {
            const Token op = current();
            ++index_;
            auto expr = std::make_unique<BinaryExprNode>();
            expr->loc = op.location;
            expr->op = toBinaryOp(op.kind);
            expr->lhs = std::move(left);
            expr->rhs = parseMultiplicativeExpression();
            left = std::move(expr);
        }

        return left;
    }

    // 解析乘法层（左结合）：(* | / | div | mod | and)。
    // 示例：a * b div 2 mod 3
    std::unique_ptr<Expr> parseMultiplicativeExpression() {
        // *、/、div、mod、and 的左结合链。
        auto left = parseUnaryExpression();

        while (check(TokenKind::Star) || check(TokenKind::Slash) || check(TokenKind::Div) ||
               check(TokenKind::Mod) || check(TokenKind::And)) {
            const Token op = current();
            ++index_;
            auto expr = std::make_unique<BinaryExprNode>();
            expr->loc = op.location;
            expr->op = toBinaryOp(op.kind);
            expr->lhs = std::move(left);
            expr->rhs = parseUnaryExpression();
            left = std::move(expr);
        }

        return left;
    }

    // 解析一元表达式：+x / -x / not x。
    // 示例：-a, not flag
    std::unique_ptr<Expr> parseUnaryExpression() {
        if (match(TokenKind::Plus)) {
            auto expr = std::make_unique<UnaryExprNode>();
            expr->loc = previous().location;
            expr->op = UnaryOp::Plus;
            expr->operand = parseUnaryExpression();
            return expr;
        }
        if (match(TokenKind::Minus)) {
            auto expr = std::make_unique<UnaryExprNode>();
            expr->loc = previous().location;
            expr->op = UnaryOp::Minus;
            expr->operand = parseUnaryExpression();
            return expr;
        }
        if (match(TokenKind::Not)) {
            auto expr = std::make_unique<UnaryExprNode>();
            expr->loc = previous().location;
            expr->op = UnaryOp::Not;
            expr->operand = parseUnaryExpression();
            return expr;
        }
        return parsePrimaryExpression();
    }

    // 解析原子表达式：字面量、标识符表达式、括号表达式。
    // 示例：(a + b), 123, foo(x), arr[i]
    std::unique_ptr<Expr> parsePrimaryExpression() {
        switch (current().kind) {
        case TokenKind::IntegerLiteral:
        case TokenKind::RealLiteral:
        case TokenKind::CharLiteral:
        case TokenKind::StringLiteral:
        case TokenKind::True:
        case TokenKind::False:
            return parseLiteral();
        case TokenKind::Identifier:
            return parseIdentifierExpression();
        case TokenKind::LParen: {
            expect(TokenKind::LParen, "expected '('");
            auto expr = parseExpression();
            expect(TokenKind::RParen, "expected ')' after expression");
            return expr;
        }
        default:
            throw CompilerError("parser", "unexpected token in expression", current().location);
        }
    }

    // 解析字面量并确定 LiteralKind。
    // 示例：42、3.14、'c'、'hello'、true
    std::unique_ptr<Expr> parseLiteral() {
        Token token = current();
        ++index_;

        auto expr = std::make_unique<LiteralExprNode>();
        expr->loc = token.location;
        expr->rawText = token.lexeme;

        switch (token.kind) {
        case TokenKind::IntegerLiteral:
            expr->kind = LiteralKind::Int;
            break;
        case TokenKind::RealLiteral:
            expr->kind = LiteralKind::Real;
            break;
        case TokenKind::CharLiteral:
            expr->kind = LiteralKind::Char;
            break;
        case TokenKind::StringLiteral:
            expr->kind = LiteralKind::String;
            break;
        case TokenKind::True:
        case TokenKind::False:
            expr->kind = LiteralKind::Bool;
            break;
        default:
            throw CompilerError("parser", "expected literal", token.location);
        }

        return expr;
    }

    // 解析标识符表达式：变量、函数调用、数组索引。
    // 示例：x / f(a, b) / arr[i]
    std::unique_ptr<Expr> parseIdentifierExpression() {
        const Token name = expect(TokenKind::Identifier, "expected identifier");

        if (match(TokenKind::LParen)) {
            auto expr = std::make_unique<CallExprNode>();
            expr->loc = name.location;
            expr->name = name.lexeme;
            if (!check(TokenKind::RParen)) {
                expr->args = parseExpressionList();
            }
            expect(TokenKind::RParen, "expected ')' after argument list");
            return expr;
        }

        if (match(TokenKind::LBracket)) {
            auto expr = std::make_unique<IndexExprNode>();
            expr->loc = name.location;
            expr->baseName = name.lexeme;
            expr->indices = parseExpressionList();
            expect(TokenKind::RBracket, "expected ']' after index list");
            return expr;
        }

        auto expr = std::make_unique<VarExprNode>();
        expr->loc = name.location;
        expr->name = name.lexeme;
        return expr;
    }

    // 判断 token 是否是基础类型关键字。
    // 示例：TokenKind::Integer -> true，TokenKind::Array -> false
    static bool isBasicType(TokenKind kind) {
        return kind == TokenKind::Integer || kind == TokenKind::Real ||
               kind == TokenKind::Boolean || kind == TokenKind::Char;
    }

    // 判断 token 是否是关系运算符。
    // 示例：<、<=、=、<> 均返回 true。
    static bool isRelationalOperator(TokenKind kind) {
        return kind == TokenKind::Equal || kind == TokenKind::NotEqual ||
               kind == TokenKind::Less || kind == TokenKind::LessEqual ||
               kind == TokenKind::Greater || kind == TokenKind::GreaterEqual;
    }

    // 将词法运算符 token 映射到 AST 的 BinaryOp。
    // 示例：TokenKind::Slash -> BinaryOp::RealDiv。
    static BinaryOp toBinaryOp(TokenKind kind) {
        switch (kind) {
        case TokenKind::Plus:
            return BinaryOp::Add;
        case TokenKind::Minus:
            return BinaryOp::Sub;
        case TokenKind::Star:
            return BinaryOp::Mul;
        case TokenKind::Slash:
            return BinaryOp::RealDiv;
        case TokenKind::Div:
            return BinaryOp::IntDiv;
        case TokenKind::Mod:
            return BinaryOp::Mod;
        case TokenKind::Equal:
            return BinaryOp::Eq;
        case TokenKind::NotEqual:
            return BinaryOp::Ne;
        case TokenKind::Less:
            return BinaryOp::Lt;
        case TokenKind::LessEqual:
            return BinaryOp::Le;
        case TokenKind::Greater:
            return BinaryOp::Gt;
        case TokenKind::GreaterEqual:
            return BinaryOp::Ge;
        case TokenKind::And:
            return BinaryOp::And;
        case TokenKind::Or:
            return BinaryOp::Or;
        default:
            throw std::runtime_error("invalid binary operator token");
        }
    }

    const TokenList& tokens_;
    std::size_t index_ = 0;
};

}  // namespace

ProgramPtr Parser::parse(const TokenList& tokens) const {
    // 对外统一入口：构造实现体并解析完整 Program。
    // 示例：Parser().parse(tokens) -> ProgramPtr
    ParserImpl parser(tokens);
    return parser.parseProgram();
}

}  // namespace pascal_s2c



