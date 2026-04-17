#pragma once

#include <stdexcept>
#include <string>

#include "common/location.h"

namespace pascal_s2c {

class CompilerError : public std::runtime_error {
public:
    CompilerError(std::string stage, std::string message, SourceLocation location = {});

    const std::string& stage() const noexcept;
    const SourceLocation& location() const noexcept;

private:
    std::string stage_;
    SourceLocation location_;
};

std::string formatError(const CompilerError& error);

}  // namespace pascal_s2c
