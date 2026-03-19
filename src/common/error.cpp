#include "common/error.h"

#include <sstream>

namespace pascal_s2c {

CompilerError::CompilerError(std::string stage, std::string message, SourceLocation location)
    : std::runtime_error(std::move(message)), stage_(std::move(stage)), location_(location) {}

const std::string& CompilerError::stage() const noexcept {
    return stage_;
}

const SourceLocation& CompilerError::location() const noexcept {
    return location_;
}

std::string formatError(const CompilerError& error) {
    std::ostringstream oss;
    oss << error.stage() << " error"
        << " (" << error.location().line << ":" << error.location().column << "): "
        << error.what();
    return oss.str();
}

}  // namespace pascal_s2c
