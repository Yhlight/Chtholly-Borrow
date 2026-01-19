#include "Backend/CodeGenerator.h"
#include <iostream>
#include <filesystem>
#include <cstdlib>

int testNativeMain() {
    using namespace chtholly;
    
    MIRModule mirModule;
    auto mirFunc = std::make_unique<MIRFunction>("main", Type::getI32());
    auto block = std::make_unique<BasicBlock>("entry");
    
    block->appendInstruction(std::make_unique<ConstIntInst>("%t0", 42));
    block->appendInstruction(std::make_unique<ReturnInst>("%t0"));
    
    mirFunc->appendBlock(std::move(block));
    mirModule.appendFunction(std::move(mirFunc));
    
    CodeGenerator codeGen(mirModule);
    codeGen.generate();
    
    std::string objFile = "test_main.obj";
    codeGen.emitObjectFile(objFile);
    
    if (std::filesystem::exists(objFile)) {
        std::cout << "Successfully emitted " << objFile << std::endl;
        return 0;
    } else {
        std::cerr << "Failed to emit " << objFile << std::endl;
        return 1;
    }
}

void testHelloWorld() {
    using namespace chtholly;
    
    MIRModule mirModule;
    
    // extern fn printf(fmt: i8*, ...): i32
    auto printfFunc = std::make_unique<MIRFunction>("printf", Type::getI32());
    printfFunc->addParameter("fmt", Type::getI8Ptr());
    printfFunc->setVarArg(true);
    mirModule.appendFunction(std::move(printfFunc));
    
    // fn main(): i32 { printf("Hello, Chtholly!\n"); return 0; }
    auto mainFunc = std::make_unique<MIRFunction>("main", Type::getI32());
    auto block = std::make_unique<BasicBlock>("entry");
    
    block->appendInstruction(std::make_unique<ConstStringInst>("%fmt", "Hello, Chtholly!\n"));
    block->appendInstruction(std::make_unique<CallInst>("", "printf", std::vector<std::string>{"%fmt"}));
    block->appendInstruction(std::make_unique<ConstIntInst>("%t0", 0));
    block->appendInstruction(std::make_unique<ReturnInst>("%t0"));
    
    mainFunc->appendBlock(std::move(block));
    mirModule.appendFunction(std::move(mainFunc));
    
    CodeGenerator codeGen(mirModule);
    codeGen.generate();
    
    std::string objFile = "hello.obj";
    codeGen.emitObjectFile(objFile);
    
    if (std::filesystem::exists(objFile)) {
        std::cout << "Successfully emitted " << objFile << std::endl;
    } else {
        std::cerr << "Failed to emit " << objFile << std::endl;
    }
}

int main() {
    if (testNativeMain() != 0) return 1;
    testHelloWorld();
    return 0;
}