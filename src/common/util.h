#pragma once

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace pascal_s2c {

inline std::string readTextFile(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("failed to open input file: " + path);
    }
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

}  // namespace pascal_s2c
