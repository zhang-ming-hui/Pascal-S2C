#include "semantic/analyzer.h"

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include "common/error.h"

namespace pascal_s2c {

namespace {

class AnalyzerImpl {
public:
    explicit AnalyzerImpl(SemanticContext& context) : context_(context) {}

    void analyzeProgram(const ProgramNode& program) {
        context_.entryProgramName = program.name;
        Scope& global = createScope(nullptr);
        context_.globalScope = &global;
        analyzeBlock(*program.block, global, true);
    }

private:
    Scope& createScope(const Scope* parent) {
        context_.ownedScopes.push_back(std::make_unique<Scope>(parent));
        return *context_.ownedScopes.back();
    }

    void analyzeBlock(const BlockNode& block, Scope& scope, bool isGlobalScope) {
        declareConsts(block, scope, isGlobalScope);
        declareVars(block, scope, isGlobalScope);
        declareSubprogramHeaders(block, scope, isGlobalScope);
        analyzeSubprogramBodies(block, scope);
        if (block.body) {
            analyzeStatement(*block.body, scope);
        }
    }

    void declareConsts(const BlockNode& block, Scope& scope, bool isGlobalScope) {
        for (const auto& decl : block.constDecls) {
            TypeInfo type = analyzeExpression(*decl->value, scope);
            Symbol symbol;
            symbol.name = decl->name;
            symbol.kind = SymbolKind::Constant;
            symbol.type = type;
            symbol.isGlobal = isGlobalScope;
            defineSymbol(scope, std::move(symbol), decl->loc);
        }
    }

    void declareVars(const BlockNode& block, Scope& scope, bool isGlobalScope) {
        for (const auto& decl : block.varDecls) {
            TypeInfo type = resolveType(*decl->type);
            for (const std::string& name : decl->names) {
                Symbol symbol;
                symbol.name = name;
                symbol.kind = SymbolKind::Variable;
                symbol.type = type;
                symbol.isGlobal = isGlobalScope;
                defineSymbol(scope, std::move(symbol), decl->loc);
            }
        }
    }

    void declareSubprogramHeaders(const BlockNode& block, Scope& scope, bool isGlobalScope) {
        for (const auto& subprogram : block.subprograms) {
            Symbol symbol;
            symbol.name = subprogram->name;
            symbol.kind = dynamic_cast<const FunctionDeclNode*>(subprogram.get()) != nullptr
                ? SymbolKind::Function
                : SymbolKind::Procedure;
            symbol.isGlobal = isGlobalScope;
            symbol.type = makeScalarType(BasicTypeKind::Void);

            if (const auto* function = dynamic_cast<const FunctionDeclNode*>(subprogram.get())) {
                symbol.type = makeScalarType(function->returnType);
            }

            for (const auto& param : subprogram->params) {
                TypeInfo paramType = makeScalarType(param->type);
                for (std::size_t i = 0; i < param->names.size(); ++i) {
                    SymbolParameter parameter;
                    parameter.type = paramType;
                    parameter.isVar = param->passMode == ParamPassMode::Var;
                    symbol.parameters.push_back(parameter);
                }
            }

            defineSymbol(scope, std::move(symbol), subprogram->loc);
        }
    }

    void analyzeSubprogramBodies(const BlockNode& block, Scope& parentScope) {
        for (const auto& subprogram : block.subprograms) {
            Scope& childScope = createScope(&parentScope);
            declareParameters(*subprogram, childScope);
            analyzeBlock(*subprogram->block, childScope, false);
        }
    }

    void declareParameters(const SubprogramDeclNode& subprogram, Scope& scope) {
        for (const auto& param : subprogram.params) {
            TypeInfo type = makeScalarType(param->type);
            for (const std::string& name : param->names) {
                Symbol symbol;
                symbol.name = name;
                symbol.kind = SymbolKind::Parameter;
                symbol.type = type;
                symbol.isVarParameter = param->passMode == ParamPassMode::Var;
                defineSymbol(scope, std::move(symbol), param->loc);
            }
        }
    }

    void analyzeStatement(const Stmt& stmt, Scope& scope) {
        if (const auto* compound = dynamic_cast<const CompoundStmtNode*>(&stmt)) {
            for (const auto& child : compound->statements) {
                analyzeStatement(*child, scope);
            }
            return;
        }

        if (const auto* assign = dynamic_cast<const AssignStmtNode*>(&stmt)) {
            TypeInfo targetType = analyzeLValue(*assign->target, scope);
            TypeInfo valueType = analyzeExpression(*assign->value, scope);
            if (!isAssignmentCompatible(targetType, valueType)) {
                throw CompilerError("semantic", "incompatible assignment: cannot assign " + toString(valueType) + " to " + toString(targetType), assign->loc);
            }
            return;
        }

        if (const auto* callStmt = dynamic_cast<const CallStmtNode*>(&stmt)) {
            const Symbol& symbol = resolveCallable(callStmt->name, scope, callStmt->loc);
            context_.callStmtBindings.emplace(callStmt, &symbol);
            analyzeCallArguments(symbol, callStmt->args, scope, callStmt->loc);
            return;
        }

        if (const auto* ifStmt = dynamic_cast<const IfStmtNode*>(&stmt)) {
            analyzeExpression(*ifStmt->condition, scope);
            analyzeStatement(*ifStmt->thenBranch, scope);
            if (ifStmt->elseBranch) {
                analyzeStatement(*ifStmt->elseBranch, scope);
            }
            return;
        }

        if (const auto* whileStmt = dynamic_cast<const WhileStmtNode*>(&stmt)) {
            analyzeExpression(*whileStmt->condition, scope);
            analyzeStatement(*whileStmt->body, scope);
            return;
        }

        if (const auto* forStmt = dynamic_cast<const ForStmtNode*>(&stmt)) {
            const Symbol& symbol = resolveSymbol(forStmt->varName, scope, forStmt->loc);
            if (symbol.type.basic != BasicTypeKind::Integer || symbol.type.isArray) {
                throw CompilerError("semantic", "for loop variable must be integer", forStmt->loc);
            }
            analyzeExpression(*forStmt->start, scope);
            analyzeExpression(*forStmt->stop, scope);
            analyzeStatement(*forStmt->body, scope);
            return;
        }

        if (const auto* readStmt = dynamic_cast<const ReadStmtNode*>(&stmt)) {
            for (const auto& target : readStmt->targets) {
                analyzeLValue(*target, scope);
            }
            return;
        }

        if (const auto* writeStmt = dynamic_cast<const WriteStmtNode*>(&stmt)) {
            for (const auto& value : writeStmt->values) {
                analyzeExpression(*value, scope);
            }
            return;
        }
    }

    TypeInfo analyzeLValue(const Expr& expr, Scope& scope) {
        if (const auto* var = dynamic_cast<const VarExprNode*>(&expr)) {
            const Symbol& symbol = resolveSymbol(var->name, scope, var->loc);
            if (!isAssignableSymbol(symbol)) {
                throw CompilerError("semantic", "symbol is not assignable: " + var->name, var->loc);
            }
            context_.variableBindings.emplace(var, &symbol);
            context_.expressionTypes.emplace(var, symbol.type);
            return symbol.type;
        }

        if (const auto* index = dynamic_cast<const IndexExprNode*>(&expr)) {
            const Symbol& symbol = resolveSymbol(index->baseName, scope, index->loc);
            if (!symbol.type.isArray) {
                throw CompilerError("semantic", "indexed symbol is not an array: " + index->baseName, index->loc);
            }
            if (index->indices.size() != symbol.type.dims.size()) {
                throw CompilerError("semantic", "array dimension mismatch for: " + index->baseName, index->loc);
            }
            for (const auto& item : index->indices) {
                analyzeExpression(*item, scope);
            }
            TypeInfo result = arrayElementType(symbol.type);
            context_.indexBindings.emplace(index, &symbol);
            context_.expressionTypes.emplace(index, result);
            return result;
        }

        throw CompilerError("semantic", "expected assignable expression", expr.loc);
    }

    TypeInfo analyzeExpression(const Expr& expr, Scope& scope) {
        if (const auto* literal = dynamic_cast<const LiteralExprNode*>(&expr)) {
            TypeInfo type = analyzeLiteral(*literal);
            context_.expressionTypes[&expr] = type;
            return type;
        }

        if (const auto* var = dynamic_cast<const VarExprNode*>(&expr)) {
            const Symbol& symbol = resolveSymbol(var->name, scope, var->loc);
            if (symbol.kind == SymbolKind::Procedure) {
                throw CompilerError("semantic", "procedure cannot be used as an expression: " + var->name, var->loc);
            }
            context_.variableBindings.emplace(var, &symbol);
            context_.expressionTypes.emplace(var, symbol.type);
            return symbol.type;
        }

        if (const auto* index = dynamic_cast<const IndexExprNode*>(&expr)) {
            const Symbol& symbol = resolveSymbol(index->baseName, scope, index->loc);
            if (!symbol.type.isArray) {
                throw CompilerError("semantic", "indexed symbol is not an array: " + index->baseName, index->loc);
            }
            if (index->indices.size() != symbol.type.dims.size()) {
                throw CompilerError("semantic", "array dimension mismatch for: " + index->baseName, index->loc);
            }
            for (const auto& item : index->indices) {
                analyzeExpression(*item, scope);
            }
            TypeInfo result = arrayElementType(symbol.type);
            context_.indexBindings.emplace(index, &symbol);
            context_.expressionTypes.emplace(index, result);
            return result;
        }

        if (const auto* call = dynamic_cast<const CallExprNode*>(&expr)) {
            const Symbol& symbol = resolveCallable(call->name, scope, call->loc);
            if (symbol.kind == SymbolKind::Procedure) {
                throw CompilerError("semantic", "procedure cannot be used as an expression: " + call->name, call->loc);
            }
            analyzeCallArguments(symbol, call->args, scope, call->loc);
            context_.callExprBindings.emplace(call, &symbol);
            context_.expressionTypes.emplace(call, symbol.type);
            return symbol.type;
        }

        if (const auto* unary = dynamic_cast<const UnaryExprNode*>(&expr)) {
            TypeInfo operandType = analyzeExpression(*unary->operand, scope);
            TypeInfo result = analyzeUnaryType(unary->op, operandType, unary->loc);
            context_.expressionTypes.emplace(unary, result);
            return result;
        }

        if (const auto* binary = dynamic_cast<const BinaryExprNode*>(&expr)) {
            TypeInfo lhs = analyzeExpression(*binary->lhs, scope);
            TypeInfo rhs = analyzeExpression(*binary->rhs, scope);
            TypeInfo result = analyzeBinaryType(binary->op, lhs, rhs, binary->loc);
            context_.expressionTypes.emplace(binary, result);
            return result;
        }

        throw CompilerError("semantic", "unsupported expression node", expr.loc);
    }

    TypeInfo analyzeLiteral(const LiteralExprNode& literal) const {
        switch (literal.kind) {
        case LiteralKind::Int:
            return makeScalarType(BasicTypeKind::Integer);
        case LiteralKind::Real:
            return makeScalarType(BasicTypeKind::Real);
        case LiteralKind::Bool:
            return makeScalarType(BasicTypeKind::Boolean);
        case LiteralKind::Char:
            return makeScalarType(BasicTypeKind::Char);
        default:
            return makeScalarType(BasicTypeKind::Void);
        }
    }

    TypeInfo analyzeUnaryType(UnaryOp op, const TypeInfo& operand, const SourceLocation& loc) const {
        switch (op) {
        case UnaryOp::Plus:
        case UnaryOp::Minus:
            if (!isNumericType(operand)) {
                throw CompilerError("semantic", "unary +/- requires numeric operand", loc);
            }
            return operand;
        case UnaryOp::Not:
            if (isBooleanType(operand)) {
                return operand;
            }
            if (operand.basic == BasicTypeKind::Integer && !operand.isArray) {
                return operand;
            }
            throw CompilerError("semantic", "not requires boolean or integer operand", loc);
        default:
            throw CompilerError("semantic", "unsupported unary operator", loc);
        }
    }

    TypeInfo analyzeBinaryType(BinaryOp op, const TypeInfo& lhs, const TypeInfo& rhs, const SourceLocation& loc) const {
        switch (op) {
        case BinaryOp::Add:
        case BinaryOp::Sub:
        case BinaryOp::Mul:
            if (!isNumericType(lhs) || !isNumericType(rhs)) {
                throw CompilerError("semantic", "arithmetic operator requires numeric operands", loc);
            }
            if (lhs.basic == BasicTypeKind::Real || rhs.basic == BasicTypeKind::Real) {
                return makeScalarType(BasicTypeKind::Real);
            }
            return makeScalarType(BasicTypeKind::Integer);
        case BinaryOp::RealDiv:
            if (!isNumericType(lhs) || !isNumericType(rhs)) {
                throw CompilerError("semantic", "'/' requires numeric operands", loc);
            }
            return makeScalarType(BasicTypeKind::Real);
        case BinaryOp::IntDiv:
        case BinaryOp::Mod:
            if (lhs.basic != BasicTypeKind::Integer || rhs.basic != BasicTypeKind::Integer || lhs.isArray || rhs.isArray) {
                throw CompilerError("semantic", "div/mod require integer operands", loc);
            }
            return makeScalarType(BasicTypeKind::Integer);
        case BinaryOp::Eq:
        case BinaryOp::Ne:
        case BinaryOp::Lt:
        case BinaryOp::Le:
        case BinaryOp::Gt:
        case BinaryOp::Ge:
            return makeScalarType(BasicTypeKind::Boolean);
        case BinaryOp::And:
        case BinaryOp::Or:
            if (isBooleanType(lhs) && isBooleanType(rhs)) {
                return makeScalarType(BasicTypeKind::Boolean);
            }
            if (lhs.basic == BasicTypeKind::Integer && rhs.basic == BasicTypeKind::Integer && !lhs.isArray && !rhs.isArray) {
                return makeScalarType(BasicTypeKind::Integer);
            }
            throw CompilerError("semantic", "and/or require both operands to be boolean or integer", loc);
        default:
            throw CompilerError("semantic", "unsupported binary operator", loc);
        }
    }

    void analyzeCallArguments(const Symbol& symbol, const std::vector<std::unique_ptr<Expr>>& args, Scope& scope, const SourceLocation& loc) {
        if (args.size() != symbol.parameters.size()) {
            throw CompilerError("semantic", "argument count mismatch in call to: " + symbol.name, loc);
        }

        for (std::size_t i = 0; i < args.size(); ++i) {
            const SymbolParameter& parameter = symbol.parameters[i];
            const Expr& arg = *args[i];
            TypeInfo argType = parameter.isVar ? analyzeLValue(arg, scope) : analyzeExpression(arg, scope);
            if (!isAssignmentCompatible(parameter.type, argType)) {
                throw CompilerError("semantic", "argument type mismatch in call to: " + symbol.name, loc);
            }
        }
    }

    TypeInfo resolveType(const TypeNode& typeNode) const {
        if (const auto* scalar = dynamic_cast<const ScalarTypeNode*>(&typeNode)) {
            return makeScalarType(scalar->kind);
        }

        if (const auto* array = dynamic_cast<const ArrayTypeNode*>(&typeNode)) {
            return makeArrayType(array->elementType->kind, array->dims);
        }

        throw CompilerError("semantic", "unknown type node", typeNode.loc);
    }

    const Symbol& resolveSymbol(const std::string& name, Scope& scope, const SourceLocation& loc) const {
        const Symbol* symbol = scope.lookup(name);
        if (symbol == nullptr) {
            throw CompilerError("semantic", "undefined symbol: " + name, loc);
        }
        return *symbol;
    }

    const Symbol& resolveCallable(const std::string& name, Scope& scope, const SourceLocation& loc) const {
        const Symbol& symbol = resolveSymbol(name, scope, loc);
        if (!isCallableSymbol(symbol)) {
            if (symbol.kind == SymbolKind::Function && symbol.parameters.empty()) {
                return symbol;
            }
            if (symbol.kind == SymbolKind::Variable || symbol.kind == SymbolKind::Parameter || symbol.kind == SymbolKind::Constant) {
                if (!symbol.parameters.empty()) {
                    return symbol;
                }
            }
            throw CompilerError("semantic", "symbol is not callable: " + name, loc);
        }
        return symbol;
    }

    void defineSymbol(Scope& scope, Symbol symbol, const SourceLocation& loc) const {
        const std::string name = symbol.name;
        if (!scope.define(std::move(symbol))) {
            throw CompilerError("semantic", "duplicate symbol: " + name, loc);
        }
    }

    bool isAssignmentCompatible(const TypeInfo& target, const TypeInfo& value) const {
        if (sameType(target, value)) {
            return true;
        }
        if (!target.isArray && !value.isArray && target.basic == BasicTypeKind::Real && value.basic == BasicTypeKind::Integer) {
            return true;
        }
        return false;
    }

    SemanticContext& context_;
};

}  // namespace

SemanticContext SemanticAnalyzer::analyze(const ProgramNode& program) const {
    SemanticContext context;
    AnalyzerImpl analyzer(context);
    analyzer.analyzeProgram(program);
    return context;
}

}  // namespace pascal_s2c
