#include "ast/ast.h"

namespace pascal_s2c {

ProgramPtr makePlaceholderProgram(std::string name) {
    auto program = std::make_unique<ProgramNode>();
    program->name = std::move(name);
    program->block = std::make_unique<BlockNode>();
    program->block->body = std::make_unique<CompoundStmtNode>();
    return program;
}

}  // namespace pascal_s2c
