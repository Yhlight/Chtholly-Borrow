#include "MIR/MIR.h"
#include <cassert>
#include <iostream>

void testMIRCore() {
    using namespace chtholly;
    
    auto block = std::make_unique<BasicBlock>("entry");
    assert(block->getName() == "entry");
    
    // Test Instruction
    block->appendInstruction(std::make_unique<AllocaInst>("x", Type::getI32()));
    assert(block->getInstructions().size() == 1);
    assert(block->getInstructions()[0]->toString() == "x = alloca i32");
    
    std::cout << "testMIRCore passed!" << std::endl;
}

void testMIRFunction() {
    using namespace chtholly;
    auto func = std::make_unique<MIRFunction>("main", Type::getVoid());
    assert(func->getName() == "main");
    
    auto block = std::make_unique<BasicBlock>("entry");
    func->appendBlock(std::move(block));
    assert(func->getBlocks().size() == 1);
    
    std::cout << "testMIRFunction passed!" << std::endl;
}

void testMIRModule() {
    using namespace chtholly;
    MIRModule module;
    auto func = std::make_unique<MIRFunction>("test", Type::getVoid());
    module.appendFunction(std::move(func));
    assert(module.getFunctions().size() == 1);
    
    std::cout << "testMIRModule passed!" << std::endl;
}

int main() {
    testMIRCore();
    testMIRFunction();
    testMIRModule();
    return 0;
}
