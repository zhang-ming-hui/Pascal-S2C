#pragma once

#include <sstream>
#include <string>

namespace pascal_s2c {

class CWriter {
public:
    void indent();
    void dedent();
    void writeLine(const std::string& line = "");
    void writeRaw(const std::string& text);
    std::string str() const;

private:
    std::string indentText() const;

    std::ostringstream out_;
    int indentLevel_ = 0;
};

}  // namespace pascal_s2c
