#include "driver/compiler.h"

#include "codegen/c_codegen.h"
#include "common/util.h"
#include "lexer/lexer.h"
#include "lower/lower.h"
#include "parser/parser.h"
#include "semantic/analyzer.h"

namespace pascal_s2c {

std::string Compiler::compileSource(const std::string& source) const {
    Lexer lexer;
    Parser parser;
    SemanticAnalyzer analyzer;
    LoweringPass lowering;
    CCodeGenerator codegen;

    // 前端流水线：源码 -> token 序列 -> AST -> 语义上下文 -> lowered 视图 -> C 代码文本。
    TokenList tokens = lexer.tokenize(source);
    ProgramPtr program = parser.parse(tokens);
    SemanticContext semantic = analyzer.analyze(*program);
    LoweredProgramView lowered = lowering.lower(*program, semantic);
    return codegen.generate(lowered);
}

std::string Compiler::compileFile(const std::string& path) const {
    // 文件入口统一复用 compileSource 的编译流水线。
    return compileSource(readTextFile(path));
}

}  // namespace pascal_s2c
