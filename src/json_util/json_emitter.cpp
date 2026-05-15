#include "json_util/json_emitter.h"

#include <sstream>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

#include "semantic/type.h"

namespace pascal_s2c {

namespace {

class JsonWriter {
public:
    void beginObject() {
        beforeValue();
        output_ << '{';
        stack_.push_back(Context{'}', true});
    }

    void endObject() {
        endContainer('}');
    }

    void beginArray() {
        beforeValue();
        output_ << '[';
        stack_.push_back(Context{']', true});
    }

    void endArray() {
        endContainer(']');
    }

    void key(std::string_view name) {
        if (stack_.empty() || stack_.back().closing != '}') {
            throw std::logic_error("JSON key outside of object");
        }

        beforeElement();
        writeEscapedString(name);
        output_ << ": ";
        expectingValue_ = true;
    }

    void stringValue(std::string_view value) {
        beforeValue();
        writeEscapedString(value);
    }

    void intValue(int value) {
        beforeValue();
        output_ << value;
    }

    void boolValue(bool value) {
        beforeValue();
        output_ << (value ? "true" : "false");
    }

    void nullValue() {
        beforeValue();
        output_ << "null";
    }

    std::string finish() const {
        return output_.str();
    }

private:
    struct Context {
        char closing;
        bool first;
    };

    std::ostringstream output_;
    std::vector<Context> stack_;
    bool expectingValue_ = false;

    void beforeValue() {
        if (expectingValue_) {
            expectingValue_ = false;
            return;
        }
        beforeElement();
    }

    void beforeElement() {
        if (stack_.empty()) {
            return;
        }

        Context& context = stack_.back();
        if (context.first) {
            output_ << '\n';
            context.first = false;
        } else {
            output_ << ",\n";
        }
        writeIndent(stack_.size());
    }

    void endContainer(char expected) {
        if (stack_.empty() || stack_.back().closing != expected) {
            throw std::logic_error("JSON container mismatch");
        }

        Context context = stack_.back();
        stack_.pop_back();
        if (!context.first) {
            output_ << '\n';
            writeIndent(stack_.size());
        }
        output_ << expected;
    }

    void writeIndent(std::size_t depth) {
        for (std::size_t i = 0; i < depth; ++i) {
            output_ << "  ";
        }
    }

    void writeEscapedString(std::string_view value) {
        output_ << '"';
        for (char ch : value) {
            switch (ch) {
            case '\\':
                output_ << "\\\\";
                break;
            case '"':
                output_ << "\\\"";
                break;
            case '\b':
                output_ << "\\b";
                break;
            case '\f':
                output_ << "\\f";
                break;
            case '\n':
                output_ << "\\n";
                break;
            case '\r':
                output_ << "\\r";
                break;
            case '\t':
                output_ << "\\t";
                break;
            default:
                if (static_cast<unsigned char>(ch) < 0x20) {
                    static const char hex[] = "0123456789abcdef";
                    output_ << "\\u00";
                    output_ << hex[(static_cast<unsigned char>(ch) >> 4) & 0x0f];
                    output_ << hex[static_cast<unsigned char>(ch) & 0x0f];
                } else {
                    output_ << ch;
                }
                break;
            }
        }
        output_ << '"';
    }
};

template <typename EmitFn>
void emitObjectArray(JsonWriter& writer,
                     std::string_view key,
                     EmitFn emitItems) {
    writer.key(key);
    writer.beginArray();
    emitItems();
    writer.endArray();
}

void emitLocation(JsonWriter& writer, const SourceLocation& loc) {
    writer.beginObject();
    writer.key("line");
    writer.intValue(loc.line);
    writer.key("column");
    writer.intValue(loc.column);
    writer.endObject();
}

void emitArrayBound(JsonWriter& writer, const ArrayBound& bound) {
    writer.beginObject();
    writer.key("lower");
    writer.intValue(bound.lower);
    writer.key("upper");
    writer.intValue(bound.upper);
    writer.endObject();
}

std::string_view toJsonName(ParamPassMode mode) {
    switch (mode) {
    case ParamPassMode::Value:
        return "value";
    case ParamPassMode::Var:
        return "var";
    default:
        return "unknown";
    }
}

std::string_view toJsonName(BinaryOp op) {
    switch (op) {
    case BinaryOp::Add:
        return "+";
    case BinaryOp::Sub:
        return "-";
    case BinaryOp::Mul:
        return "*";
    case BinaryOp::RealDiv:
        return "/";
    case BinaryOp::IntDiv:
        return "div";
    case BinaryOp::Mod:
        return "mod";
    case BinaryOp::Eq:
        return "=";
    case BinaryOp::Ne:
        return "<>";
    case BinaryOp::Lt:
        return "<";
    case BinaryOp::Le:
        return "<=";
    case BinaryOp::Gt:
        return ">";
    case BinaryOp::Ge:
        return ">=";
    case BinaryOp::And:
        return "and";
    case BinaryOp::Or:
        return "or";
    default:
        return "unknown";
    }
}

std::string_view toJsonName(UnaryOp op) {
    switch (op) {
    case UnaryOp::Plus:
        return "+";
    case UnaryOp::Minus:
        return "-";
    case UnaryOp::Not:
        return "not";
    default:
        return "unknown";
    }
}

std::string_view toJsonName(LiteralKind kind) {
    switch (kind) {
    case LiteralKind::Int:
        return "int";
    case LiteralKind::Real:
        return "real";
    case LiteralKind::Bool:
        return "bool";
    case LiteralKind::Char:
        return "char";
    case LiteralKind::String:
        return "string";
    default:
        return "unknown";
    }
}

void emitStringArray(JsonWriter& writer, std::string_view key, const std::vector<std::string>& values) {
    writer.key(key);
    writer.beginArray();
    for (const std::string& value : values) {
        writer.stringValue(value);
    }
    writer.endArray();
}

template <typename T, typename EmitFn>
void emitNodeArray(JsonWriter& writer,
                   std::string_view key,
                   const std::vector<std::unique_ptr<T>>& values,
                   EmitFn emitValue) {
    emitObjectArray(writer, key, [&]() {
        for (const std::unique_ptr<T>& value : values) {
            emitValue(writer, *value);
        }
    });
}

void emitExpr(JsonWriter& writer, const Expr& expr);
void emitStmt(JsonWriter& writer, const Stmt& stmt);
void emitType(JsonWriter& writer, const TypeNode& type);
void emitBlock(JsonWriter& writer, const BlockNode& block);
void emitSubprogram(JsonWriter& writer, const SubprogramDeclNode& decl);

void emitNodeHeader(JsonWriter& writer, std::string_view nodeType, const Node& node) {
    writer.key("node_type");
    writer.stringValue(nodeType);
    writer.key("loc");
    emitLocation(writer, node.loc);
}

void emitType(JsonWriter& writer, const TypeNode& type) {
    writer.beginObject();

    if (const auto* scalar = dynamic_cast<const ScalarTypeNode*>(&type)) {
        emitNodeHeader(writer, "ScalarType", *scalar);
        writer.key("kind");
        writer.stringValue(toString(scalar->kind));
    } else if (const auto* array = dynamic_cast<const ArrayTypeNode*>(&type)) {
        emitNodeHeader(writer, "ArrayType", *array);
        writer.key("dims");
        writer.beginArray();
        for (const ArrayBound& bound : array->dims) {
            emitArrayBound(writer, bound);
        }
        writer.endArray();
        writer.key("element_type");
        if (array->elementType) {
            emitType(writer, *array->elementType);
        } else {
            writer.nullValue();
        }
    } else {
        throw std::runtime_error("unknown TypeNode subtype");
    }

    writer.endObject();
}

void emitParamDecl(JsonWriter& writer, const ParamDeclNode& param) {
    writer.beginObject();
    emitNodeHeader(writer, "ParamDecl", param);
    emitStringArray(writer, "names", param.names);
    writer.key("type");
    writer.stringValue(toString(param.type));
    writer.key("pass_mode");
    writer.stringValue(toJsonName(param.passMode));
    writer.endObject();
}

void emitConstDecl(JsonWriter& writer, const ConstDeclNode& decl) {
    writer.beginObject();
    emitNodeHeader(writer, "ConstDecl", decl);
    writer.key("name");
    writer.stringValue(decl.name);
    writer.key("value");
    if (decl.value) {
        emitExpr(writer, *decl.value);
    } else {
        writer.nullValue();
    }
    writer.endObject();
}

void emitVarDecl(JsonWriter& writer, const VarDeclNode& decl) {
    writer.beginObject();
    emitNodeHeader(writer, "VarDecl", decl);
    emitStringArray(writer, "names", decl.names);
    writer.key("type");
    if (decl.type) {
        emitType(writer, *decl.type);
    } else {
        writer.nullValue();
    }
    writer.endObject();
}

void emitSubprogram(JsonWriter& writer, const SubprogramDeclNode& decl) {
    writer.beginObject();

    if (const auto* function = dynamic_cast<const FunctionDeclNode*>(&decl)) {
        emitNodeHeader(writer, "FunctionDecl", *function);
        writer.key("name");
        writer.stringValue(function->name);
        emitNodeArray(writer, "params", function->params, emitParamDecl);
        writer.key("return_type");
        writer.stringValue(toString(function->returnType));
        writer.key("block");
        if (function->block) {
            emitBlock(writer, *function->block);
        } else {
            writer.nullValue();
        }
    } else if (const auto* procedure = dynamic_cast<const ProcedureDeclNode*>(&decl)) {
        emitNodeHeader(writer, "ProcedureDecl", *procedure);
        writer.key("name");
        writer.stringValue(procedure->name);
        emitNodeArray(writer, "params", procedure->params, emitParamDecl);
        writer.key("block");
        if (procedure->block) {
            emitBlock(writer, *procedure->block);
        } else {
            writer.nullValue();
        }
    } else {
        throw std::runtime_error("unknown SubprogramDeclNode subtype");
    }

    writer.endObject();
}

void emitCaseBranch(JsonWriter& writer, const CaseBranchNode& branch) {
    writer.beginObject();
    emitNodeHeader(writer, "CaseBranch", branch);
    emitNodeArray(writer, "labels", branch.labels, emitExpr);
    writer.key("body");
    if (branch.body) {
        emitStmt(writer, *branch.body);
    } else {
        writer.nullValue();
    }
    writer.endObject();
}

void emitStmt(JsonWriter& writer, const Stmt& stmt) {
    writer.beginObject();

    if (const auto* compound = dynamic_cast<const CompoundStmtNode*>(&stmt)) {
        emitNodeHeader(writer, "CompoundStmt", *compound);
        emitNodeArray(writer, "statements", compound->statements, emitStmt);
    } else if (const auto* breakStmt = dynamic_cast<const BreakStmtNode*>(&stmt)) {
        emitNodeHeader(writer, "BreakStmt", *breakStmt);
    } else if (const auto* assign = dynamic_cast<const AssignStmtNode*>(&stmt)) {
        emitNodeHeader(writer, "AssignStmt", *assign);
        writer.key("target");
        if (assign->target) {
            emitExpr(writer, *assign->target);
        } else {
            writer.nullValue();
        }
        writer.key("value");
        if (assign->value) {
            emitExpr(writer, *assign->value);
        } else {
            writer.nullValue();
        }
    } else if (const auto* call = dynamic_cast<const CallStmtNode*>(&stmt)) {
        emitNodeHeader(writer, "CallStmt", *call);
        writer.key("name");
        writer.stringValue(call->name);
        emitNodeArray(writer, "args", call->args, emitExpr);
    } else if (const auto* ifStmt = dynamic_cast<const IfStmtNode*>(&stmt)) {
        emitNodeHeader(writer, "IfStmt", *ifStmt);
        writer.key("condition");
        if (ifStmt->condition) {
            emitExpr(writer, *ifStmt->condition);
        } else {
            writer.nullValue();
        }
        writer.key("then_branch");
        if (ifStmt->thenBranch) {
            emitStmt(writer, *ifStmt->thenBranch);
        } else {
            writer.nullValue();
        }
        writer.key("else_branch");
        if (ifStmt->elseBranch) {
            emitStmt(writer, *ifStmt->elseBranch);
        } else {
            writer.nullValue();
        }
    } else if (const auto* whileStmt = dynamic_cast<const WhileStmtNode*>(&stmt)) {
        emitNodeHeader(writer, "WhileStmt", *whileStmt);
        writer.key("condition");
        if (whileStmt->condition) {
            emitExpr(writer, *whileStmt->condition);
        } else {
            writer.nullValue();
        }
        writer.key("body");
        if (whileStmt->body) {
            emitStmt(writer, *whileStmt->body);
        } else {
            writer.nullValue();
        }
    } else if (const auto* forStmt = dynamic_cast<const ForStmtNode*>(&stmt)) {
        emitNodeHeader(writer, "ForStmt", *forStmt);
        writer.key("var_name");
        writer.stringValue(forStmt->varName);
        writer.key("start");
        if (forStmt->start) {
            emitExpr(writer, *forStmt->start);
        } else {
            writer.nullValue();
        }
        writer.key("stop");
        if (forStmt->stop) {
            emitExpr(writer, *forStmt->stop);
        } else {
            writer.nullValue();
        }
        writer.key("body");
        if (forStmt->body) {
            emitStmt(writer, *forStmt->body);
        } else {
            writer.nullValue();
        }
    } else if (const auto* readStmt = dynamic_cast<const ReadStmtNode*>(&stmt)) {
        emitNodeHeader(writer, "ReadStmt", *readStmt);
        emitNodeArray(writer, "targets", readStmt->targets, emitExpr);
    } else if (const auto* writeStmt = dynamic_cast<const WriteStmtNode*>(&stmt)) {
        emitNodeHeader(writer, "WriteStmt", *writeStmt);
        emitNodeArray(writer, "values", writeStmt->values, emitExpr);
    } else if (const auto* caseStmt = dynamic_cast<const CaseStmtNode*>(&stmt)) {
        emitNodeHeader(writer, "CaseStmt", *caseStmt);
        writer.key("selector");
        if (caseStmt->selector) {
            emitExpr(writer, *caseStmt->selector);
        } else {
            writer.nullValue();
        }
        emitNodeArray(writer, "branches", caseStmt->branches, emitCaseBranch);
    } else {
        throw std::runtime_error("unknown Stmt subtype");
    }

    writer.endObject();
}

void emitExpr(JsonWriter& writer, const Expr& expr) {
    writer.beginObject();

    if (const auto* binary = dynamic_cast<const BinaryExprNode*>(&expr)) {
        emitNodeHeader(writer, "BinaryExpr", *binary);
        writer.key("op");
        writer.stringValue(toJsonName(binary->op));
        writer.key("lhs");
        if (binary->lhs) {
            emitExpr(writer, *binary->lhs);
        } else {
            writer.nullValue();
        }
        writer.key("rhs");
        if (binary->rhs) {
            emitExpr(writer, *binary->rhs);
        } else {
            writer.nullValue();
        }
    } else if (const auto* unary = dynamic_cast<const UnaryExprNode*>(&expr)) {
        emitNodeHeader(writer, "UnaryExpr", *unary);
        writer.key("op");
        writer.stringValue(toJsonName(unary->op));
        writer.key("operand");
        if (unary->operand) {
            emitExpr(writer, *unary->operand);
        } else {
            writer.nullValue();
        }
    } else if (const auto* call = dynamic_cast<const CallExprNode*>(&expr)) {
        emitNodeHeader(writer, "CallExpr", *call);
        writer.key("name");
        writer.stringValue(call->name);
        emitNodeArray(writer, "args", call->args, emitExpr);
    } else if (const auto* var = dynamic_cast<const VarExprNode*>(&expr)) {
        emitNodeHeader(writer, "VarExpr", *var);
        writer.key("name");
        writer.stringValue(var->name);
    } else if (const auto* index = dynamic_cast<const IndexExprNode*>(&expr)) {
        emitNodeHeader(writer, "IndexExpr", *index);
        writer.key("base_name");
        writer.stringValue(index->baseName);
        emitNodeArray(writer, "indices", index->indices, emitExpr);
    } else if (const auto* literal = dynamic_cast<const LiteralExprNode*>(&expr)) {
        emitNodeHeader(writer, "LiteralExpr", *literal);
        writer.key("kind");
        writer.stringValue(toJsonName(literal->kind));
        writer.key("raw_text");
        writer.stringValue(literal->rawText);
    } else {
        throw std::runtime_error("unknown Expr subtype");
    }

    writer.endObject();
}

void emitBlock(JsonWriter& writer, const BlockNode& block) {
    writer.beginObject();
    emitNodeHeader(writer, "Block", block);
    emitNodeArray(writer, "const_decls", block.constDecls, emitConstDecl);
    emitNodeArray(writer, "var_decls", block.varDecls, emitVarDecl);
    emitNodeArray(writer, "subprograms", block.subprograms, emitSubprogram);
    writer.key("body");
    if (block.body) {
        emitStmt(writer, *block.body);
    } else {
        writer.nullValue();
    }
    writer.endObject();
}

void emitProgram(JsonWriter& writer, const ProgramNode& program) {
    writer.beginObject();
    emitNodeHeader(writer, "Program", program);
    writer.key("name");
    writer.stringValue(program.name);
    writer.key("block");
    if (program.block) {
        emitBlock(writer, *program.block);
    } else {
        writer.nullValue();
    }
    writer.endObject();
}

}  // namespace

std::string emitParseJson(const ProgramNode& program) {
    JsonWriter writer;
    writer.beginObject();
    writer.key("phase");
    writer.stringValue("PARSE");
    writer.key("status");
    writer.stringValue("ok");
    writer.key("ast");
    emitProgram(writer, program);
    writer.endObject();
    return writer.finish();
}

}  // namespace pascal_s2c
