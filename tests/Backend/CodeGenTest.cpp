#include "Backend/CodeGenerator.h"
#include <cassert>
#include <iostream>

void testCodeGenInit() {
    using namespace chtholly;
    
    MIRModule mirModule;
    CodeGenerator codeGen(mirModule);
    
    auto& llvmModule = codeGen.getLLVMModule();
    assert(llvmModule.getName() == "chtholly");
    
    std::cout << "testCodeGenInit passed!" << std::endl;
}

void testLowerMIR() {
    using namespace chtholly;
    
    MIRModule mirModule;
    auto mirFunc = std::make_unique<MIRFunction>("test", Type::getI32());
    auto block = std::make_unique<BasicBlock>("entry");
    
    block->appendInstruction(std::make_unique<AllocaInst>("%x", Type::getI32()));
    block->appendInstruction(std::make_unique<ConstIntInst>("%t0", 42));
    block->appendInstruction(std::make_unique<StoreInst>("%t0", "%x"));
    block->appendInstruction(std::make_unique<LoadInst>("%t1", "%x"));
    block->appendInstruction(std::make_unique<ReturnInst>("%t1"));
    
    mirFunc->appendBlock(std::move(block));
    mirModule.appendFunction(std::move(mirFunc));
    
    CodeGenerator codeGen(mirModule);
    codeGen.generate();
    
    auto& llvmModule = codeGen.getLLVMModule();
    auto* llvmFunc = llvmModule.getFunction("test");
    assert(llvmFunc != nullptr);
    
    // We can print IR for debugging
    // llvmModule.print(llvm::errs(), nullptr);
    
    std::cout << "testLowerMIR passed!" << std::endl;
}

void testBinOp() {
    using namespace chtholly;
    
    MIRModule mirModule;
    auto mirFunc = std::make_unique<MIRFunction>("add", Type::getI32());
    auto block = std::make_unique<BasicBlock>("entry");
    
    block->appendInstruction(std::make_unique<ConstIntInst>("%t0", 10));
    block->appendInstruction(std::make_unique<ConstIntInst>("%t1", 32));
    block->appendInstruction(std::make_unique<BinOpInst>("%t2", "%t0", "%t1", TokenType::Plus));
    block->appendInstruction(std::make_unique<ReturnInst>("%t2"));
    
    mirFunc->appendBlock(std::move(block));
    mirModule.appendFunction(std::move(mirFunc));
    
    CodeGenerator codeGen(mirModule);
    codeGen.generate();
    
    std::cout << "testBinOp passed!" << std::endl;
}

void testControlFlow() {
    using namespace chtholly;
    
    MIRModule mirModule;
    auto mirFunc = std::make_unique<MIRFunction>("test_cf", Type::getI32());
    
    auto entry = std::make_unique<BasicBlock>("entry");
    auto thenBlock = std::make_unique<BasicBlock>("then");
    auto elseBlock = std::make_unique<BasicBlock>("else");
    auto merge = std::make_unique<BasicBlock>("merge");
    
    entry->appendInstruction(std::make_unique<AllocaInst>("%res", Type::getI32()));
    entry->appendInstruction(std::make_unique<ConstBoolInst>("%cond", true));
    entry->appendInstruction(std::make_unique<CondBrInst>("%cond", "then", "else"));
    
    thenBlock->appendInstruction(std::make_unique<ConstIntInst>("%t0", 1));
    thenBlock->appendInstruction(std::make_unique<StoreInst>("%t0", "%res"));
    thenBlock->appendInstruction(std::make_unique<BrInst>("merge"));
    
    elseBlock->appendInstruction(std::make_unique<ConstIntInst>("%t1", 0));
    elseBlock->appendInstruction(std::make_unique<StoreInst>("%t1", "%res"));
    elseBlock->appendInstruction(std::make_unique<BrInst>("merge"));
    
    merge->appendInstruction(std::make_unique<LoadInst>("%t2", "%res"));
    merge->appendInstruction(std::make_unique<ReturnInst>("%t2"));
    
    mirFunc->appendBlock(std::move(entry));
    mirFunc->appendBlock(std::move(thenBlock));
    mirFunc->appendBlock(std::move(elseBlock));
    mirFunc->appendBlock(std::move(merge));
    
    mirModule.appendFunction(std::move(mirFunc));
    
    CodeGenerator codeGen(mirModule);
    codeGen.generate();
    
    std::cout << "testControlFlow passed!" << std::endl;
}

void testFunctionCall() {
    using namespace chtholly;
    
    MIRModule mirModule;
    
    // fn callee(): i32 { return 42; }
    auto calleeFunc = std::make_unique<MIRFunction>("callee", Type::getI32());
    auto calleeBlock = std::make_unique<BasicBlock>("entry");
    calleeBlock->appendInstruction(std::make_unique<ConstIntInst>("%t0", 42));
    calleeBlock->appendInstruction(std::make_unique<ReturnInst>("%t0"));
    calleeFunc->appendBlock(std::move(calleeBlock));
    
    // fn caller(): i32 { return callee(); }
    auto callerFunc = std::make_unique<MIRFunction>("caller", Type::getI32());
    auto callerBlock = std::make_unique<BasicBlock>("entry");
    callerBlock->appendInstruction(std::make_unique<CallInst>("%res", "callee", std::vector<std::string>{}));
    callerBlock->appendInstruction(std::make_unique<ReturnInst>("%res"));
    callerFunc->appendBlock(std::move(callerBlock));
    
    mirModule.appendFunction(std::move(calleeFunc));
    mirModule.appendFunction(std::move(callerFunc));
    
    CodeGenerator codeGen(mirModule);
    codeGen.generate();
    
    std::cout << "testFunctionCall passed!" << std::endl;
}

int main() {
    testCodeGenInit();
    testLowerMIR();
    testBinOp();
    testControlFlow();
    testFunctionCall();
    return 0;
}
