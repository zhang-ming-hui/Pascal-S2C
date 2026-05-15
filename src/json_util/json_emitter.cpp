#include "driver/json_emitter.h"
#include "driver/json_utils.h"

#include <nlohmann/json.hpp>

namespace pascal_s2c {

using json = nlohmann::json;
using namespace json_utils;

// ------------------ 词法 JSON（序列化） ------------------

json emitLexJson(const TokenList& tokens) {
    json j;
    j["phase"] = "LEX";
    j["status"] = "ok";

    j["tokens"] = json::array();
    for (const Token& token : tokens) {
        j["tokens"].push_back({
            {"type", tokenKindToString(token.kind)},
            {"val", token.lexeme},
            {"line", token.location.line},
            {"col", token.location.column}
        });
    }

    return j;
}

std::string ir_token_to_json(const TokenList& tokens) {
    return emitLexJson(tokens).dump(2);
}

// ------------------ 词法 JSON（反序列化） ------------------

TokenList ir_json_to_token(const std::string& json_str) {
    json j = json::parse(json_str);
    TokenList tokens;

    for (const auto& item : j["tokens"]) {
        Token token;
        token.kind = stringToTokenKind(item.value("type", ""));
        token.lexeme = item.value("val", "");
        token.location.line = item.value("line", 1);
        token.location.column = item.value("col", 1);
        tokens.push_back(token);
    }

    return tokens;
}

// ------------------ 语法 JSON（序列化） ------------------

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

std::string ir_ast_to_json(const ProgramNode& program) {
    return emitParseJson(program).dump(2);
}

// ------------------ 语法 JSON（反序列化，基础骨架） ------------------

std::unique_ptr<ProgramNode> ir_json_to_ast(const std::string& json_str) {
    json j = json::parse(json_str);
    auto program = std::make_unique<ProgramNode>();

    const json& ast = j["ast"];
    program->name = ast.value("name", "");
    program->loc = jsonToLocation(ast.value("loc", json::object()));

    if (ast.contains("block")) {
        const json& blockJson = ast["block"];
        auto block = std::make_unique<BlockNode>();
        block->loc = jsonToLocation(blockJson.value("loc", json::object()));

        // 恢复 constDecls（仅 name，value 为占位 Expr）
        if (blockJson.contains("constDecls")) {
            for (const auto& declJson : blockJson["constDecls"]) {
                auto decl = std::make_unique<ConstDeclNode>();
                decl->name = declJson.value("name", "");
                decl->loc = jsonToLocation(declJson.value("loc", json::object()));
                // value 为简化占位
                decl->value = std::make_unique<LiteralExprNode>();
                block->constDecls.push_back(std::move(decl));
            }
        }

        // 恢复 varDecls（仅 names，type 为占位 TypeNode）
        if (blockJson.contains("varDecls")) {
            for (const auto& declJson : blockJson["varDecls"]) {
                auto decl = std::make_unique<VarDeclNode>();
                if (declJson.contains("names")) {
                    for (const auto& name : declJson["names"]) {
                        decl->names.push_back(name.get<std::string>());
                    }
                }
                decl->loc = jsonToLocation(declJson.value("loc", json::object()));
                decl->type = std::make_unique<ScalarTypeNode>();
                block->varDecls.push_back(std::move(decl));
            }
        }

        // 恢复 subprograms（仅 name 和 return_type）
        if (blockJson.contains("subprograms")) {
            for (const auto& subJson : blockJson["subprograms"]) {
                std::unique_ptr<SubprogramDeclNode> sub;
                if (subJson.contains("return_type")) {
                    auto fn = std::make_unique<FunctionDeclNode>();
                    // 简化处理：默认 Integer
                    fn->returnType = BasicTypeKind::Integer;
                    sub = std::move(fn);
                } else {
                    sub = std::make_unique<ProcedureDeclNode>();
                }
                sub->name = subJson.value("name", "");
                sub->loc = jsonToLocation(subJson.value("loc", json::object()));
                // block 为占位
                sub->block = std::make_unique<BlockNode>();
                block->subprograms.push_back(std::move(sub));
            }
        }

        // 恢复 body（占位 CompoundStmt）
        if (blockJson.contains("body")) {
            block->body = std::make_unique<CompoundStmtNode>();
            block->body->loc = jsonToLocation(blockJson["body"].value("loc", json::object()));
        }

        program->block = std::move(block);
    }

    return program;
}

// ------------------ 错误 JSON（单条） ------------------

json emitErrorJson(const CompilerError& error) {
    json j;
    j["phase"] = error.stage();
    j["status"] = "error";

    json err;
    err["err_type"] = error.stage();
    err["message"] = error.what();
    err["line"] = error.location().line;
    err["col"] = error.location().column;

    j["errors"] = json::array({err});
    return j;
}

// ------------------ 错误 JSON（批量错误流） ------------------

json emitErrorStreamJson(const ErrorStream& errors) {
    json j;
    j["phase"] = "compilation";
    j["status"] = "error";

    j["errors"] = json::array();
    for (const auto& item : errors) {
        j["errors"].push_back({
            {"err_type", item.err_type},
            {"message", item.message},
            {"line", item.line},
            {"col", item.col}
        });
    }

    return j;
}

std::string ir_errors_to_json(const ErrorStream& errors) {
    return emitErrorStreamJson(errors).dump(2);
}

} // namespace pascal_s2c
