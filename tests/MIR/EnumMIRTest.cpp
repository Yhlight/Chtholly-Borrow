#include "MIR/MIR.h"
#include "MIR/MIRBuilder.h"
#include "Sema/Sema.h"
#include "Parser.h"
#include <cassert>
#include <iostream>

void testEnumMIR() {
    using namespace chtholly;

    try {
        // 1. Enum Variant construction MIR
        {
            fprintf(stderr, "Starting Enum Variant construction MIR test...\n");
            std::string source = R"(
                enum Message {
                    Quit,
                    Move(i32, i32)
                }
                fn main() {
                    let q = Quit;
                    let m = Move(10, 20);
                }
            )";
            Parser parser(source);
            auto program = parser.parseProgram();
            fprintf(stderr, "Parsed program, nodes: %zu\n", program.size());
            
            Sema sema;
            for (auto const& node : program) {
                sema.analyze(node.get());
            }
            fprintf(stderr, "Semantic analysis complete.\n");

            MIRModule module;
            MIRBuilder builder(module);
            for (auto const& node : program) {
                builder.lower(node.get());
            }
            fprintf(stderr, "Lowering complete.\n");

            auto* mainFunc = module.getFunction("main");
            assert(mainFunc != nullptr);
            
            std::string mir = mainFunc->toString();
            fprintf(stderr, "Generated MIR:\n%s\n", mir.c_str());

            // Check for variant_data instructions
            assert(mir.find("variant_data") != std::string::npos);
            assert(mir.find("tag 0") != std::string::npos); // Quit
            assert(mir.find("tag 1") != std::string::npos); // Move
            fprintf(stderr, "Enum Variant construction MIR test passed.\n");
        }
    } catch (const std::exception& e) {
        fprintf(stderr, "Unexpected exception in EnumMIRTest: %s\n", e.what());
        exit(1);
    }

    // 2. Test Enum Switch matching
    {
        std::cout << "Starting Enum Switch matching MIR test..." << std::endl;
        try {
            std::string source = R"(
                extern fn printf(fmt: i8*, ...): i32;
                enum Color { Red, Green, Blue }
                fn main() {
                    let c = Color::Green;
                    switch (c) {
                        case Color::Red: { printf("red"); }
                        case Color::Green: { printf("green"); }
                        case _: { printf("other"); }
                    }
                }
            )";
            std::cout << "Parsing..." << std::endl;
            Parser parser(source);
            auto program = parser.parseProgram();
            std::cout << "Parsed program, nodes: " << program.size() << std::endl;

            std::cout << "Analyzing..." << std::endl;
            Sema sema;
            for (const auto& node : program) sema.analyze(node.get());
            std::cout << "Semantic analysis complete." << std::endl;

            std::cout << "Lowering..." << std::endl;
            MIRModule module;
            MIRBuilder builder(module);
            for (const auto& node : program) builder.lower(node.get());
            std::cout << "Lowering complete." << std::endl;

            std::string mir = module.toString();
            std::cout << "Generated MIR:\n" << mir << std::endl;

            assert(mir.find("variant_tag") != std::string::npos);
            assert(mir.find("case.body") != std::string::npos);
            std::cout << "Enum Switch matching MIR test passed." << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Enum Switch matching MIR test FAILED: " << e.what() << std::endl;
            throw;
        }
    }

    std::cout << "testEnumMIR passed!" << std::endl;
}

int main() {
    testEnumMIR();
    return 0;
}

