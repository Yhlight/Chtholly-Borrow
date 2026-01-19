#include "Backend/CodeGenerator.h"
#include "MIR/MIR.h"
#include <cassert>
#include <iostream>
#include <llvm/IR/Instructions.h>

void testArrayNative() {
    using namespace chtholly;
    
    MIRModule mirModule;
    CodeGenerator codegen(mirModule);
    
    // fn test(): i32
    // %a = alloca i32[10]
    // %t0 = const 1
    // %t1 = array_gep %a, %t0 (i32)
    // %t2 = load %t1
    // ret %t2
    auto mirFunc = std::make_unique<MIRFunction>("test_array", Type::getI32());
    auto entry = std::make_unique<BasicBlock>("entry");
    
    auto arrayType = std::make_shared<ArrayType>(Type::getI32(), 10);
    entry->appendInstruction(std::make_unique<AllocaInst>("%a", arrayType));
    entry->appendInstruction(std::make_unique<ConstIntInst>("%t0", 1));
    entry->appendInstruction(std::make_unique<ArrayElementPtrInst>("%t1", "%a", "%t0", Type::getI32()));
    entry->appendInstruction(std::make_unique<LoadInst>("%t2", "%t1"));
    entry->appendInstruction(std::make_unique<ReturnInst>("%t2"));
    
    mirFunc->appendBlock(std::move(entry));
    mirModule.appendFunction(std::move(mirFunc));
    
    codegen.generate();
    
    auto& llvmModule = codegen.getLLVMModule();
    auto* func = llvmModule.getFunction("test_array");
    assert(func != nullptr);
    
    bool foundGep = false;
    for (auto& bb : *func) {
        for (auto& inst : bb) {
            if (auto* gep = llvm::dyn_cast<llvm::GetElementPtrInst>(&inst)) {
                foundGep = true;
                // For array indexing on a pointer (alloca returns T*), GEP usually has 1 index.
                // If it's a pointer to an array (alloca T[N]), it might have 2 indices [0, index].
                // In our CodeGen, we use builder->CreateGEP(elemTy, ptr, index), which is 1 index GEP.
                assert(gep->getNumIndices() == 1); 
            }
        }
    }
    assert(foundGep);
    std::cout << "testArrayNative passed!" << std::endl;
}

void testMallocNative() {
    using namespace chtholly;
    
    MIRModule mirModule;
    CodeGenerator codegen(mirModule);
    
    // extern fn malloc(size: i32): i8*
    auto mallocFunc = std::make_unique<MIRFunction>("malloc", Type::getI8Ptr());
    mallocFunc->addParameter("size", Type::getI32());
    mirModule.appendFunction(std::move(mallocFunc));

    // extern fn free(ptr: i8*)
    auto freeFunc = std::make_unique<MIRFunction>("free", Type::getVoid());
    freeFunc->addParameter("ptr", Type::getI8Ptr());
    mirModule.appendFunction(std::move(freeFunc));

    auto mainFunc = std::make_unique<MIRFunction>("test_malloc", Type::getVoid());
    auto entry = std::make_unique<BasicBlock>("entry");
    
    entry->appendInstruction(std::make_unique<ConstIntInst>("%size", 40));
    entry->appendInstruction(std::make_unique<CallInst>("%p", "malloc", std::vector<std::string>{"%size"}));
    entry->appendInstruction(std::make_unique<CallInst>("", "free", std::vector<std::string>{"%p"}));
    entry->appendInstruction(std::make_unique<ReturnInst>());
    
    mainFunc->appendBlock(std::move(entry));
    mirModule.appendFunction(std::move(mainFunc));
    
    codegen.generate();
    
    auto& llvmModule = codegen.getLLVMModule();
    assert(llvmModule.getFunction("malloc") != nullptr);
    assert(llvmModule.getFunction("free") != nullptr);
    
    std::cout << "testMallocNative passed!" << std::endl;
}

int main() {
    testArrayNative();
    testMallocNative();
    return 0;
}
