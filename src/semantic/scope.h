#pragma once

#include <cctype>
#include <string>
#include <unordered_map>

#include "semantic/symbol.h"

namespace pascal_s2c {

class Scope {
public:
    explicit Scope(const Scope* parent = nullptr) : parent_(parent) {}

    bool define(Symbol symbol) {
        return symbols_.emplace(normalize(symbol.name), std::move(symbol)).second;
    }

    const Symbol* lookup(const std::string& name) const {
        const auto it = symbols_.find(normalize(name));
        if (it != symbols_.end()) {
            return &it->second;
        }
        return parent_ ? parent_->lookup(name) : nullptr;
    }

    const Symbol* lookupLocal(const std::string& name) const {
        const auto it = symbols_.find(normalize(name));
        return it != symbols_.end() ? &it->second : nullptr;
    }

    const std::unordered_map<std::string, Symbol>& symbols() const noexcept {
        return symbols_;
    }

    const Scope* parent() const noexcept {
        return parent_;
    }

private:
    static std::string normalize(const std::string& name) {
        std::string lowered;
        lowered.reserve(name.size());
        for (const char ch : name) {
            lowered.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
        }
        return lowered;
    }

    const Scope* parent_;
    std::unordered_map<std::string, Symbol> symbols_;
};

}  // namespace pascal_s2c
