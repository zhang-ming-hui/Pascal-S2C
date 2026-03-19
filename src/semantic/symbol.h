#pragma once

#include <string>
#include <vector>

#include "semantic/type.h"

namespace pascal_s2c {

enum class SymbolKind {
    Constant,
    Variable,
    Parameter,
    Function,
    Procedure
};

struct SymbolParameter {
    TypeInfo type;
    bool isVar = false;
};

struct Symbol {
    std::string name;
    SymbolKind kind = SymbolKind::Variable;
    TypeInfo type;
    bool isGlobal = false;
    bool isVarParameter = false;
    std::vector<SymbolParameter> parameters;
};

inline bool isCallableSymbol(const Symbol& symbol) {
    return symbol.kind == SymbolKind::Function || symbol.kind == SymbolKind::Procedure;
}

inline bool isAssignableSymbol(const Symbol& symbol) {
    return symbol.kind == SymbolKind::Variable || symbol.kind == SymbolKind::Parameter || symbol.kind == SymbolKind::Function;
}

}  // namespace pascal_s2c
