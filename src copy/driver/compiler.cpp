#include "driver/compiler.h"

#include <sstream>

#include "codegen/c_codegen.h"
#include "common/util.h"
#include "lexer/lexer.h"
#include "lower/lower.h"
#include "parser/parser.h"
#include "semantic/analyzer.h"

namespace pascal_s2c {

namespace {

std::string escapeLexeme(const std::string& lexeme) {
    std::string escaped;
    escaped.reserve(lexeme.size());

    for (char ch : lexeme) {
        switch (ch) {
        case '\n':
            escaped += "\\n";
            break;
        case '\r':
            escaped += "\\r";
            break;
        case '\t':
            escaped += "\\t";
            break;
        case '\\':
            escaped += "\\\\";
            break;
        case '"':
            escaped += "\\\"";
            break;
        default:
            escaped.push_back(ch);
            break;
        }
    }

    return escaped;
}

std::string formatTokens(const TokenList& tokens) {
    std::ostringstream output;
    for (const Token& token : tokens) {
        output << token.location.line << ':' << token.location.column << ' '
               << tokenKindName(token.kind) << " \"" << escapeLexeme(token.lexeme) << "\"\n";
    }
    return output.str();
}

}  // namespace

std::string Compiler::lexSource(const std::string& source) const {
    Lexer lexer;
    return formatTokens(lexer.tokenize(source));
}

std::string Compiler::lexFile(const std::string& path) const {
    return lexSource(readTextFile(path));
}

std::string Compiler::compileSource(const std::string& source) const {
    Lexer lexer;
    Parser parser;
    SemanticAnalyzer analyzer;
    LoweringPass lowering;
    CCodeGenerator codegen;

    TokenList tokens = lexer.tokenize(source);
    ProgramPtr program = parser.parse(tokens);
    SemanticContext semantic = analyzer.analyze(*program);
    LoweredProgramView lowered = lowering.lower(*program, semantic);
    return codegen.generate(lowered);
}

std::string Compiler::compileFile(const std::string& path) const {
    return compileSource(readTextFile(path));
}

}  // namespace pascal_s2c
