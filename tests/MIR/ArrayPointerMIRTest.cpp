#include "MIR/MIRBuilder.h"
#include "Sema/Sema.h"
#include "Parser.h"
#include "AST/Declarations.h"
#include <cassert>
#include <iostream>

void testArrayPointerMIR() {
    using namespace chtholly;

    // 1. Array Indexing
    {
        std::string source = "fn main() { let a = [1, 2]; let b = a[0]; }";
        Parser parser(source);
        auto program = parser.parseProgram();
        
        Sema sema;
        MIRModule module;
        MIRBuilder builder(module);
        for (auto& decl : program) {
            sema.analyze(decl.get());
            builder.lower(decl.get());
        }
        
        std::string mir = module.toString();
        assert(mir.find("array_gep") != std::string::npos);
        assert(mir.find("alloca i32[2]") != std::string::npos);
    }

    // 2. Address-of and Dereference
    {
        std::string source = "fn main() { let x = 10; let p = &x; let y = *p; }";
        Parser parser(source);
        auto program = parser.parseProgram();
        
        Sema sema;
        MIRModule module;
        MIRBuilder builder(module);
        for (auto& decl : program) {
            sema.analyze(decl.get());
            builder.lower(decl.get());
        }
        
        std::string mir = module.toString();
        // &x should return the alloca %x
        // let p = &x -> store %x, %p
        // let y = *p -> %t = load %p, %t2 = load %t, store %t2, %y
        assert(mir.find("store %x, %p") != std::string::npos);
    }

    // 3. Intrinsics
    {
        std::string source = "fn main() { let p = malloc[i32](10); free(p); }";
        Parser parser(source);
        auto program = parser.parseProgram();
        
        Sema sema;
        MIRModule module;
        MIRBuilder builder(module);
        for (auto& decl : program) {
            sema.analyze(decl.get());
            builder.lower(decl.get());
        }
        
        std::string mir = module.toString();
        assert(mir.find("call malloc") != std::string::npos);
        assert(mir.find("call free") != std::string::npos);
    }

    std::cout << "testArrayPointerMIR passed!" << std::endl;
}

int main() {
    testArrayPointerMIR();
    return 0;
}