#include <exception>
#include <fstream>
#include <iostream>
#include <string>

#include "common/error.h"
#include "driver/compiler.h"

int main(int argc, char** argv) {
    if (argc < 2 || argc > 3) {
        std::cerr << "usage: pascal_s2c <input.pas> [output.c]\n";
        return 1;
    }

    const std::string inputPath = argv[1];

    try {
        pascal_s2c::Compiler compiler;
        const std::string output = compiler.compileFile(inputPath);

        if (argc == 3) {
            const std::string outputPath = argv[2];
            std::ofstream out(outputPath, std::ios::binary);
            if (!out) {
                std::cerr << "failed to open output file: " << outputPath << '\n';
                return 1;
            }
            out << output;
        } else {
            std::cout << output;
        }
    } catch (const pascal_s2c::CompilerError& error) {
        std::cerr << pascal_s2c::formatError(error) << '\n';
        return 1;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }

    return 0;
}
