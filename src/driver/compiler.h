#pragma once

#include <string>

namespace pascal_s2c {

class Compiler {
public:
    std::string compileSource(const std::string& source) const;
    std::string compileFile(const std::string& path) const;
};

}  // namespace pascal_s2c
