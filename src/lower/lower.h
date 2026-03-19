#pragma once

#include "ast/ast.h"
#include "semantic/analyzer.h"

namespace pascal_s2c {

struct LoweredProgramView {
    const ProgramNode* program = nullptr;
    const SemanticContext* semantic = nullptr;
};

class LoweringPass {
public:
    LoweredProgramView lower(const ProgramNode& program, const SemanticContext& semantic) const;
};

}  // namespace pascal_s2c
