#ifndef CHTHOLLY_CODEGENERATOR_H
#define CHTHOLLY_CODEGENERATOR_H

#include "MIR/MIR.h"
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <map>

namespace chtholly {

class CodeGenerator {
public:
    CodeGenerator(MIRModule& mirModule);

    void generate();
    void emitObjectFile(const std::string& filename);
    
    llvm::Value* getOrCreateGlobalString(const std::string& str);

    llvm::Module& getLLVMModule() { return *llvmModule; }

private:
    void generateFunction(MIRFunction* mirFunc);
    llvm::Type* getLLVMType(Type* chthollyType);

    MIRModule& mirModule;
    std::unique_ptr<llvm::LLVMContext> context;
    std::unique_ptr<llvm::Module> llvmModule;
    std::unique_ptr<llvm::IRBuilder<>> builder;
    
    std::map<std::string, llvm::Value*> valueMap;
    std::map<std::string, std::shared_ptr<Type>> mirTypeMap;
    std::map<std::string, llvm::StructType*> structMap;
    std::map<std::string, std::shared_ptr<StructType>> structDefMap;
    std::map<std::string, llvm::StructType*> enumMap;
    std::map<std::string, std::shared_ptr<EnumType>> enumDefMap;
    std::map<std::string, llvm::Value*> globalStrings;
};

} // namespace chtholly

#endif // CHTHOLLY_CODEGENERATOR_H
