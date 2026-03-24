#pragma once

#include <cctype>
#include <string>
#include <unordered_map>

#include "semantic/symbol.h"

namespace pascal_s2c {

// 作用域与符号表。
// 使用位置：
// 1) SemanticAnalyzer 在分析 block 时创建层级 Scope。
// 2) 名称查找沿 parent 链向上递归。
// 3) 语义阶段结束后，Scope 数据保存在 SemanticContext.ownedScopes。
class Scope {
public:
    // parent 为空表示全局作用域；否则表示嵌套作用域。
    explicit Scope(const Scope* parent = nullptr) : parent_(parent) {}

    // 在当前作用域定义符号；若同名已存在则返回 false。
    // 名称会先 normalize（大小写不敏感）。
    bool define(Symbol symbol) {
        return symbols_.emplace(normalize(symbol.name), std::move(symbol)).second;
    }

    // 从当前作用域向父作用域链递归查找。
    // 常用于标识符解析（变量、函数、过程）。
    const Symbol* lookup(const std::string& name) const {
        const auto it = symbols_.find(normalize(name));
        if (it != symbols_.end()) {
            return &it->second;
        }
        return parent_ ? parent_->lookup(name) : nullptr;
    }

    // 仅在当前作用域查找，不访问父作用域。
    // 常用于重复定义检查。
    const Symbol* lookupLocal(const std::string& name) const {
        const auto it = symbols_.find(normalize(name));
        return it != symbols_.end() ? &it->second : nullptr;
    }

    // 返回当前作用域符号集合只读视图。
    const std::unordered_map<std::string, Symbol>& symbols() const noexcept {
        return symbols_;
    }

    // 返回父作用域指针。
    const Scope* parent() const noexcept {
        return parent_;
    }

private:
    // Pascal 名称按大小写不敏感处理，统一转小写作为 key。
    static std::string normalize(const std::string& name) {
        std::string lowered;
        lowered.reserve(name.size());
        for (const char ch : name) {
            lowered.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
        }
        return lowered;
    }

    // 父作用域引用（不拥有）。
    const Scope* parent_;
    // 当前作用域的符号表（key 为归一化后的名字）。
    std::unordered_map<std::string, Symbol> symbols_;
};

}  // namespace pascal_s2c
