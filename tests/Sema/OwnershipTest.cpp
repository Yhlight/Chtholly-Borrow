#include "Sema/Sema.h"
#include "Parser.h"
#include "AST/Declarations.h"
#include <cassert>
#include <iostream>

void testOwnership() {
    using namespace chtholly;

    try {
        // 1. Move on Assignment (Non-Copy Type)
        {
            fprintf(stderr, "Starting Test 1 (Move on Assignment)...\n");
            std::string source = R"(
                struct Data { x: i32 }
                fn main() {
                    let a = Data { x: 10 };
                    let b = a;
                    let c = a; // Error: use of moved variable
                }
            )";
            Parser parser(source);
            auto program = parser.parseProgram();
            
            Sema sema;
            sema.analyze(program[0].get()); // struct Data
            assert(program[1]->getKind() == ASTNodeKind::FunctionDecl);
            auto mainFunc = static_cast<FunctionDecl*>(program[1].get());
            
            try {
                sema.analyze(mainFunc);
                assert(false && "Should have thrown use-of-moved-variable error");
            } catch (const std::exception& e) {
                fprintf(stderr, "Caught expected error: %s\n", e.what());
                assert(std::string(e.what()).find("moved") != std::string::npos);
            }
            fprintf(stderr, "Test 1 passed.\n");
        }

        // 2. Copy on Assignment (Primitive Type)
        {
            fprintf(stderr, "Starting Test 2 (Primitive Copy)...\n");
            std::string source = R"(
                fn main() {
                    let a = 10;
                    let b = a;
                    let c = a; // OK: i32 is Copy
                }
            )";
            Parser parser(source);
            auto program = parser.parseProgram();
            
            Sema sema;
            for (auto& decl : program) {
                sema.analyze(decl.get());
            }
            fprintf(stderr, "Test 2 passed.\n");
        }

        // 3. Re-initialization after Move
        {
            fprintf(stderr, "Starting Test 3 (Re-initialization)...\n");
            std::string source = R"(
                struct Data { x: i32 }
                fn main() {
                    let mut a = Data { x: 10 };
                    let b = a;
                    a = Data { x: 20 };
                    let c = a; // OK: re-initialized
                }
            )";
            Parser parser(source);
            auto program = parser.parseProgram();
            
            Sema sema;
            sema.analyze(program[0].get());
            sema.analyze(program[1].get());
            fprintf(stderr, "Test 3 passed.\n");
        }
    } catch (const std::exception& e) {
        fprintf(stderr, "Unexpected exception: %s\n", e.what());
        exit(1);
    }

    std::cout << "testOwnership passed!" << std::endl;
}

int main() {
    testOwnership();
    return 0;
}
