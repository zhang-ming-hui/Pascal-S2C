#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>

#include "common/error.h"
#include "driver/compiler.h"

namespace {

enum class OutputMode {
    Code,
    Lexer
};

struct CliOptions {
    std::string inputPath;
    std::optional<std::string> outputPath;
    OutputMode mode = OutputMode::Code;
};

void printUsage() {
    std::cerr << "usage: pascal_s2c <input.pas> [output.c]\n";
    std::cerr << "   or: pascal_s2c -i <input.pas> [-o <output.c>]\n";
    std::cerr << "   or: pascal_s2c --lexer <input.pas> [output.tokens]\n";
    std::cerr << "   or: pascal_s2c --lexer -i <input.pas> [-o <output.tokens>]\n";
}

std::optional<CliOptions> parseArgs(int argc, char** argv) {
    if (argc < 2) {
        return std::nullopt;
    }

    CliOptions options;
    bool sawNamedPathFlag = false;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--lexer") {
            options.mode = OutputMode::Lexer;
            continue;
        }
        if (arg == "-i" || arg == "--input") {
            sawNamedPathFlag = true;
            if (i + 1 >= argc) {
                return std::nullopt;
            }
            options.inputPath = argv[++i];
            continue;
        }
        if (arg == "-o" || arg == "--output") {
            sawNamedPathFlag = true;
            if (i + 1 >= argc) {
                return std::nullopt;
            }
            options.outputPath = std::string(argv[++i]);
            continue;
        }
        if (!arg.empty() && arg[0] == '-') {
            return std::nullopt;
        }

        if (sawNamedPathFlag) {
            return std::nullopt;
        }

        if (options.inputPath.empty()) {
            options.inputPath = arg;
        } else if (!options.outputPath.has_value()) {
            options.outputPath = arg;
        } else {
            return std::nullopt;
        }
    }

    if (options.inputPath.empty()) {
        return std::nullopt;
    }
    return options;
}

std::string inferOutputPath(const std::string& inputPath, OutputMode mode) {
    std::filesystem::path path(inputPath);
    path.replace_extension(mode == OutputMode::Lexer ? ".tokens" : ".c");
    return path.string();
}

}  // namespace

int main(int argc, char** argv) {
    const std::optional<CliOptions> options = parseArgs(argc, argv);
    if (!options.has_value()) {
        printUsage();
        return 1;
    }

    try {
        pascal_s2c::Compiler compiler;
        const std::string output = options->mode == OutputMode::Lexer
                                       ? compiler.lexFile(options->inputPath)
                                       : compiler.compileFile(options->inputPath);
        const std::string outputPath =
            options->outputPath.value_or(inferOutputPath(options->inputPath, options->mode));

        std::ofstream out(outputPath, std::ios::binary);
        if (!out) {
            std::cerr << "failed to open output file: " << outputPath << '\n';
            return 1;
        }
        out << output;
    } catch (const pascal_s2c::CompilerError& error) {
        std::cerr << pascal_s2c::formatError(error) << '\n';
        return 1;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }

    return 0;
}
