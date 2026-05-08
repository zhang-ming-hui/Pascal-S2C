#include "driver/json_emitter.h"
#include "driver/json_utils.h"

#include <nlohmann/json.hpp>

namespace pascal_s2c {

using json = nlohmann::json;
using namespace json_utils;

// ------------------ 词法 JSON ------------------

json emitLexJson(const TokenList& tokens) {
    json j;
    j["phase"] = "LEX";
    j["status"] = "ok";

    j["tokens"] = json::array();
    for (const Token& token : tokens) {
        j["tokens"].push_back({
            {"type", tokenKindToString(token.kind)},
            {"value", token.lexeme},
            {"line", token.location.line},
            {"column", token.location.column}
        });
    }

    return j;
}

// ------------------ 语法 JSON ------------------

namespace {

json emitExprJson(const Expr& expr) {
    json j;
    j["node_type"] = "Expr";
    j["loc"] = locationToJson(expr.loc);
    return j;
}

json emitStmtJson(const Stmt& stmt) {
    json j;
    j["node_type"] = "Stmt";
    j["loc"] = locationToJson(stmt.loc);
    return j;
}

json emitTypeJson(const TypeNode& type) {
    json j;
    j["node_type"] = "Type";
    j["loc"] = locationToJson(type.loc);
    return j;
}

json emitConstDeclJson(const ConstDeclNode& decl) {
    json j;
    j["node_type"] = "ConstDecl";
    j["name"] = decl.name;
    j["loc"] = locationToJson(decl.loc);
    if (decl.value) {
        j["value"] = emitExprJson(*decl.value);
    }
    return j;
}

json emitVarDeclJson(const VarDeclNode& decl) {
    json j;
    j["node_type"] = "VarDecl";
    j["names"] = decl.names;
    j["loc"] = locationToJson(decl.loc);
    if (decl.type) {
        j["type"] = emitTypeJson(*decl.type);
    }
    return j;
}

json emitParamDeclJson(const ParamDeclNode& param) {
    json j;
    j["node_type"] = "ParamDecl";
    j["names"] = param.names;
    j["loc"] = locationToJson(param.loc);
    return j;
}

json emitSubprogramJson(const SubprogramDeclNode& sub) {
    json j;
    j["node_type"] = "Subprogram";
    j["name"] = sub.name;
    j["loc"] = locationToJson(sub.loc);

    j["params"] = json::array();
    for (const auto& param : sub.params) {
        j["params"].push_back(emitParamDeclJson(*param));
    }

    if (sub.block) {
        j["block"] = {
            {"node_type", "Block"},
            {"loc", locationToJson(sub.block->loc)}
        };
    }

    if (const auto* fn = dynamic_cast<const FunctionDeclNode*>(&sub)) {
        j["return_type"] = toString(fn->returnType);
    }

    return j;
}

json emitCompoundStmtJson(const CompoundStmtNode& compound) {
    json j;
    j["node_type"] = "CompoundStmt";
    j["loc"] = locationToJson(compound.loc);

    j["statements"] = json::array();
    for (const auto& stmt : compound.statements) {
        j["statements"].push_back(emitStmtJson(*stmt));
    }

    return j;
}

json emitBlockJson(const BlockNode& block) {
    json j;
    j["node_type"] = "Block";
    j["loc"] = locationToJson(block.loc);

    j["constDecls"] = json::array();
    for (const auto& decl : block.constDecls) {
        j["constDecls"].push_back(emitConstDeclJson(*decl));
    }

    j["varDecls"] = json::array();
    for (const auto& decl : block.varDecls) {
        j["varDecls"].push_back(emitVarDeclJson(*decl));
    }

    j["subprograms"] = json::array();
    for (const auto& sub : block.subprograms) {
        j["subprograms"].push_back(emitSubprogramJson(*sub));
    }

    if (block.body) {
        j["body"] = emitCompoundStmtJson(*block.body);
    }

    return j;
}

} // namespace

json emitParseJson(const ProgramNode& program) {
    json j;
    j["phase"] = "PARSE";
    j["status"] = "ok";

    json ast;
    ast["node_type"] = "Program";
    ast["name"] = program.name;
    ast["loc"] = locationToJson(program.loc);

    if (program.block) {
        ast["block"] = emitBlockJson(*program.block);
    }

    j["ast"] = ast;
    return j;
}

// ------------------ 错误 JSON ------------------

json emitErrorJson(const CompilerError& error) {
    json j;
    j["phase"] = error.stage();
    j["status"] = "error";

    json err;
    err["phase"] = error.stage();
    err["message"] = error.what();
    err["line"] = error.location().line;
    err["column"] = error.location().column;

    j["errors"] = json::array({err});
    return j;
}

} // namespace pascal_s2c
