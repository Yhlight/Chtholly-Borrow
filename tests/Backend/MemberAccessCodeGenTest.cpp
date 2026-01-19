#include "Backend/CodeGenerator.h"
#include "MIR/MIR.h"
#include <cassert>
#include <iostream>
#include <llvm/IR/Instructions.h>

void testMemberAccessCodeGen() {
    using namespace chtholly;
    
    MIRModule mirModule;
    CodeGenerator codegen(mirModule);
    
    // 1. Setup struct Point { x: i32, y: i32 }
    std::vector<StructType::Field> fields;
    fields.push_back({"x", Type::getI32()});
    fields.push_back({"y", Type::getI32()});
    auto pointType = std::make_shared<StructType>("Point", std::move(fields));
    
    // 2. Create MIR:
    // fn test(): i32 {
    // entry:
    //   %p = alloca Point
    //   %t0 = gep %p (Point), y
    //   %t1 = load %t0
    //   ret %t1
    // }
    auto mirFunc = std::make_unique<MIRFunction>("test", Type::getI32());
    auto entry = std::make_unique<BasicBlock>("entry");
    
    entry->appendInstruction(std::make_unique<AllocaInst>("%p", pointType));
    entry->appendInstruction(std::make_unique<StructElementPtrInst>("%t0", "%p", "Point", "y"));
    entry->appendInstruction(std::make_unique<LoadInst>("%t1", "%t0"));
    entry->appendInstruction(std::make_unique<ReturnInst>("%t1"));
    
    mirFunc->appendBlock(std::move(entry));
    mirModule.appendFunction(std::move(mirFunc));
    
    // 3. Generate LLVM IR
    codegen.generate();
    
    auto& llvmModule = codegen.getLLVMModule();
    auto* func = llvmModule.getFunction("test");
    assert(func != nullptr);
    
    // 4. Verify IR
    // We expect a GEP with index 1 for field 'y'
    bool foundGep = false;
    for (auto& bb : *func) {
        for (auto& inst : bb) {
            if (auto* gep = llvm::dyn_cast<llvm::GetElementPtrInst>(&inst)) {
                foundGep = true;
                assert(gep->getNumIndices() == 2); // 0 and 1
                auto* index = llvm::dyn_cast<llvm::ConstantInt>(gep->getOperand(2));
                assert(index != nullptr);
                assert(index->getZExtValue() == 1);
            }
        }
    }
    assert(foundGep);
    
    std::cout << "testMemberAccessCodeGen passed!" << std::endl;
}

int main() {
    testMemberAccessCodeGen();
    return 0;
}
