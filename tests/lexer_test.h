#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

#include "lexer/lexer.h"
#include "lexer/token.h"

namespace pascal_s2c {
namespace test {

// 测试结果结构
struct TestResult {
    std::string testName;
    bool passed;
    std::string message;
    int errorCount;
    int tokenCount;
};

// 测试运行器类
class LexerTestRunner {
public:
    LexerTestRunner();
    void runAllTests();
    void printSummary();
    
private:
    std::vector<TestResult> results;
    Lexer lexer;
    
    // 测试函数
    void testValidProgram();
    void testProgramWithErrors();
    void testComments();
    void testTokens();  // 测试特定 Token 识别
    
    // 辅助函数
    std::string readFile(const std::string& filename);
    void printTokens(const TokenList& tokens, int maxTokens = 30);
    void printErrors(const std::vector<LexicalError>& errors);
    
    // 断言函数
    void assertTrue(bool condition, const std::string& testName, 
                    const std::string& message, int& errorCount);
};

} // namespace test
} // namespace pascal_s2c