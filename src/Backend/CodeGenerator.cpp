#include "Backend/CodeGenerator.h"
#include <llvm/IR/Verifier.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/raw_ostream.h>
#include <iostream>

namespace chtholly {

CodeGenerator::CodeGenerator(MIRModule& mirModule) : mirModule(mirModule) {
    context = std::make_unique<llvm::LLVMContext>();
    llvmModule = std::make_unique<llvm::Module>("chtholly", *context);
    builder = std::make_unique<llvm::IRBuilder<>>(*context);
}

void CodeGenerator::generate() {
    // 1. Declare standard functions if used but not defined
    bool usesMalloc = false;
    bool usesFree = false;
    for (auto& func : mirModule.getFunctions()) {
        for (auto& block : func->getBlocks()) {
            for (auto& inst : block->getInstructions()) {
                if (inst->getKind() == MIRInstructionKind::Call) {
                    auto* call = static_cast<CallInst*>(inst.get());
                    if (call->getCallee() == "malloc") usesMalloc = true;
                    if (call->getCallee() == "free") usesFree = true;
                }
            }
        }
    }

    if (usesMalloc && !llvmModule->getFunction("malloc")) {
        llvm::FunctionType* mallocTy = llvm::FunctionType::get(
            llvm::PointerType::get(*context, 0),
            {llvm::Type::getInt64Ty(*context)},
            false
        );
        llvm::Function::Create(mallocTy, llvm::Function::ExternalLinkage, "malloc", *llvmModule);
    }
    if (usesFree && !llvmModule->getFunction("free")) {
        llvm::FunctionType* freeTy = llvm::FunctionType::get(
            llvm::Type::getVoidTy(*context),
            {llvm::PointerType::get(*context, 0)},
            false
        );
        llvm::Function::Create(freeTy, llvm::Function::ExternalLinkage, "free", *llvmModule);
    }

    for (auto& func : mirModule.getFunctions()) {
        std::vector<llvm::Type*> paramTypes;
        for (const auto& param : func->getParameters()) {
            paramTypes.push_back(getLLVMType(param.second.get()));
        }

        llvm::FunctionType* funcType = llvm::FunctionType::get(
            getLLVMType(func->getReturnType().get()), 
            paramTypes, 
            func->getVarArg()
        );

        llvm::Function::Create(
            funcType, 
            llvm::Function::ExternalLinkage, 
            func->getName(), 
            *llvmModule
        );
    }

    for (auto& func : mirModule.getFunctions()) {
        generateFunction(func.get());
    }
}

void CodeGenerator::generateFunction(MIRFunction* mirFunc) {
    llvm::Function* func = llvmModule->getFunction(mirFunc->getName());
    if (!func || mirFunc->getBlocks().empty()) {
        return;
    }

    std::map<std::string, llvm::BasicBlock*> blockMap;
    for (auto& mirBlock : mirFunc->getBlocks()) {
        if (mirBlock->getInstructions().empty()) continue; // Skip empty blocks
        blockMap[mirBlock->getName()] = llvm::BasicBlock::Create(*context, mirBlock->getName(), func);
    }

    valueMap.clear();
    mirTypeMap.clear();

    // Add parameters to valueMap
    size_t argIdx = 0;
    for (auto& arg : func->args()) {
        std::string argName = "%" + mirFunc->getParameters()[argIdx].first;
        valueMap[argName] = &arg;
        mirTypeMap[argName] = mirFunc->getParameters()[argIdx].second;
        argIdx++;
    }

    for (auto& mirBlock : mirFunc->getBlocks()) {
        if (blockMap.find(mirBlock->getName()) == blockMap.end()) continue;
        builder->SetInsertPoint(blockMap[mirBlock->getName()]);
        
        for (auto& inst : mirBlock->getInstructions()) {
            switch (inst->getKind()) {
                case MIRInstructionKind::Alloca: {
                    auto* allocaInst = static_cast<AllocaInst*>(inst.get());
                    auto* type = getLLVMType(allocaInst->getType().get());
                    valueMap[allocaInst->getName()] = builder->CreateAlloca(type, nullptr, allocaInst->getName());
                    mirTypeMap[allocaInst->getName()] = allocaInst->getType();
                    break;
                }
                case MIRInstructionKind::Load: {
                    auto* loadInst = static_cast<LoadInst*>(inst.get());
                    llvm::Value* ptr = valueMap[loadInst->getSrc()];
                    auto chthollyTy = mirTypeMap[loadInst->getSrc()];
                    llvm::Type* llvmTy = getLLVMType(chthollyTy.get());
                    valueMap[loadInst->getDest()] = builder->CreateLoad(llvmTy, ptr, loadInst->getDest());
                    mirTypeMap[loadInst->getDest()] = chthollyTy;
                    break;
                }
                case MIRInstructionKind::Store: {
                    auto* storeInst = static_cast<StoreInst*>(inst.get());
                    llvm::Value* val = valueMap[storeInst->getSrc()];
                    llvm::Value* ptr = valueMap[storeInst->getDest()];
                    builder->CreateStore(val, ptr);
                    break;
                }
                case MIRInstructionKind::BinOp: {
                    auto* binOp = static_cast<BinOpInst*>(inst.get());
                    llvm::Value* left = valueMap[binOp->getLeft()];
                    llvm::Value* right = valueMap[binOp->getRight()];
                    auto chthollyTy = mirTypeMap[binOp->getLeft()];
                    bool isUnsigned = chthollyTy && chthollyTy->isUnsigned();
                    
                    llvm::Value* res = nullptr;
                    switch (binOp->getOp()) {
                        case TokenType::Plus: res = builder->CreateAdd(left, right); break;
                        case TokenType::Minus: res = builder->CreateSub(left, right); break;
                        case TokenType::Star: res = builder->CreateMul(left, right); break;
                        case TokenType::Slash: 
                            res = isUnsigned ? builder->CreateUDiv(left, right) : builder->CreateSDiv(left, right); 
                            break;
                        case TokenType::Percent:
                            res = isUnsigned ? builder->CreateURem(left, right) : builder->CreateSRem(left, right);
                            break;
                        case TokenType::EqualEqual: res = builder->CreateICmpEQ(left, right); break;
                        case TokenType::NotEqual: res = builder->CreateICmpNE(left, right); break;
                        case TokenType::Less: 
                            res = isUnsigned ? builder->CreateICmpULT(left, right) : builder->CreateICmpSLT(left, right); 
                            break;
                        case TokenType::LessEqual: 
                            res = isUnsigned ? builder->CreateICmpULE(left, right) : builder->CreateICmpSLE(left, right); 
                            break;
                        case TokenType::Greater: 
                            res = isUnsigned ? builder->CreateICmpUGT(left, right) : builder->CreateICmpSGT(left, right); 
                            break;
                        case TokenType::GreaterEqual: 
                            res = isUnsigned ? builder->CreateICmpUGE(left, right) : builder->CreateICmpSGE(left, right); 
                            break;
                        case TokenType::ShiftLeft: res = builder->CreateShl(left, right); break;
                        case TokenType::ShiftRight:
                            res = isUnsigned ? builder->CreateLShr(left, right) : builder->CreateAShr(left, right);
                            break;
                        case TokenType::Ampersand: res = builder->CreateAnd(left, right); break;
                        case TokenType::Pipe: res = builder->CreateOr(left, right); break;
                        case TokenType::Caret: res = builder->CreateXor(left, right); break;
                        default: break;
                    }
                    valueMap[binOp->getDest()] = res;
                    mirTypeMap[binOp->getDest()] = mirTypeMap[binOp->getLeft()];
                    break;
                }
                case MIRInstructionKind::UnaryOp: {
                    auto* unaryOp = static_cast<UnaryOpInst*>(inst.get());
                    llvm::Value* operand = valueMap[unaryOp->getOperand()];
                    auto chthollyTy = mirTypeMap[unaryOp->getOperand()];
                    
                    llvm::Value* res = nullptr;
                    switch (unaryOp->getOp()) {
                        case TokenType::Plus: res = operand; break;
                        case TokenType::Minus: 
                            if (chthollyTy && chthollyTy->isFloat()) res = builder->CreateFNeg(operand);
                            else res = builder->CreateNeg(operand); 
                            break;
                        case TokenType::Not: res = builder->CreateNot(operand); break;
                        case TokenType::Tilde: res = builder->CreateNot(operand); break;
                        default: break;
                    }
                    valueMap[unaryOp->getDest()] = res;
                    mirTypeMap[unaryOp->getDest()] = chthollyTy;
                    break;
                }
                case MIRInstructionKind::Call: {
                    auto* call = static_cast<CallInst*>(inst.get());
                    llvm::Function* callee = llvmModule->getFunction(call->getCallee());
                    if (!callee) throw std::runtime_error("Undefined function: " + call->getCallee());
                    
                    std::vector<llvm::Value*> args;
                    for (const auto& argName : call->getArgs()) {
                        args.push_back(valueMap[argName]);
                    }
                    
                    if (callee->getReturnType()->isVoidTy()) {
                        builder->CreateCall(callee, args);
                    } else {
                        valueMap[call->getDest()] = builder->CreateCall(callee, args, call->getDest());
                        auto* targetFunc = mirModule.getFunction(call->getCallee());
                        if (targetFunc) {
                            mirTypeMap[call->getDest()] = targetFunc->getReturnType();
                        }
                    }
                    break;
                }
                case MIRInstructionKind::Ret: {
                    auto* ret = static_cast<ReturnInst*>(inst.get());
                    if (ret->getVal().empty()) {
                        builder->CreateRetVoid();
                    } else {
                        builder->CreateRet(valueMap[ret->getVal()]);
                    }
                    break;
                }
                case MIRInstructionKind::Br: {
                    auto* br = static_cast<BrInst*>(inst.get());
                    builder->CreateBr(blockMap[br->getTarget()]);
                    break;
                }
                case MIRInstructionKind::CondBr: {
                    auto* condBr = static_cast<CondBrInst*>(inst.get());
                    builder->CreateCondBr(valueMap[condBr->getCond()], 
                                          blockMap[condBr->getThenLabel()], 
                                          blockMap[condBr->getElseLabel()]);
                    break;
                }
                case MIRInstructionKind::ArrayElementPtr: {
                    auto* aepInst = static_cast<ArrayElementPtrInst*>(inst.get());
                    llvm::Value* arrayPtr = valueMap[aepInst->getPtr()];
                    llvm::Value* index = valueMap[aepInst->getIndex()];
                    llvm::Type* elemTy = getLLVMType(aepInst->getElementType().get());
                    valueMap[aepInst->getDest()] = builder->CreateGEP(elemTy, arrayPtr, index, aepInst->getDest());
                    mirTypeMap[aepInst->getDest()] = std::make_shared<PointerType>(aepInst->getElementType());
                    break;
                }
                case MIRInstructionKind::StructElementPtr: {
                    auto* sepInst = static_cast<StructElementPtrInst*>(inst.get());
                    llvm::Value* ptr = valueMap[sepInst->getPtr()];
                    
                    auto chthollyStructTy = structDefMap[sepInst->getStructName()];
                    if (!chthollyStructTy) {
                        throw std::runtime_error("Unknown struct type in codegen: " + sepInst->getStructName());
                    }
                    auto* llvmStructTy = structMap[sepInst->getStructName()];
                    
                    int fieldIndex = -1;
                    const auto& fields = chthollyStructTy->getFields();
                    for (int i = 0; i < (int)fields.size(); ++i) {
                        if (fields[i].name == sepInst->getFieldName()) {
                            fieldIndex = i;
                            break;
                        }
                    }
                    
                    if (fieldIndex == -1) {
                        throw std::runtime_error("Field '" + sepInst->getFieldName() + "' not found in struct " + sepInst->getStructName());
                    }

                    valueMap[sepInst->getDest()] = builder->CreateStructGEP(llvmStructTy, ptr, fieldIndex, sepInst->getDest());
                    mirTypeMap[sepInst->getDest()] = fields[fieldIndex].type;
                    break;
                }
                case MIRInstructionKind::VariantTag: {
                    auto* vTagInst = static_cast<VariantTagInst*>(inst.get());
                    llvm::Value* enumPtr = valueMap[vTagInst->getEnumPtr()];
                    auto chthollyTy = mirTypeMap[vTagInst->getEnumPtr()];
                    auto enumTy = std::dynamic_pointer_cast<EnumType>(chthollyTy);
                    auto* llvmEnumTy = enumMap[enumTy->getName()];

                    llvm::Value* tagPtr = builder->CreateStructGEP(llvmEnumTy, enumPtr, 0, "tagptr");
                    valueMap[vTagInst->getDest()] = builder->CreateLoad(llvm::Type::getInt32Ty(*context), tagPtr, vTagInst->getDest());
                    mirTypeMap[vTagInst->getDest()] = Type::getI32();
                    break;
                }
                case MIRInstructionKind::VariantData: {
                    auto* vDataInst = static_cast<VariantDataInst*>(inst.get());
                    llvm::Value* enumPtr = valueMap[vDataInst->getEnumPtr()];
                    auto chthollyTy = mirTypeMap[vDataInst->getEnumPtr()];
                    auto enumTy = std::dynamic_pointer_cast<EnumType>(chthollyTy);
                    auto* llvmEnumTy = enumMap[enumTy->getName()];
                    const auto& variant = enumTy->getVariants()[vDataInst->getTag()];

                    llvm::Value* tagPtr = builder->CreateStructGEP(llvmEnumTy, enumPtr, 0, "tagptr");
                    builder->CreateStore(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), vDataInst->getTag()), tagPtr);

                    if (!vDataInst->getArgs().empty()) {
                        llvm::Value* dataPtr = builder->CreateStructGEP(llvmEnumTy, enumPtr, 1, "dataptr");
                        std::vector<llvm::Type*> fieldTypes;
                        if (variant.kind == EnumType::Variant::Kind::Tuple) {
                            for (const auto& t : variant.tupleTypes) fieldTypes.push_back(getLLVMType(t.get()));
                        } else {
                            for (const auto& f : variant.structFields) fieldTypes.push_back(getLLVMType(f.type.get()));
                        }
                        llvm::StructType* variantTy = llvm::StructType::get(*context, fieldTypes);
                        llvm::Value* castedPtr = builder->CreatePointerCast(dataPtr, llvm::PointerType::get(variantTy, 0));
                        for (size_t i = 0; i < vDataInst->getArgs().size(); ++i) {
                            llvm::Value* fieldPtr = builder->CreateStructGEP(variantTy, castedPtr, (unsigned)i);
                            builder->CreateStore(valueMap[vDataInst->getArgs()[i]], fieldPtr);
                        }
                    }
                    break;
                }
                case MIRInstructionKind::VariantExtract: {
                    auto* vExtInst = static_cast<VariantExtractInst*>(inst.get());
                    llvm::Value* enumPtr = valueMap[vExtInst->getEnumPtr()];
                    auto chthollyTy = mirTypeMap[vExtInst->getEnumPtr()];
                    auto enumTy = std::dynamic_pointer_cast<EnumType>(chthollyTy);
                    auto* llvmEnumTy = enumMap[enumTy->getName()];
                    const auto& variant = enumTy->getVariants()[vExtInst->getTag()];

                    llvm::Value* dataPtr = builder->CreateStructGEP(llvmEnumTy, enumPtr, 1, "dataptr");
                    std::vector<llvm::Type*> fieldTypes;
                    if (variant.kind == EnumType::Variant::Kind::Tuple) {
                        for (const auto& t : variant.tupleTypes) fieldTypes.push_back(getLLVMType(t.get()));
                    } else {
                        for (const auto& f : variant.structFields) fieldTypes.push_back(getLLVMType(f.type.get()));
                    }
                    llvm::StructType* variantTy = llvm::StructType::get(*context, fieldTypes);
                    llvm::Value* castedPtr = builder->CreatePointerCast(dataPtr, llvm::PointerType::get(variantTy, 0));
                    llvm::Value* fieldPtr = builder->CreateStructGEP(variantTy, castedPtr, (unsigned)vExtInst->getFieldIndex());
                    
                    llvm::Type* llvmFieldTy = getLLVMType(vExtInst->getFieldType().get());
                    valueMap[vExtInst->getDest()] = builder->CreateLoad(llvmFieldTy, fieldPtr, vExtInst->getDest());
                    mirTypeMap[vExtInst->getDest()] = vExtInst->getFieldType();
                    break;
                }
                case MIRInstructionKind::Sizeof: {
                    auto* sizeofInst = static_cast<SizeofInst*>(inst.get());
                    llvm::Type* llvmTy = getLLVMType(sizeofInst->getType().get());
                    uint64_t size = llvmModule->getDataLayout().getTypeAllocSize(llvmTy);
                    valueMap[sizeofInst->getDest()] = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context), size);
                    mirTypeMap[sizeofInst->getDest()] = Type::getI64();
                    break;
                }
                case MIRInstructionKind::Alignof: {
                    auto* alignofInst = static_cast<AlignofInst*>(inst.get());
                    llvm::Type* llvmTy = getLLVMType(alignofInst->getType().get());
                    uint64_t align = llvmModule->getDataLayout().getABITypeAlign(llvmTy).value();
                    valueMap[alignofInst->getDest()] = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context), align);
                    mirTypeMap[alignofInst->getDest()] = Type::getI64();
                    break;
                }
                case MIRInstructionKind::Offsetof: {
                    auto* offsetofInst = static_cast<OffsetofInst*>(inst.get());
                    auto* llvmTy = llvm::dyn_cast<llvm::StructType>(getLLVMType(offsetofInst->getType().get()));
                    if (!llvmTy) throw std::runtime_error("offsetof requires a struct type");
                    
                    auto chthollyStructTy = std::dynamic_pointer_cast<StructType>(offsetofInst->getType());
                    int fieldIndex = chthollyStructTy->findFieldIndex(offsetofInst->getMemberName());
                    if (fieldIndex == -1) throw std::runtime_error("Field not found for offsetof: " + offsetofInst->getMemberName());

                    const llvm::StructLayout* layout = llvmModule->getDataLayout().getStructLayout(llvmTy);
                    uint64_t offset = layout->getElementOffset(fieldIndex);
                    valueMap[offsetofInst->getDest()] = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context), offset);
                    mirTypeMap[offsetofInst->getDest()] = Type::getI64();
                    break;
                }
                case MIRInstructionKind::ConstInt: {
                    auto* constInt = static_cast<ConstIntInst*>(inst.get());
                    valueMap[constInt->getDest()] = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), constInt->getValue());
                    mirTypeMap[constInt->getDest()] = Type::getI32();
                    break;
                }
                case MIRInstructionKind::ConstBool: {
                    auto* constBool = static_cast<ConstBoolInst*>(inst.get());
                    valueMap[constBool->getDest()] = llvm::ConstantInt::get(llvm::Type::getInt1Ty(*context), constBool->getValue());
                    mirTypeMap[constBool->getDest()] = Type::getBool();
                    break;
                }
                case MIRInstructionKind::ConstString: {
                    auto* constString = static_cast<ConstStringInst*>(inst.get());
                    valueMap[constString->getDest()] = getOrCreateGlobalString(constString->getValue());
                    mirTypeMap[constString->getDest()] = Type::getI8Ptr();
                    break;
                }
                case MIRInstructionKind::ConstDouble: {
                    auto* constDouble = static_cast<ConstDoubleInst*>(inst.get());
                    valueMap[constDouble->getDest()] = llvm::ConstantFP::get(llvm::Type::getDoubleTy(*context), constDouble->getValue());
                    mirTypeMap[constDouble->getDest()] = Type::getF64();
                    break;
                }
            }
        }
    }

    llvm::verifyFunction(*func);
}

llvm::Value* CodeGenerator::getOrCreateGlobalString(const std::string& str) {
    if (globalStrings.count(str)) return globalStrings[str];
    llvm::Value* ptr = builder->CreateGlobalStringPtr(str);
    globalStrings[str] = ptr;
    return ptr;
}

llvm::Type* CodeGenerator::getLLVMType(Type* chthollyType) {
    if (chthollyType->isI8() || chthollyType->getKind() == TypeKind::U8) return llvm::Type::getInt8Ty(*context);
    if (chthollyType->getKind() == TypeKind::I16 || chthollyType->getKind() == TypeKind::U16) return llvm::Type::getInt16Ty(*context);
    if (chthollyType->isI32() || chthollyType->getKind() == TypeKind::U32) return llvm::Type::getInt32Ty(*context);
    if (chthollyType->isI64() || chthollyType->getKind() == TypeKind::U64) return llvm::Type::getInt64Ty(*context);
    if (chthollyType->isFloatingPoint()) {
        if (chthollyType->getKind() == TypeKind::F32) return llvm::Type::getFloatTy(*context);
        return llvm::Type::getDoubleTy(*context);
    }
    if (chthollyType->isBoolean()) return llvm::Type::getInt1Ty(*context);
    if (chthollyType->isVoid()) return llvm::Type::getVoidTy(*context);
    if (chthollyType->isPointer()) {
        auto* ptrTy = static_cast<PointerType*>(chthollyType);
        if (ptrTy->getBaseType()->isStruct() || ptrTy->getBaseType()->isEnum()) {
            getLLVMType(ptrTy->getBaseType().get());
        }
        return llvm::PointerType::get(*context, 0);
    }
    if (chthollyType->isArray()) {
        auto* arrayTy = static_cast<ArrayType*>(chthollyType);
        return llvm::ArrayType::get(getLLVMType(arrayTy->getBaseType().get()), arrayTy->getSize());
    }
    
    if (chthollyType->isStruct()) {
        auto* structTy = static_cast<StructType*>(chthollyType);
        auto name = structTy->getName();
        if (structMap.count(name)) return structMap[name];

        auto* llvmStructTy = llvm::StructType::create(*context, name);
        structMap[name] = llvmStructTy;
        structDefMap[name] = std::dynamic_pointer_cast<StructType>(chthollyType->shared_from_this());

        std::vector<llvm::Type*> fieldTypes;
        for (const auto& field : structTy->getFields()) {
            fieldTypes.push_back(getLLVMType(field.type.get()));
        }
        llvmStructTy->setBody(fieldTypes);
        return llvmStructTy;
    }

    if (chthollyType->isEnum()) {
        auto* enumTy = static_cast<EnumType*>(chthollyType);
        auto name = enumTy->getName();
        if (enumMap.count(name)) return enumMap[name];

        auto* llvmEnumTy = llvm::StructType::create(*context, name);
        enumMap[name] = llvmEnumTy;
        enumDefMap[name] = std::dynamic_pointer_cast<EnumType>(chthollyType->shared_from_this());

        uint64_t maxSize = 8;
        for (const auto& variant : enumTy->getVariants()) {
            uint64_t currentSize = 0;
            if (variant.kind == EnumType::Variant::Kind::Tuple) {
                for (const auto& t : variant.tupleTypes) {
                    if (t->isI8() || t->isBoolean()) currentSize += 1;
                    else if (t->isInteger() || t->getKind() == TypeKind::F32) currentSize += 4;
                    else currentSize += 8;
                }
            } else {
                for (const auto& f : variant.structFields) {
                    auto t = f.type;
                    if (t->isI8() || t->isBoolean()) currentSize += 1;
                    else if (t->isInteger() || t->getKind() == TypeKind::F32) currentSize += 4;
                    else currentSize += 8;
                }
            }
            if (currentSize > maxSize) maxSize = currentSize;
        }
        maxSize = (maxSize + 7) & ~7;

        std::vector<llvm::Type*> fields;
        fields.push_back(llvm::Type::getInt32Ty(*context)); // Tag
        fields.push_back(llvm::ArrayType::get(llvm::Type::getInt8Ty(*context), maxSize)); // Data area
        llvmEnumTy->setBody(fields);
        return llvmEnumTy;
    }
    
    return llvm::Type::getVoidTy(*context);
}

void CodeGenerator::emitObjectFile(const std::string& filename) {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    auto targetTriple = llvm::sys::getDefaultTargetTriple();
    llvmModule->setTargetTriple(targetTriple);

    std::string error;
    auto target = llvm::TargetRegistry::lookupTarget(targetTriple, error);

    if (!target) {
        std::cerr << "CodeGenerator: Failed to lookup target: " << error << std::endl;
        return;
    }

    auto CPU = "generic";
    auto features = "";

    llvm::TargetOptions opt;
    auto RM = std::optional<llvm::Reloc::Model>(llvm::Reloc::PIC_);
    auto targetMachine = target->createTargetMachine(targetTriple, CPU, features, opt, RM);

    if (!targetMachine) {
        std::cerr << "CodeGenerator: Failed to create target machine" << std::endl;
        return;
    }

    llvmModule->setDataLayout(targetMachine->createDataLayout());

    std::error_code EC;
    llvm::raw_fd_ostream dest(filename, EC, llvm::sys::fs::OF_None);

    if (EC) {
        std::cerr << "CodeGenerator: Could not open file: " << EC.message() << std::endl;
        return;
    }

    llvm::legacy::PassManager pass;
    auto fileType = llvm::CodeGenFileType::ObjectFile;

    if (targetMachine->addPassesToEmitFile(pass, dest, nullptr, fileType)) {
        std::cerr << "CodeGenerator: TargetMachine can't emit a file of this type" << std::endl;
        return;
    }

    if (llvm::verifyModule(*llvmModule, &llvm::errs())) {
        std::cerr << "CodeGenerator: Module verification FAILED!" << std::endl;
        return;
    }

    pass.run(*llvmModule);
    dest.flush();
}

} // namespace chtholly
