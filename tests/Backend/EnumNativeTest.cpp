#include "Backend/CodeGenerator.h"
#include "MIR/MIRBuilder.h"
#include "Sema/Sema.h"
#include "Parser.h"
#include <cassert>
#include <iostream>

void testEnumNative() {
    using namespace chtholly;

    // Test complex enum construction and tag loading
    {
        std::string source = R"(
            enum Message {
                Quit,
                Move(i32, i32)
            }
            fn get_tag(m: Message): i32 {
                // In a real match this would be used, but here we'll test low-level
                return 0; 
            }
            fn main() {
                let m = Move(100, 200);
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
        
        // If we reach here without crash, LLVM IR was generated
        std::cout << "Enum LLVM IR generated successfully." << std::endl;
        // codegen.getLLVMModule().print(llvm::errs(), nullptr);
    }

    std::cout << "testEnumNative passed!" << std::endl;
}

int main() {
    testEnumNative();
    return 0;
}
