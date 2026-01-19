#include "Parser.h"
#include "Sema/Sema.h"
#include "MIR/MIRBuilder.h"
#include "Backend/CodeGenerator.h"
#include <iostream>
#include <filesystem>
#include <cstdlib>
#include <vector>

int main() {
    using namespace chtholly;
    
    std::string source = R"(
        struct Point {
            let x: i32;
            let y: i32;
        }

        fn main(): i32 {
            let p = Point { x: 10, y: 20 };
            return p.y;
        }
    )";
    
    try {
        Parser parser(source);
        Sema sema;
        std::vector<std::unique_ptr<ASTNode>> nodes;

        while (true) {
            try {
                auto stmt = parser.parseStatement();
                if (stmt) {
                    nodes.push_back(std::move(stmt));
                }
            } catch (const std::exception& e) {
                break; // Assume end of source
            }
        }

        for (auto& node : nodes) {
            sema.analyze(node.get());
        }
        
        MIRModule mirModule;
        MIRBuilder mirBuilder(mirModule);
        for (auto& node : nodes) {
            mirBuilder.lower(node.get());
        }
        
        CodeGenerator codeGen(mirModule);
        codeGen.generate();
        
        std::string objFile = "struct_test.obj";
        codeGen.emitObjectFile(objFile);
        
        if (!std::filesystem::exists(objFile)) {
            std::cerr << "Failed to emit object file" << std::endl;
            return 1;
        }
        
        std::cout << "Successfully emitted " << objFile << std::endl;
        std::filesystem::remove(objFile);
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}