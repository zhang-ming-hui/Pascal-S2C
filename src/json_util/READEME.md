Pascal-S2C 编译器 JSON 接口设计与实现


1. 文件总体分工

文件 核心职责

json_utils.h 原子级转换工具

json_emitter.h 对外接口定义

json_emitter.cpp 递归构建 AST / Token / Error 的 JSON


2. json_utils.h 详细说明

2.1 tokenKindToString

函数作用：  
将 Lexer 中的 TokenKind 枚举值转换为字符串，供 JSON 输出使用。
inline std::string tokenKindToString(TokenKind kind) {
    return tokenKindName(kind);  // 复用 Lexer 已有映射
}

设计理由：  
• 避免在 JSON 模块中维护第二份 Token 类型表  

• 保证 Lexer 与 JSON 输出语义一致  

2.2 locationToJson

函数作用：  
统一生成位置信息 JSON。
inline nlohmann::json locationToJson(const SourceLocation& loc) {
    return {
        {"line", loc.line},     // 行号（从 1 开始）
        {"column", loc.column}  // 列号（从 1 开始）
    };
}

设计理由：  
• 所有阶段（词法 / 语法 / 错误）共用同一位置格式  

• 后期如需增加 file 字段，只需修改此处  


3. json_emitter.h 详细说明

3.1 emitLexJson

nlohmann::json emitLexJson(const TokenList& tokens);

函数作用：  
将 Lexer 输出的 TokenList 转换为标准 JSON。

输入：  
• tokens：完整的 Token 流  

输出：  
• 词法阶段 JSON 文档  

3.2 emitParseJson

nlohmann::json emitParseJson(const ProgramNode& program);


函数作用：  
将 Parser 构建的 AST 转换为标准 JSON。

输入：  
• program：AST 根节点  

输出：  
• 语法阶段 JSON 文档  

3.3 emitErrorJson

nlohmann::json emitErrorJson(const CompilerError& error);


函数作用：  
将编译错误包装为统一 JSON 格式。

输入：  
• error：CompilerError 对象  

输出：  
• 错误 JSON 文档  



4. json_emitter.cpp 详细说明（重点）

4.1 词法 JSON：emitLexJson

4.1.1 函数职责

函数作用：  
构建词法分析阶段的完整 JSON 输出，包括阶段标识、状态和 Token 列表。
json emitLexJson(const TokenList& tokens) {
    json j;

    j["phase"] = "LEX";    
    // 标识当前阶段为词法分析

    j["status"] = "ok";    
    // 表示阶段执行成功

    j["tokens"] = json::array();
    // 初始化 Token 数组

    for (const Token& token : tokens) {
        j["tokens"].push_back({
            {"type", tokenKindToString(token.kind)},
            // Token 类型（如 Identifier）

            {"value", token.lexeme},
            // Token 的原始字符串

            {"line", token.location.line},
            // 行号

            {"column", token.location.column}
            // 列号
        });
    }

    return j;
}


对应输出格式：
{
  "phase": "LEX",
  "status": "ok",
  "tokens": [
    {
      "type": "Identifier",
      "value": "main",
      "line": 1,
      "column": 9
    }
  ]
}


4.2 语法 JSON：AST 构建体系

4.2.1 emitExprJson

函数作用：  
处理表达式节点（基类），作为表达式 JSON 的兜底实现。
json emitExprJson(const Expr& expr) {
    json j;
    j["node_type"] = "Expr";       // 表达式节点类型
    j["loc"] = locationToJson(expr.loc); // 位置信息
    return j;
}


4.2.2 emitStmtJson

函数作用：  
处理语句节点（基类），作为语句 JSON 的兜底实现。
json emitStmtJson(const Stmt& stmt) {
    json j;
    j["node_type"] = "Stmt";       // 语句节点类型
    j["loc"] = locationToJson(stmt.loc); // 位置信息
    return j;
}


4.2.3 emitConstDeclJson

函数作用：  
将常量声明节点转换为 JSON。
json emitConstDeclJson(const ConstDeclNode& decl) {
    json j;
    j["node_type"] = "ConstDecl";  // 常量声明
    j["name"] = decl.name;         // 常量名
    j["loc"] = locationToJson(decl.loc);

    if (decl.value) {
        j["value"] = emitExprJson(*decl.value);
        // 常量值（表达式）
    }
    return j;
}


4.2.4 emitVarDeclJson

函数作用：  
将变量声明节点转换为 JSON，支持多变量同类型声明。
json emitVarDeclJson(const VarDeclNode& decl) {
    json j;
    j["node_type"] = "VarDecl";    // 变量声明
    j["names"] = decl.names;       // 变量名列表
    j["loc"] = locationToJson(decl.loc);

    if (decl.type) {
        j["type"] = emitTypeJson(*decl.type);
        // 变量类型
    }
    return j;
}


4.2.5 emitCompoundStmtJson

函数作用：  
处理 begin ... end 语句块，递归处理其中的每条语句。
json emitCompoundStmtJson(const CompoundStmtNode& compound) {
    json j;
    j["node_type"] = "CompoundStmt"; // 复合语句
    j["loc"] = locationToJson(compound.loc);

    j["statements"] = json::array();
    for (const auto& stmt : compound.statements) {
        j["statements"].push_back(emitStmtJson(*stmt));
        // 递归生成每条语句的 JSON
    }
    return j;
}


4.2.6 emitBlockJson

函数作用：  
构建 Pascal 的 Block 节点，连接常量、变量、子程序和主语句体。
json emitBlockJson(const BlockNode& block) {
    json j;
    j["node_type"] = "Block";      // Block 节点
    j["loc"] = locationToJson(block.loc);

    j["constDecls"] = json::array();
    for (const auto& decl : block.constDecls) {
        j["constDecls"].push_back(emitConstDeclJson(*decl));
    }

    j["varDecls"] = json::array();
    for (const auto& decl : block.varDecls) {
        j["varDecls"].push_back(emitVarDeclJson(*decl));
    }

    j["subprograms"] = json::array();
    for (const auto& sub : block.subprograms) {
        j["subprograms"].push_back(emitSubprogramJson(*sub));
    }

    if (block.body) {
        j["body"] = emitCompoundStmtJson(*block.body);
        // 主语句块
    }

    return j;
}


4.2.7 emitParseJson

函数作用：  
语法分析的顶层入口，构建完整的 AST JSON。
json emitParseJson(const ProgramNode& program) {
    json j;
    j["phase"] = "PARSE";           // 标识语法阶段
    j["status"] = "ok";             // 执行成功

    json ast;
    ast["node_type"] = "Program";   // 程序根节点
    ast["name"] = program.name;     // 程序名
    ast["loc"] = locationToJson(program.loc);

    if (program.block) {
        ast["block"] = emitBlockJson(*program.block);
        // 程序体
    }

    j["ast"] = ast;
    return j;
}


4.3 错误 JSON：emitErrorJson

4.3.1 函数职责

函数作用：  
将单个 CompilerError 包装为标准错误 JSON。
json emitErrorJson(const CompilerError& error) {
    json j;
    j["phase"] = error.stage();     // 错误所在阶段
    j["status"] = "error";          // 错误状态

    json err;
    err["phase"] = error.stage();   // 错误阶段
    err["message"] = error.what(); // 错误描述
    err["line"] = error.location().line;   // 行号
    err["column"] = error.location().column; // 列号

    j["errors"] = json::array({err});
    return j;
}

