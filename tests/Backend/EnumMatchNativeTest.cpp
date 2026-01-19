#include "Backend/CodeGenerator.h"
#include "MIR/MIRBuilder.h"
#include "Sema/Sema.h"
#include "Parser.h"
#include <cassert>
#include <iostream>
#include <filesystem>

using namespace chtholly;

void testEnumMatchNative() {
    std::cout << "Starting Enum Match Native test..." << std::endl;
    try {
        std::string source = R"(
            extern fn printf(fmt: i8*, ...): i32;
            enum Color {
                Red,
                Green,
                Blue(i32, i32)
            }
            fn main() {
                let c = Color::Blue(10, 20);
                switch (c) {
                    case Color::Red: { printf("red\n"); }
                    case Color::Green: { printf("green\n"); }
                    case Color::Blue(x, y): { printf("blue %d %d\n", x, y); }
                }
            }
        )";
        Parser parser(source);
        auto program = parser.parseProgram();
        
        Sema sema;
        for (auto const& node : program) sema.analyze(node.get());

        MIRModule module;
        MIRBuilder builder(module);
        for (auto const& node : program) builder.lower(node.get());

        CodeGenerator codegen(module);
        codegen.generate();
        
        std::cout << "Enum Match LLVM IR generated successfully." << std::endl;
        codegen.emitObjectFile("enum_match.obj");
        if (std::filesystem::exists("enum_match.obj")) {
            std::cout << "Successfully emitted enum_match.obj" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Enum Match Native test FAILED: " << e.what() << std::endl;
        exit(1);
    }
}

int main() {
    testEnumMatchNative();
    return 0;
}

