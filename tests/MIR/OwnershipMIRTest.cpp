#include "Parser.h"
#include "Sema/Sema.h"
#include "MIR/MIRBuilder.h"
#include <iostream>
#include <cassert>

using namespace chtholly;

void testOwnershipMIR() {
    std::cout << "Starting Ownership (Destructor) MIR test..." << std::endl;
    try {
        std::string source = R"(
            class Foo {
                fn ~Foo() { }
            }
            fn main() {
                let f = Foo();
                {
                    let f2 = Foo();
                }
                return;
            }
        )";
        Parser parser(source);
        auto program = parser.parseProgram();
        
        Sema sema;
        for (auto const& node : program) {
            sema.analyze(node.get());
        }

        MIRModule module;
        MIRBuilder builder(module);
        for (auto const& node : program) {
            builder.lower(node.get());
        }

        auto* mainFunc = module.getFunction("main");
        assert(mainFunc != nullptr);
        
        std::string mir = mainFunc->toString();
        std::cout << "Generated MIR:\n" << mir << std::endl;

        // Check for destructor calls
        // f2 should be dropped at end of inner block
        // f should be dropped before return
        assert(mir.find("call Foo::~Foo(%f2)") != std::string::npos);
        assert(mir.find("call Foo::~Foo(%f)") != std::string::npos);
        
        // Ensure f2 is dropped BEFORE f
        size_t posF2 = mir.find("call Foo::~Foo(%f2)");
        size_t posF = mir.find("call Foo::~Foo(%f)");
        assert(posF2 < posF);

        std::cout << "Ownership MIR test passed." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Ownership MIR test FAILED: " << e.what() << std::endl;
        exit(1);
    }
}

int main() {
    testOwnershipMIR();
    return 0;
}

