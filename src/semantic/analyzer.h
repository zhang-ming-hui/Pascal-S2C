#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "ast/ast.h"
#include "semantic/scope.h"
#include "semantic/symbol.h"
#include "semantic/type.h"

namespace pascal_s2c {

struct SemanticContext {
    std::string entryProgramName;
    std::vector<std::unique_ptr<Scope>> ownedScopes;
    const Scope* globalScope = nullptr;
    std::unordered_map<const Expr*, TypeInfo> expressionTypes;
    std::unordered_map<const VarExprNode*, const Symbol*> variableBindings;
    std::unordered_map<const IndexExprNode*, const Symbol*> indexBindings;
    std::unordered_map<const CallExprNode*, const Symbol*> callExprBindings;
    std::unordered_map<const CallStmtNode*, const Symbol*> callStmtBindings;
};

class SemanticAnalyzer {
public:
    SemanticContext analyze(const ProgramNode& program) const;
};

}  // namespace pascal_s2c
