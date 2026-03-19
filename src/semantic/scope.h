#pragma once

#include <string>
#include <unordered_map>

#include "semantic/symbol.h"

namespace pascal_s2c {

class Scope {
public:
    explicit Scope(const Scope* parent = nullptr) : parent_(parent) {}

    bool define(Symbol symbol) {
        return symbols_.emplace(symbol.name, std::move(symbol)).second;
    }

    const Symbol* lookup(const std::string& name) const {
        auto it = symbols_.find(name);
        if (it != symbols_.end()) {
            return &it->second;
        }
        return parent_ ? parent_->lookup(name) : nullptr;
    }

    const Symbol* lookupLocal(const std::string& name) const {
        auto it = symbols_.find(name);
        return it != symbols_.end() ? &it->second : nullptr;
    }

    const std::unordered_map<std::string, Symbol>& symbols() const noexcept {
        return symbols_;
    }

    const Scope* parent() const noexcept {
        return parent_;
    }

private:
    const Scope* parent_;
    std::unordered_map<std::string, Symbol> symbols_;
};

}  // namespace pascal_s2c
