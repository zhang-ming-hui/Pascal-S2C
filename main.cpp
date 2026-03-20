#include <iostream>
#include <fstream>
#include <string>
#include <cstring>

void printUsage() {
    std::cout << "Usage: pascc.exe -i <filename>" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -i <filename>    Display the content of the specified file" << std::endl;
    std::cout << "  -h, --help       Display this help message" << std::endl;
}

bool displayFileContent(const std::string& filename) {
    std::ifstream file(filename);
    
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file '" << filename << "'" << std::endl;
        return false;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        std::cout << line << std::endl;
    }
    
    file.close();
    return true;
}

int main(int argc, char* argv[]) {
    // 检查参数数量
    if (argc < 2) {
        printUsage();
        return 1;
    }
    
    // 处理参数
    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        printUsage();
        return 0;
    }
    
    if (strcmp(argv[1], "-i") == 0) {
        if (argc < 3) {
            std::cerr << "Error: Missing filename after -i option" << std::endl;
            printUsage();
            return 1;
        }
        
        std::string filename = argv[2];
        if (!displayFileContent(filename)) {
            return 1;
        }
    } else {
        std::cerr << "Error: Unknown option '" << argv[1] << "'" << std::endl;
        printUsage();
        return 1;
    }
    
    return 0;
}