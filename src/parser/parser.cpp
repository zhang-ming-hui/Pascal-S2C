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

    ProgramPtr parseProgram() {
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
    const Token& current() const {
        if (index_ < tokens_.size()) {
            return tokens_[index_];
        }
        return tokens_.back();
    }

    const Token& previous() const {
        return tokens_[index_ - 1];
    }

    bool isAtEnd() const {
        return current().kind == TokenKind::EndOfFile;
    }

    bool check(TokenKind kind) const {
        return current().kind == kind;
    }

    bool match(TokenKind kind) {
        if (!check(kind)) {
            return false;
        }
        ++index_;
        return true;
    }

    Token expect(TokenKind kind, const std::string& message) {
        if (!check(kind)) {
            throw CompilerError("parser", message, current().location);
        }
        return tokens_[index_++];
    }

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

    std::unique_ptr<BlockNode> parseBlock() {
        auto block = std::make_unique<BlockNode>();
        block->loc = current().location;

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

    void parseVarDeclarations(BlockNode& block) {
        if (!match(TokenKind::Var)) {
            return;
        }

        while (check(TokenKind::Identifier)) {
            block.varDecls.push_back(parseVarDeclaration());
            expect(TokenKind::Semicolon, "expected ';' after variable declaration");
        }
    }

    std::unique_ptr<VarDeclNode> parseVarDeclaration() {
        auto decl = std::make_unique<VarDeclNode>();
        decl->loc = current().location;
        decl->names = parseIdentifierList();
        expect(TokenKind::Colon, "expected ':' after identifier list");
        decl->type = parseType();
        return decl;
    }

    std::vector<std::string> parseIdentifierList() {
        std::vector<std::string> names;
        names.push_back(expect(TokenKind::Identifier, "expected identifier").lexeme);
        while (match(TokenKind::Comma)) {
            names.push_back(expect(TokenKind::Identifier, "expected identifier after ','").lexeme);
        }
        return names;
    }

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

    std::vector<ArrayBound> parseArrayBounds() {
        std::vector<ArrayBound> bounds;
        bounds.push_back(parseArrayBound());
        while (match(TokenKind::Comma)) {
            bounds.push_back(parseArrayBound());
        }
        return bounds;
    }

    ArrayBound parseArrayBound() {
        ArrayBound bound;
        bound.lower = parseSignedIntegerLiteral();
        expect(TokenKind::Range, "expected '..' in array bound");
        bound.upper = parseSignedIntegerLiteral();
        return bound;
    }

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

    std::unique_ptr<TypeNode> parseScalarType() {
        return parseScalarTypeNode();
    }

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

    std::unique_ptr<Stmt> parseIdentifierLedStatement() {
        const Token name = expect(TokenKind::Identifier, "expected identifier");

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

    std::unique_ptr<Stmt> parseWhileStatement() {
        const Token keyword = expect(TokenKind::While, "expected 'while'");
        auto stmt = std::make_unique<WhileStmtNode>();
        stmt->loc = keyword.location;
        stmt->condition = parseExpression();
        expect(TokenKind::Do, "expected 'do'");
        stmt->body = parseStatement();
        return stmt;
    }

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

    std::vector<std::unique_ptr<Expr>> parseExpressionList() {
        std::vector<std::unique_ptr<Expr>> values;
        values.push_back(parseExpression());
        while (match(TokenKind::Comma)) {
            values.push_back(parseExpression());
        }
        return values;
    }

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

    std::unique_ptr<Expr> parseExpression() {
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

    std::unique_ptr<Expr> parseAdditiveExpression() {
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

    std::unique_ptr<Expr> parseMultiplicativeExpression() {
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

    std::unique_ptr<Expr> parsePrimaryExpression() {
        switch (current().kind) {
        case TokenKind::IntegerLiteral:
        case TokenKind::RealLiteral:
        case TokenKind::CharLiteral:
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
        case TokenKind::True:
        case TokenKind::False:
            expr->kind = LiteralKind::Bool;
            break;
        default:
            throw CompilerError("parser", "expected literal", token.location);
        }

        return expr;
    }

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

    static bool isBasicType(TokenKind kind) {
        return kind == TokenKind::Integer || kind == TokenKind::Real ||
               kind == TokenKind::Boolean || kind == TokenKind::Char;
    }

    static bool isRelationalOperator(TokenKind kind) {
        return kind == TokenKind::Equal || kind == TokenKind::NotEqual ||
               kind == TokenKind::Less || kind == TokenKind::LessEqual ||
               kind == TokenKind::Greater || kind == TokenKind::GreaterEqual;
    }

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
    ParserImpl parser(tokens);
    return parser.parseProgram();
}

}  // namespace pascal_s2c


