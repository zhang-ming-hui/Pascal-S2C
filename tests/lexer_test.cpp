#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "../src/lexer/lexer.h"

using namespace pascal_s2c;

int main(int argc, char* argv[]) {
    // 默认文件路径
    std::string filename = "tests/test_cases/t1.pas";
    
    // 如果指定了参数
    if (argc >= 2) {
        filename = argv[1];
    }
    
    // 读取文件内容
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Cannot open file: " << filename << std::endl;
        std::cerr << "Usage: " << argv[0] << " [filename]" << std::endl;
        std::cerr << "Example: " << argv[0] << " tests/test_cases/t1.pas" << std::endl;
        return 1;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();
    file.close();
    
    std::cout << "=== Input file: " << filename << " ===" << std::endl;
    std::cout << "=== Source code ===" << std::endl;
    std::cout << source << std::endl;
    std::cout << std::endl;
    
    // 执行词法分析
    Lexer lexer;
    LexerResult result = lexer.tokenize(source);
    
    // 直接输出传给语法分析器的内容（原始数据）
    std::cout << "=== What Parser receives (LexerResult) ===" << std::endl;
    std::cout << std::endl;
    
    // 输出 tokens
    std::cout << "tokens (" << result.tokens.size() << " items):" << std::endl;
    for (size_t i = 0; i < result.tokens.size(); ++i) {
        const auto& t = result.tokens[i];
        std::cout << "  [" << i << "] kind=" << (int)t.kind 
                  << " (" << tokenKindName(t.kind) << ")"
                  << ", lexeme=\"" << t.lexeme << "\""
                  << ", location=(" << t.location.line 
                  << "," << t.location.column << ")"
                  << std::endl;
        if (t.kind == TokenKind::EndOfFile) break;
    }
    
    std::cout << std::endl;
    
    // 输出 errors
    if (!result.errors.empty()) {
        std::cout << "errors (" << result.errors.size() << " items):" << std::endl;
        for (size_t i = 0; i < result.errors.size(); ++i) {
            const auto& e = result.errors[i];
            std::cout << "  [" << i << "] message=\"" << e.message << "\""
                      << ", location=(" << e.location.line 
                      << "," << e.location.column << ")"
                      << std::endl;
        }
    } else {
        std::cout << "errors: 0 items" << std::endl;
    }
    
    return 0;
}