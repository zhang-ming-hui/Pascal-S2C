#include "lower/lower.h"

namespace pascal_s2c {

LoweredProgramView LoweringPass::lower(const ProgramNode& program, const SemanticContext& semantic) const {
    LoweredProgramView view;
    view.program = &program;
    view.semantic = &semantic;
    return view;
}

}  // namespace pascal_s2c
