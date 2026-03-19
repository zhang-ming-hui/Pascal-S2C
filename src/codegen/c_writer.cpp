#include "codegen/c_writer.h"

#include <stdexcept>

namespace pascal_s2c {

void CWriter::indent() {
    ++indentLevel_;
}

void CWriter::dedent() {
    if (indentLevel_ == 0) {
        throw std::runtime_error("cannot dedent below zero");
    }
    --indentLevel_;
}

void CWriter::writeLine(const std::string& line) {
    out_ << indentText() << line << '\n';
}

void CWriter::writeRaw(const std::string& text) {
    out_ << text;
}

std::string CWriter::str() const {
    return out_.str();
}

std::string CWriter::indentText() const {
    return std::string(static_cast<std::size_t>(indentLevel_) * 2U, ' ');
}

}  // namespace pascal_s2c
