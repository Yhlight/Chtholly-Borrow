#include "Backend/CodeGenerator.h"
#include "MIR/MIR.h"
#include <cassert>
#include <iostream>
#include <llvm/IR/Type.h>
#include <llvm/IR/DerivedTypes.h>

void testStructTypeMapping() {
    using namespace chtholly;
    
    MIRModule mirModule;
    CodeGenerator codegen(mirModule);
    
    // Create a Chtholly StructType: struct Point { x: i32, y: i32 }
    std::vector<StructType::Field> fields;
    fields.push_back({"x", Type::getI32()});
    fields.push_back({"y", Type::getI32()});
    auto pointType = std::make_shared<StructType>("Point", std::move(fields));
    
    auto mirFunc = std::make_unique<MIRFunction>("test_func", Type::getVoid());
    mirFunc->addParameter("p", pointType);
    mirModule.appendFunction(std::move(mirFunc));
    
    codegen.generate();
    
    auto& llvmModule = codegen.getLLVMModule();
    auto* func = llvmModule.getFunction("test_func");
    assert(func != nullptr);
    
    auto* paramTy = func->getFunctionType()->getParamType(0);
    
    // Rigorous check
    if (!paramTy->isStructTy()) {
        std::cerr << "Expected struct type, but got: ";
        paramTy->print(llvm::errs());
        std::cerr << std::endl;
        exit(1);
    }
    
    assert(paramTy->getStructName() == "Point");
    
    auto* structTy = llvm::cast<llvm::StructType>(paramTy);
    assert(structTy->getNumElements() == 2);
    assert(structTy->getElementType(0)->isIntegerTy(32));
    assert(structTy->getElementType(1)->isIntegerTy(32));
    
    std::cout << "testStructTypeMapping passed!" << std::endl;
}

int main() {
    testStructTypeMapping();
    return 0;
}