#include "MIR/MIRBuilder.h"
#include "Parser.h"
#include <cassert>
#include <iostream>

void testLowerFunction() {
    using namespace chtholly;
    // Source: fn test(): i32 { let x: i32 = 1 + 2; return x; }
    std::string source = "fn test(): i32 { let x: i32 = 1 + 2; return x; }";
    Parser parser(source);
    auto decl = parser.parseFunctionDecl();
    
    MIRModule module;
    MIRBuilder builder(module);
    builder.lower(decl.get());
    
    auto& functions = module.getFunctions();
    assert(functions.size() == 1);
    assert(functions[0]->getName() == "test");
    
    auto& blocks = functions[0]->getBlocks();
    assert(blocks.size() == 1);
    assert(blocks[0]->getName() == "entry");
    
    // Instructions should be:
    // %x = alloca i32
    // %t0 = const 1
    // %t1 = const 2
    // %t2 = add %t0, %t1
    // store %t2, %x
    // %t3 = load %x
    // ret %t3
    
    auto& insts = blocks[0]->getInstructions();
    assert(insts.size() == 7);
    assert(insts[0]->toString() == "%x = alloca i32");
    assert(insts[1]->toString() == "%t0 = const 1");
    assert(insts[2]->toString() == "%t1 = const 2");
    assert(insts[3]->toString() == "%t2 = add %t0, %t1");
    assert(insts[4]->toString() == "store %t2, %x");
    assert(insts[5]->toString() == "%t3 = load %x");
        assert(insts[6]->toString() == "ret %t3");
    
        std::cout << "testLowerFunction passed!" << std::endl;
    }
    
    void testLowerIfStmt() {
        using namespace chtholly;
        // Source: fn test(cond: bool) { if cond { let x = 1; } else { let x = 2; } }
        std::string source = "fn test(cond: bool) { if cond { let x: i32 = 1; } else { let x: i32 = 2; } }";
        Parser parser(source);
        auto decl = parser.parseFunctionDecl();
        
        MIRModule module;
        MIRBuilder builder(module);
        builder.lower(decl.get());
        
        auto& functions = module.getFunctions();
        assert(functions.size() == 1);
        
        auto& blocks = functions[0]->getBlocks();
        // entry, then, else, merge
        assert(blocks.size() == 4);
        
            std::cout << "testLowerIfStmt passed!" << std::endl;
        }
        
        void testLowerWhileStmt() {
            using namespace chtholly;
            // Source: fn test(cond: bool) { while cond { let x = 1; } }
            std::string source = "fn test(cond: bool) { while cond { let x: i32 = 1; } }";
            Parser parser(source);
            auto decl = parser.parseFunctionDecl();
            
            MIRModule module;
            MIRBuilder builder(module);
            builder.lower(decl.get());
            
            auto& functions = module.getFunctions();
            assert(functions.size() == 1);
            
            auto& blocks = functions[0]->getBlocks();
            // entry, cond, body, merge
            assert(blocks.size() == 4);
            
            std::cout << "testLowerWhileStmt passed!" << std::endl;
        }
        
        int main() {
            testLowerFunction();
            testLowerIfStmt();
            testLowerWhileStmt();
            return 0;
        }
        