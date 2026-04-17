#include "codegen/c_codegen.h"

#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "codegen/c_writer.h"

namespace pascal_s2c {

namespace {

class CodegenImpl {
public:
    explicit CodegenImpl(const LoweredProgramView& program) : view_(program) {}

    std::string generate() {
        emitIncludes();
        writer_.writeLine();
        emitGlobalDeclarations();
        emitTopLevelSubprograms();
        emitMain();
        return writer_.str();
    }

private:
    struct FunctionContext {
        std::string name;
        BasicTypeKind returnType = BasicTypeKind::Void;
    };

    struct RenderedExpr {
        std::string text;
        int precedence = 0;
        const BinaryExprNode* binary = nullptr;
    };

    static constexpr int kPrecLogicalOr = 10;
    static constexpr int kPrecLogicalAnd = 20;
    static constexpr int kPrecBitwiseOr = 30;
    static constexpr int kPrecBitwiseAnd = 40;
    static constexpr int kPrecEquality = 50;
    static constexpr int kPrecRelational = 60;
    static constexpr int kPrecAdditive = 70;
    static constexpr int kPrecMultiplicative = 80;
    static constexpr int kPrecUnary = 90;
    static constexpr int kPrecPrimary = 100;

    void emitIncludes() {
        writer_.writeLine("#include <stdio.h>");
        if (programUsesBoolean(*view_.program)) {
            writer_.writeLine("#include <stdbool.h>");
        }
    }

    bool programUsesBoolean(const ProgramNode& program) const {
        return blockUsesBoolean(*program.block);
    }

    bool blockUsesBoolean(const BlockNode& block) const {
        for (const auto& decl : block.constDecls) {
            if (isBooleanType(exprType(*decl->value))) {
                return true;
            }
        }
        for (const auto& decl : block.varDecls) {
            if (typeNodeUsesBoolean(*decl->type)) {
                return true;
            }
        }
        for (const auto& sub : block.subprograms) {
            if (const auto* fn = dynamic_cast<const FunctionDeclNode*>(sub.get())) {
                if (fn->returnType == BasicTypeKind::Boolean) {
                    return true;
                }
            }
            for (const auto& param : sub->params) {
                if (param->type == BasicTypeKind::Boolean) {
                    return true;
                }
            }
            if (blockUsesBoolean(*sub->block)) {
                return true;
            }
        }
        return false;
    }

    bool typeNodeUsesBoolean(const TypeNode& type) const {
        if (const auto* scalar = dynamic_cast<const ScalarTypeNode*>(&type)) {
            return scalar->kind == BasicTypeKind::Boolean;
        }
        if (const auto* array = dynamic_cast<const ArrayTypeNode*>(&type)) {
            return array->elementType->kind == BasicTypeKind::Boolean;
        }
        return false;
    }

    void emitGlobalDeclarations() {
        if (view_.program->block->constDecls.empty() && view_.program->block->varDecls.empty()) {
            return;
        }

        for (const auto& decl : view_.program->block->constDecls) {
            emitConstDecl(*decl, nullptr);
        }
        for (const auto& decl : view_.program->block->varDecls) {
            emitVarDecl(*decl);
        }
        writer_.writeLine();
    }

    void emitTopLevelSubprograms() {
        for (const auto& subprogram : view_.program->block->subprograms) {
            emitSubprogram(*subprogram);
            writer_.writeLine();
        }
    }

    void emitMain() {
        writer_.writeLine("int main()");
        writer_.writeLine("{");
        writer_.indent();
        emitCompoundStatementsInline(*view_.program->block->body, nullptr);
        writer_.writeLine("return 0;");
        writer_.dedent();
        writer_.writeLine("}");
    }

    void emitSubprogram(const SubprogramDeclNode& subprogram) {
        if (!subprogram.block->subprograms.empty()) {
            throw std::runtime_error("nested subprogram code generation is not supported yet");
        }

        if (const auto* function = dynamic_cast<const FunctionDeclNode*>(&subprogram)) {
            FunctionContext context{function->name, function->returnType};
            writer_.writeLine(typeName(makeScalarType(function->returnType)) + " " + function->name + "(" + parameterList(*function) + ")");
            writer_.writeLine("{");
            writer_.indent();
            emitLocalDeclarations(*function->block, &context);
            emitCompoundStatementsInline(*function->block->body, &context);
            writer_.writeLine("return _;");
            writer_.dedent();
            writer_.writeLine("}");
            return;
        }

        const auto& procedure = static_cast<const ProcedureDeclNode&>(subprogram);
        FunctionContext context{procedure.name, BasicTypeKind::Void};
        writer_.writeLine("void " + procedure.name + "(" + parameterList(procedure) + ")");
        writer_.writeLine("{");
        writer_.indent();
        emitLocalDeclarations(*procedure.block, &context);
        emitCompoundStatementsInline(*procedure.block->body, &context);
        writer_.dedent();
        writer_.writeLine("}");
    }

    std::string parameterList(const SubprogramDeclNode& subprogram) const {
        std::vector<std::string> parts;
        for (const auto& param : subprogram.params) {
            for (const std::string& name : param->names) {
                TypeInfo type = makeScalarType(param->type);
                std::string piece = typeName(type);
                if (param->passMode == ParamPassMode::Var) {
                    piece += " *" + name;
                } else {
                    piece += " " + name;
                }
                parts.push_back(piece);
            }
        }
        return join(parts, ", ");
    }

    void emitLocalDeclarations(const BlockNode& block, const FunctionContext* functionContext) {
        if (functionContext != nullptr && functionContext->returnType != BasicTypeKind::Void) {
            writer_.writeLine(typeName(makeScalarType(functionContext->returnType)) + " _;");
        }

        for (const auto& decl : block.constDecls) {
            emitConstDecl(*decl, functionContext);
        }
        for (const auto& decl : block.varDecls) {
            emitVarDecl(*decl);
        }
    }

    void emitConstDecl(const ConstDeclNode& decl, const FunctionContext* context) {
        const TypeInfo& type = exprType(*decl.value);
        writer_.writeLine("const " + typeName(type) + " " + decl.name + " = " + emitExpr(*decl.value, context) + ";");
    }

    void emitVarDecl(const VarDeclNode& decl) {
        const TypeInfo type = typeOfDeclaration(*decl.type);
        std::vector<std::string> names;
        for (const std::string& name : decl.names) {
            names.push_back(name + typeSuffix(type));
        }
        writer_.writeLine(typeName(type) + " " + join(names, ", ") + ";");
    }

    TypeInfo typeOfDeclaration(const TypeNode& node) const {
        if (const auto* scalar = dynamic_cast<const ScalarTypeNode*>(&node)) {
            return makeScalarType(scalar->kind);
        }
        const auto& array = static_cast<const ArrayTypeNode&>(node);
        return makeArrayType(array.elementType->kind, array.dims);
    }

    void emitCompoundStatementsInline(const CompoundStmtNode& compound, const FunctionContext* context) {
        for (const auto& stmt : compound.statements) {
            emitStatement(*stmt, context);
        }
    }

    void emitStatement(const Stmt& stmt, const FunctionContext* context) {
        if (const auto* compound = dynamic_cast<const CompoundStmtNode*>(&stmt)) {
            writer_.writeLine("{");
            writer_.indent();
            emitCompoundStatementsInline(*compound, context);
            writer_.dedent();
            writer_.writeLine("}");
            return;
        }

        if (const auto* assign = dynamic_cast<const AssignStmtNode*>(&stmt)) {
            writer_.writeLine(emitLValue(*assign->target, context) + " = " + emitExpr(*assign->value, context) + ";");
            return;
        }

        if (dynamic_cast<const BreakStmtNode*>(&stmt) != nullptr) {
            writer_.writeLine("break;");
            return;
        }

        if (const auto* callStmt = dynamic_cast<const CallStmtNode*>(&stmt)) {
            writer_.writeLine(emitCall(callStmt->name, callStmt->args, view_.semantic->callStmtBindings.at(callStmt), context) + ";");
            return;
        }

        if (const auto* ifStmt = dynamic_cast<const IfStmtNode*>(&stmt)) {
            writer_.writeLine("if (" + emitExpr(*ifStmt->condition, context) + ")");
            emitStructuredStatement(*ifStmt->thenBranch, context);
            if (ifStmt->elseBranch) {
                writer_.writeLine("else");
                emitStructuredStatement(*ifStmt->elseBranch, context);
            }
            return;
        }

        if (const auto* whileStmt = dynamic_cast<const WhileStmtNode*>(&stmt)) {
            writer_.writeLine("while (" + emitExpr(*whileStmt->condition, context) + ")");
            emitStructuredStatement(*whileStmt->body, context);
            return;
        }

        if (const auto* forStmt = dynamic_cast<const ForStmtNode*>(&stmt)) {
            const std::string loopVar = emitNamedLValue(forStmt->varName, context);
            writer_.writeLine(
                "for (" + loopVar + " = " + emitExpr(*forStmt->start, context) + "; " +
                loopVar + " <= " + emitExpr(*forStmt->stop, context) + "; " + loopVar + "++)");
            emitStructuredStatement(*forStmt->body, context);
            return;
        }

        if (const auto* readStmt = dynamic_cast<const ReadStmtNode*>(&stmt)) {
            for (const auto& target : readStmt->targets) {
                const TypeInfo& type = exprType(*target);
                writer_.writeLine("scanf(\"" + scanfSpecifier(type) + "\", " + emitAddress(*target, context) + ");");
            }
            return;
        }

        if (const auto* writeStmt = dynamic_cast<const WriteStmtNode*>(&stmt)) {
            std::string format;
            std::vector<std::string> args;
            for (const auto& value : writeStmt->values) {
                const TypeInfo& type = exprType(*value);
                format += printfSpecifier(type);
                args.push_back(emitExpr(*value, context));
            }
            std::string line = "printf(\"" + format + "\"";
            for (const std::string& arg : args) {
                line += ", " + arg;
            }
            line += ");";
            writer_.writeLine(line);
            return;
        }
    }

    void emitStructuredStatement(const Stmt& stmt, const FunctionContext* context) {
        if (dynamic_cast<const CompoundStmtNode*>(&stmt) != nullptr) {
            emitStatement(stmt, context);
            return;
        }

        writer_.indent();
        emitStatement(stmt, context);
        writer_.dedent();
    }

    std::string emitCall(const std::string& name, const std::vector<std::unique_ptr<Expr>>& args, const Symbol* symbol, const FunctionContext* context) const {
        std::vector<std::string> emitted;
        for (std::size_t i = 0; i < args.size(); ++i) {
            if (symbol != nullptr && i < symbol->parameters.size() && symbol->parameters[i].isVar) {
                emitted.push_back(emitAddress(*args[i], context));
            } else {
                emitted.push_back(emitExpr(*args[i], context));
            }
        }
        return name + "(" + join(emitted, ", ") + ")";
    }

    std::string emitExpr(const Expr& expr, const FunctionContext* context) const {
        return renderExpr(expr, context).text;
    }

    RenderedExpr renderExpr(const Expr& expr, const FunctionContext* context) const {
        if (const auto* literal = dynamic_cast<const LiteralExprNode*>(&expr)) {
            return RenderedExpr{emitLiteralText(*literal), kPrecPrimary, nullptr};
        }

        if (const auto* var = dynamic_cast<const VarExprNode*>(&expr)) {
            return RenderedExpr{emitNamedReference(var->name, lookupBinding(var), context, false), kPrecPrimary, nullptr};
        }

        if (const auto* index = dynamic_cast<const IndexExprNode*>(&expr)) {
            return RenderedExpr{emitIndexedReference(*index, context), kPrecPrimary, nullptr};
        }

        if (const auto* call = dynamic_cast<const CallExprNode*>(&expr)) {
            return RenderedExpr{emitCall(call->name, call->args, view_.semantic->callExprBindings.at(call), context), kPrecPrimary, nullptr};
        }

        if (const auto* unary = dynamic_cast<const UnaryExprNode*>(&expr)) {
            RenderedExpr operand = renderExpr(*unary->operand, context);
            std::string operandText = maybeWrapUnaryOperand(operand);
            switch (unary->op) {
            case UnaryOp::Plus:
                return RenderedExpr{prefixUnary("+", operandText), kPrecUnary, nullptr};
            case UnaryOp::Minus:
                return RenderedExpr{prefixUnary("-", operandText), kPrecUnary, nullptr};
            case UnaryOp::Not:
                return RenderedExpr{(isBooleanType(exprType(*unary->operand)) ? "!" : "~") + operandText, kPrecUnary, nullptr};
            }
        }

        if (const auto* binary = dynamic_cast<const BinaryExprNode*>(&expr)) {
            RenderedExpr lhs = renderExpr(*binary->lhs, context);
            RenderedExpr rhs = renderExpr(*binary->rhs, context);
            std::string lhsText = maybeWrapBinaryChild(*binary, lhs, false);
            std::string rhsText = maybeWrapBinaryChild(*binary, rhs, true);
            if (binary->op == BinaryOp::RealDiv) {
                lhsText = maybeCastFloatOperand(*binary->lhs, lhs, lhsText);
                rhsText = maybeCastFloatOperand(*binary->rhs, rhs, rhsText);
            }
            return RenderedExpr{lhsText + " " + binaryOp(*binary) + " " + rhsText, precedenceOf(*binary), binary};
        }

        throw std::runtime_error("unsupported expression emission");
    }

    std::string maybeWrapUnaryOperand(const RenderedExpr& operand) const {
        if (operand.precedence < kPrecUnary) {
            return "(" + operand.text + ")";
        }
        return operand.text;
    }

    std::string prefixUnary(const std::string& op, const std::string& operandText) const {
        if (!operandText.empty() && (operandText.front() == '+' || operandText.front() == '-')) {
            return op + " " + operandText;
        }
        return op + operandText;
    }

    std::string maybeWrapBinaryChild(const BinaryExprNode& parent, const RenderedExpr& child, bool isRightChild) const {
        const int parentPrecedence = precedenceOf(parent);
        if (child.precedence < parentPrecedence) {
            return "(" + child.text + ")";
        }
        if (child.precedence > parentPrecedence || child.binary == nullptr) {
            return child.text;
        }
        if (!isRightChild) {
            return child.text;
        }
        if (needsRightParentheses(parent, *child.binary)) {
            return "(" + child.text + ")";
        }
        return child.text;
    }

    std::string maybeCastFloatOperand(const Expr& original, const RenderedExpr& rendered, const std::string& emitted) const {
        const TypeInfo& type = exprType(original);
        if (type.isArray || type.basic != BasicTypeKind::Integer) {
            return emitted;
        }
        if (rendered.precedence < kPrecUnary) {
            return "(float)(" + emitted + ")";
        }
        return "(float)" + emitted;
    }

    int precedenceOf(const BinaryExprNode& expr) const {
        switch (expr.op) {
        case BinaryOp::Mul:
        case BinaryOp::RealDiv:
        case BinaryOp::IntDiv:
        case BinaryOp::Mod:
            return kPrecMultiplicative;
        case BinaryOp::Add:
        case BinaryOp::Sub:
            return kPrecAdditive;
        case BinaryOp::Eq:
        case BinaryOp::Ne:
            return kPrecEquality;
        case BinaryOp::Lt:
        case BinaryOp::Le:
        case BinaryOp::Gt:
        case BinaryOp::Ge:
            return kPrecRelational;
        case BinaryOp::And:
            return isBooleanType(exprType(expr)) ? kPrecLogicalAnd : kPrecBitwiseAnd;
        case BinaryOp::Or:
            return isBooleanType(exprType(expr)) ? kPrecLogicalOr : kPrecBitwiseOr;
        default:
            throw std::runtime_error("unsupported binary precedence");
        }
    }

    bool needsRightParentheses(const BinaryExprNode& parent, const BinaryExprNode& child) const {
        if (precedenceOf(parent) != precedenceOf(child)) {
            return false;
        }

        switch (parent.op) {
        case BinaryOp::Add:
        case BinaryOp::And:
        case BinaryOp::Or:
            return false;
        case BinaryOp::Mul:
            return child.op != BinaryOp::Mul;
        case BinaryOp::Sub:
        case BinaryOp::RealDiv:
        case BinaryOp::IntDiv:
        case BinaryOp::Mod:
        case BinaryOp::Eq:
        case BinaryOp::Ne:
        case BinaryOp::Lt:
        case BinaryOp::Le:
        case BinaryOp::Gt:
        case BinaryOp::Ge:
            return true;
        default:
            return false;
        }
    }

    std::string emitLValue(const Expr& expr, const FunctionContext* context) const {
        if (const auto* var = dynamic_cast<const VarExprNode*>(&expr)) {
            return emitNamedReference(var->name, lookupBinding(var), context, true);
        }

        if (const auto* index = dynamic_cast<const IndexExprNode*>(&expr)) {
            return emitIndexedReference(*index, context);
        }

        throw std::runtime_error("unsupported lvalue emission");
    }

    std::string emitAddress(const Expr& expr, const FunctionContext* context) const {
        return "&" + emitLValue(expr, context);
    }

    std::string emitNamedReference(const std::string& name, const Symbol* symbol, const FunctionContext* context, bool asLValue) const {
        if (context != nullptr && symbol != nullptr && symbol->kind == SymbolKind::Function && name == context->name) {
            return "_";
        }
        if (symbol != nullptr && symbol->kind == SymbolKind::Function) {
            if (asLValue) {
                throw std::runtime_error("function symbol cannot be emitted as external lvalue");
            }
            return name + "()";
        }
        if (symbol != nullptr && symbol->isVarParameter) {
            return "*" + name;
        }
        return name;
    }

    std::string emitNamedLValue(const std::string& name, const FunctionContext* context) const {
        const Symbol* symbol = view_.semantic->globalScope != nullptr ? view_.semantic->globalScope->lookup(name) : nullptr;
        return emitNamedReference(name, symbol, context, true);
    }

    std::string emitIndexedReference(const IndexExprNode& index, const FunctionContext* context) const {
        const Symbol* symbol = lookupBinding(&index);
        std::string result = emitNamedReference(index.baseName, symbol, context, true);
        for (std::size_t i = 0; i < index.indices.size(); ++i) {
            std::string indexExpr = emitExpr(*index.indices[i], context);
            if (symbol != nullptr && symbol->type.isArray && i < symbol->type.dims.size() && symbol->type.dims[i].lower != 0) {
                indexExpr = "(" + indexExpr + " - " + std::to_string(symbol->type.dims[i].lower) + ")";
            }
            result += "[" + indexExpr + "]";
        }
        return result;
    }

    const TypeInfo& exprType(const Expr& expr) const {
        return view_.semantic->expressionTypes.at(&expr);
    }

    const Symbol* lookupBinding(const VarExprNode* expr) const {
        const auto it = view_.semantic->variableBindings.find(expr);
        return it == view_.semantic->variableBindings.end() ? nullptr : it->second;
    }

    const Symbol* lookupBinding(const IndexExprNode* expr) const {
        const auto it = view_.semantic->indexBindings.find(expr);
        return it == view_.semantic->indexBindings.end() ? nullptr : it->second;
    }

    std::string typeName(const TypeInfo& type) const {
        switch (type.basic) {
        case BasicTypeKind::Integer:
            return "int";
        case BasicTypeKind::Real:
            return "float";
        case BasicTypeKind::Boolean:
            return "bool";
        case BasicTypeKind::Char:
            return "char";
        case BasicTypeKind::String:
            return "char *";
        case BasicTypeKind::Void:
        default:
            return "void";
        }
    }

    std::string typeSuffix(const TypeInfo& type) const {
        std::string suffix;
        if (type.isArray) {
            for (const ArrayBound& dim : type.dims) {
                suffix += "[" + std::to_string(dim.upper - dim.lower + 1) + "]";
            }
        }
        return suffix;
    }

    std::string printfSpecifier(const TypeInfo& type) const {
        switch (type.basic) {
        case BasicTypeKind::Integer:
        case BasicTypeKind::Boolean:
            return "%d";
        case BasicTypeKind::Real:
            return "%f";
        case BasicTypeKind::Char:
            return "%c";
        case BasicTypeKind::String:
            return "%s";
        case BasicTypeKind::Void:
        default:
            return "%d";
        }
    }

    std::string scanfSpecifier(const TypeInfo& type) const {
        switch (type.basic) {
        case BasicTypeKind::Integer:
        case BasicTypeKind::Boolean:
            return "%d";
        case BasicTypeKind::Real:
            return "%f";
        case BasicTypeKind::Char:
            return "%c";
        case BasicTypeKind::String:
            return "%s";
        case BasicTypeKind::Void:
        default:
            return "%d";
        }
    }

    std::string emitLiteralText(const LiteralExprNode& literal) const {
        if (literal.kind == LiteralKind::Bool) {
            return literal.rawText == "False" || literal.rawText == "FALSE" || literal.rawText == "false" ? "false" : "true";
        }
        if (literal.kind != LiteralKind::String) {
            return literal.rawText;
        }

        std::string result = "\"";
        for (std::size_t i = 1; i + 1 < literal.rawText.size(); ++i) {
            const char ch = literal.rawText[i];
            if (ch == '\'' && i + 1 < literal.rawText.size() - 1 && literal.rawText[i + 1] == '\'') {
                result += '\'';
                ++i;
                continue;
            }
            if (ch == '\\' || ch == '"') {
                result += '\\';
            }
            result += ch;
        }
        result += "\"";
        return result;
    }

    std::string binaryOp(const BinaryExprNode& expr) const {
        switch (expr.op) {
        case BinaryOp::Add:
            return "+";
        case BinaryOp::Sub:
            return "-";
        case BinaryOp::Mul:
            return "*";
        case BinaryOp::RealDiv:
            return "/";
        case BinaryOp::IntDiv:
            return "/";
        case BinaryOp::Mod:
            return "%";
        case BinaryOp::Eq:
            return "==";
        case BinaryOp::Ne:
            return "!=";
        case BinaryOp::Lt:
            return "<";
        case BinaryOp::Le:
            return "<=";
        case BinaryOp::Gt:
            return ">";
        case BinaryOp::Ge:
            return ">=";
        case BinaryOp::And:
            return isBooleanType(exprType(expr)) ? "&&" : "&";
        case BinaryOp::Or:
            return isBooleanType(exprType(expr)) ? "||" : "|";
        default:
            throw std::runtime_error("unsupported binary operator");
        }
    }

    static std::string join(const std::vector<std::string>& parts, const std::string& separator) {
        std::ostringstream oss;
        for (std::size_t i = 0; i < parts.size(); ++i) {
            if (i != 0) {
                oss << separator;
            }
            oss << parts[i];
        }
        return oss.str();
    }

    const LoweredProgramView& view_;
    CWriter writer_;
};

}  // namespace

std::string CCodeGenerator::generate(const LoweredProgramView& program) const {
    CodegenImpl impl(program);
    return impl.generate();
}

}  // namespace pascal_s2c



