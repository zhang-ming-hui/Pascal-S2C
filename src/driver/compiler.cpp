#include "driver/compiler.h"

#include <algorithm>
#include <sstream>
#include <vector>

#include "codegen/c_codegen.h"
#include "common/error.h"
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
               << tokenKindName(token.kind) << " \"" << escapeLexeme(token.lexeme) << "\"";
        if (!token.diagnostic.empty()) {
            output << " <" << escapeLexeme(token.diagnostic) << '>';
        }
        output << '\n';
    }
    return output.str();
}

void collectLexerDiagnostics(const TokenList& tokens, std::vector<CompilerError>& diagnostics) {
    for (const Token& token : tokens) {
        if (token.kind != TokenKind::Error) {
            continue;
        }
        diagnostics.emplace_back("lexer", token.diagnostic, token.location);
    }
}

[[noreturn]] void throwAggregatedDiagnostics(std::vector<CompilerError> diagnostics) {
    std::stable_sort(diagnostics.begin(), diagnostics.end(), [](const CompilerError& lhs, const CompilerError& rhs) {
        if (lhs.location().line != rhs.location().line) {
            return lhs.location().line < rhs.location().line;
        }
        if (lhs.location().column != rhs.location().column) {
            return lhs.location().column < rhs.location().column;
        }
        return lhs.stage() < rhs.stage();
    });

    std::ostringstream oss;
    oss << diagnostics.size() << " error(s):";
    for (const CompilerError& error : diagnostics) {
        oss << "\n- [" << error.stage() << "] line " << error.location().line
            << ", column " << error.location().column
            << ": " << error.what();
    }

    throw CompilerError("frontend", oss.str(), diagnostics.front().location());
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
    std::vector<CompilerError> diagnostics;

    TokenList tokens = lexer.tokenize(source);
    collectLexerDiagnostics(tokens, diagnostics);
    if (!diagnostics.empty()) {
        throwAggregatedDiagnostics(std::move(diagnostics));
    }

    ProgramPtr program = parser.parse(tokens, &diagnostics);
    if (!diagnostics.empty()) {
        throwAggregatedDiagnostics(std::move(diagnostics));
    }

    SemanticContext semantic = analyzer.analyze(*program, &diagnostics);
    if (!diagnostics.empty()) {
        throwAggregatedDiagnostics(std::move(diagnostics));
    }

    LoweredProgramView lowered = lowering.lower(*program, semantic);
    return codegen.generate(lowered);
}

std::string Compiler::compileFile(const std::string& path) const {
    return compileSource(readTextFile(path));
}

}  // namespace pascal_s2c
