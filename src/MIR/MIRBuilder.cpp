#include "MIR/MIRBuilder.h"
#include "AST/Patterns.h"
#include <stdexcept>
#include <iostream>

namespace chtholly {

MIRBuilder::MIRBuilder(MIRModule& module) : module(module) {}

void MIRBuilder::lower(ASTNode* node) {
    if (!node) return;
    switch (node->getKind()) {
        case ASTNodeKind::VarDecl:
            lowerVarDecl(static_cast<VarDecl*>(node));
            break;
        case ASTNodeKind::StructDecl: {
            auto structDecl = static_cast<StructDecl*>(node);
            if (!structDecl->getGenericParams().empty()) return;
            lowerStructDecl(structDecl);
            break;
        }
        case ASTNodeKind::EnumDecl: {
            auto enumDecl = static_cast<EnumDecl*>(node);
            if (!enumDecl->getGenericParams().empty()) return;
            lowerEnumDecl(enumDecl);
            break;
        }
        case ASTNodeKind::ClassDecl: {
            auto classDecl = static_cast<ClassDecl*>(node);
            if (!classDecl->getGenericParams().empty()) return;
            lowerClassDecl(classDecl);
            break;
        }
        case ASTNodeKind::Block:
            lowerBlock(static_cast<Block*>(node));
            break;
        case ASTNodeKind::FunctionDecl: {
            auto funcDecl = static_cast<FunctionDecl*>(node);
            if (!funcDecl->getGenericParams().empty()) return;
            lowerFunctionDecl(funcDecl);
            break;
        }
        case ASTNodeKind::ReturnStmt:
            lowerReturnStmt(static_cast<ReturnStmt*>(node));
            break;
        case ASTNodeKind::IfStmt:
            lowerIfStmt(static_cast<IfStmt*>(node));
            break;
        case ASTNodeKind::WhileStmt:
            lowerWhileStmt(static_cast<WhileStmt*>(node));
            break;
        case ASTNodeKind::DoWhileStmt:
            lowerDoWhileStmt(static_cast<DoWhileStmt*>(node));
            break;
        case ASTNodeKind::ForStmt:
            lowerForStmt(static_cast<ForStmt*>(node));
            break;
        case ASTNodeKind::SwitchStmt:
            lowerSwitchStmt(static_cast<SwitchStmt*>(node));
            break;
        case ASTNodeKind::BreakStmt:
            lowerBreakStmt(static_cast<BreakStmt*>(node));
            break;
        case ASTNodeKind::ContinueStmt:
            lowerContinueStmt(static_cast<ContinueStmt*>(node));
            break;
        case ASTNodeKind::ExprStmt:
            lowerExpr(const_cast<Expr*>(static_cast<ExprStmt*>(node)->getExpression()));
            break;
        case ASTNodeKind::LiteralExpr:
        case ASTNodeKind::IdentifierExpr:
        case ASTNodeKind::BinaryExpr:
        case ASTNodeKind::CallExpr:
        case ASTNodeKind::MemberAccessExpr:
        case ASTNodeKind::StructLiteralExpr:
        case ASTNodeKind::ArrayLiteralExpr:
        case ASTNodeKind::IndexingExpr:
        case ASTNodeKind::UnaryExpr:
        case ASTNodeKind::AddressOfExpr:
        case ASTNodeKind::DereferenceExpr:
        case ASTNodeKind::SpecializationExpr:
        case ASTNodeKind::IntrinsicExpr:
            lowerExpr(static_cast<Expr*>(node));
            break;
        default:
            break;
    }
}

std::string MIRBuilder::lowerExpr(Expr* expr) {
    if (!expr) return "";
    switch (expr->getKind()) {
        case ASTNodeKind::LiteralExpr:
            return lowerLiteralExpr(static_cast<LiteralExpr*>(expr));
        case ASTNodeKind::IdentifierExpr:
            return lowerIdentifierExpr(static_cast<IdentifierExpr*>(expr));
        case ASTNodeKind::BinaryExpr:
            return lowerBinaryExpr(static_cast<BinaryExpr*>(expr));
        case ASTNodeKind::UnaryExpr:
            return lowerUnaryExpr(static_cast<UnaryExpr*>(expr));
        case ASTNodeKind::CallExpr:
            return lowerCallExpr(static_cast<CallExpr*>(expr));
        case ASTNodeKind::MemberAccessExpr:
            return lowerMemberAccessExpr(static_cast<MemberAccessExpr*>(expr));
        case ASTNodeKind::StructLiteralExpr:
            return lowerStructLiteralExpr(static_cast<StructLiteralExpr*>(expr));
        case ASTNodeKind::ArrayLiteralExpr:
            return lowerArrayLiteralExpr(static_cast<ArrayLiteralExpr*>(expr));
        case ASTNodeKind::IndexingExpr:
            return lowerIndexingExpr(static_cast<IndexingExpr*>(expr));
        case ASTNodeKind::AddressOfExpr:
            return lowerAddressOfExpr(static_cast<AddressOfExpr*>(expr));
        case ASTNodeKind::DereferenceExpr:
            return lowerDereferenceExpr(static_cast<DereferenceExpr*>(expr));
        case ASTNodeKind::QuestionExpr:
            return lowerQuestionExpr(static_cast<QuestionExpr*>(expr));
        case ASTNodeKind::IntrinsicExpr:
            return lowerIntrinsicExpr(static_cast<IntrinsicExpr*>(expr));
        default:
            return "";
    }
}

void MIRBuilder::lowerVarDecl(VarDecl* decl) {
    if (!currentBlock) {
        throw std::runtime_error("No current block to lower VarDecl");
    }
    
    std::shared_ptr<Type> type = decl->getType();
    std::string initVal;
    if (decl->getInitializer()) {
        auto initExpr = const_cast<Expr*>(decl->getInitializer());
        initVal = lowerExpr(initExpr);
        if (!type) {
            // Simplified type inference from initializer
            if (initExpr->getKind() == ASTNodeKind::StructLiteralExpr) {
                auto structLit = static_cast<StructLiteralExpr*>(initExpr);
                auto it = structTypes.find(structLit->getBase()->toString());
                if (it != structTypes.end()) {
                    type = it->second.type;
                }
            }
        }
    }

    if (!type) type = Type::getI32();

    std::string mirName = "%" + decl->getName();
    currentBlock->appendInstruction(std::make_unique<AllocaInst>(mirName, type));
    
    if (!scopeStack.empty()) {
        auto& scope = scopeStack.back();
        scope.variables.push_back({decl->getName(), type});
        
        ShadowedVar shadow;
        shadow.name = decl->getName();
        if (varMap.count(shadow.name)) {
            shadow.oldMirName = varMap[shadow.name];
            if (ptrTypeMap.count(shadow.oldMirName)) {
                shadow.oldPtrType = ptrTypeMap[shadow.oldMirName];
            }
            scope.shadowed.push_back(shadow);
        }
    }

    varMap[decl->getName()] = mirName;
    if (type->isStruct()) {
        ptrTypeMap[mirName] = std::dynamic_pointer_cast<StructType>(type)->getName();
    }

    if (!initVal.empty()) {
        currentBlock->appendInstruction(std::make_unique<StoreInst>(initVal, mirName));
    }
}

void MIRBuilder::lowerBlock(Block* block, bool shouldPushScope) {
    if (shouldPushScope) pushScope();
    for (auto& stmt : block->getStatements()) {
        if (currentBlock->hasTerminator()) break;
        lower(stmt.get());
    }
    if (shouldPushScope) popScope();
}

void MIRBuilder::lowerFunctionDecl(FunctionDecl* decl) {
    // Reset function-local state
    scopeStack.clear();
    varMap.clear();
    ptrTypeMap.clear();
    tempCount = 0;

    auto func = std::make_unique<MIRFunction>(decl->getName(), decl->getReturnType());
    func->setVarArg(decl->getVarArg());
    
    // Add parameters to MIRFunction first
    for (const auto& param : decl->getParams()) {
        func->addParameter(param->getName(), param->getType());
    }

    currentFunction = func.get();
    
    if (decl->getIsExtern()) {
        module.appendFunction(std::move(func));
        currentFunction = nullptr;
        return;
    }

    auto entry = std::make_unique<BasicBlock>("entry.0");
    blockCount++;
    currentBlock = entry.get();
    currentFunction->appendBlock(std::move(entry));
    
    // Lower body parameters (alloca and store)
    pushScope();
    for (const auto& param : decl->getParams()) {
        std::string argName = "%" + param->getName();
        
        std::string stackName = "%" + param->getName() + ".addr";
        currentBlock->appendInstruction(std::make_unique<AllocaInst>(stackName, param->getType()));
        currentBlock->appendInstruction(std::make_unique<StoreInst>(argName, stackName));
        
        varMap[param->getName()] = stackName;
        if (!scopeStack.empty()) {
            scopeStack.back().variables.push_back({param->getName(), param->getType()});
        }
        if (param->getType()->isStruct()) {
             ptrTypeMap[stackName] = std::dynamic_pointer_cast<StructType>(param->getType())->getName();
        } else if (param->getType()->isPointer()) {
             auto ptr = std::dynamic_pointer_cast<PointerType>(param->getType());
             if (ptr->getBaseType()->isStruct()) {
                 ptrTypeMap[stackName] = std::dynamic_pointer_cast<StructType>(ptr->getBaseType())->getName();
             }
        }
    }
    
    lowerBlock(const_cast<Block*>(decl->getBody()), false);
    if (!currentBlock->hasTerminator()) {
        popScope();
        if (decl->getReturnType()->isVoid()) {
            currentBlock->appendInstruction(std::make_unique<ReturnInst>());
        } else if (decl->getReturnType()->isInteger()) {
            std::string temp = newTemp();
            currentBlock->appendInstruction(std::make_unique<ConstIntInst>(temp, 0));
            currentBlock->appendInstruction(std::make_unique<ReturnInst>(temp));
        }
    }
    
    module.appendFunction(std::move(func));
    currentFunction = nullptr;
    currentBlock = nullptr;
}

void MIRBuilder::lowerReturnStmt(ReturnStmt* stmt) {
    std::string val;
    if (stmt->getExpression()) {
        val = lowerExpr(const_cast<Expr*>(stmt->getExpression()));
    }
    emitAllDestructors();
    currentBlock->appendInstruction(std::make_unique<ReturnInst>(val));
}

void MIRBuilder::lowerIfStmt(IfStmt* stmt) {
    std::string cond = lowerExpr(const_cast<Expr*>(stmt->getCondition()));
    
    BasicBlock* thenBB = newBlock("if.then");
    BasicBlock* elseBB = stmt->getElseBlock() ? newBlock("if.else") : nullptr;
    BasicBlock* mergeBB = newBlock("if.merge");

    std::string thenLabel = thenBB->getName();
    std::string elseLabel = elseBB ? elseBB->getName() : mergeBB->getName();
    
    currentBlock->appendInstruction(std::make_unique<CondBrInst>(cond, thenLabel, elseLabel));

    // Then block
    currentBlock = thenBB;
    lowerBlock(const_cast<Block*>(stmt->getThenBlock()));
    bool thenTerminates = currentBlock->hasTerminator();
    if (!thenTerminates) {
        currentBlock->appendInstruction(std::make_unique<BrInst>(mergeBB->getName()));
    }

    // Else block
    bool elseTerminates = false;
    if (elseBB) {
        currentBlock = elseBB;
        lowerBlock(const_cast<Block*>(stmt->getElseBlock()));
        elseTerminates = currentBlock->hasTerminator();
        if (!elseTerminates) {
            currentBlock->appendInstruction(std::make_unique<BrInst>(mergeBB->getName()));
        }
    } else {
        // No else block, so it implicitly doesn't terminate (branches to merge)
        elseTerminates = false; 
    }

    if (thenTerminates && elseTerminates) {
        // Both branches return or break, mergeBB is unreachable
        // We still need a currentBlock for subsequent instructions, 
        // but we should probably delete mergeBB or just leave it empty if nothing follows.
        // For now, set currentBlock to mergeBB but it will be empty.
        currentBlock = mergeBB;
    } else {
        currentBlock = mergeBB;
    }
}

void MIRBuilder::lowerWhileStmt(WhileStmt* stmt) {
    BasicBlock* condBB = newBlock("while.cond");
    BasicBlock* bodyBB = newBlock("while.body");
    BasicBlock* mergeBB = newBlock("while.merge");

    currentBlock->appendInstruction(std::make_unique<BrInst>(condBB->getName()));

    loopStack.push_back({mergeBB->getName(), condBB->getName()});

    // Condition block
    currentBlock = condBB;
    std::string cond = lowerExpr(const_cast<Expr*>(stmt->getCondition()));
    currentBlock->appendInstruction(std::make_unique<CondBrInst>(cond, bodyBB->getName(), mergeBB->getName()));

    // Body block
    currentBlock = bodyBB;
    lowerBlock(const_cast<Block*>(stmt->getBody()));
    if (!currentBlock->hasTerminator()) {
        currentBlock->appendInstruction(std::make_unique<BrInst>(condBB->getName()));
    }

    loopStack.pop_back();
    currentBlock = mergeBB;
}

void MIRBuilder::lowerDoWhileStmt(DoWhileStmt* stmt) {
    BasicBlock* bodyBB = newBlock("do.body");
    BasicBlock* condBB = newBlock("do.cond");
    BasicBlock* mergeBB = newBlock("do.merge");

    currentBlock->appendInstruction(std::make_unique<BrInst>(bodyBB->getName()));

    loopStack.push_back({mergeBB->getName(), condBB->getName()});

    currentBlock = bodyBB;
    lowerBlock(const_cast<Block*>(stmt->getBody()));
    if (!currentBlock->hasTerminator()) {
        currentBlock->appendInstruction(std::make_unique<BrInst>(condBB->getName()));
    }

    currentBlock = condBB;
    std::string cond = lowerExpr(const_cast<Expr*>(stmt->getCondition()));
    currentBlock->appendInstruction(std::make_unique<CondBrInst>(cond, bodyBB->getName(), mergeBB->getName()));

    loopStack.pop_back();
    currentBlock = mergeBB;
}

void MIRBuilder::lowerForStmt(ForStmt* stmt) {
    BasicBlock* condBB = newBlock("for.cond");
    BasicBlock* bodyBB = newBlock("for.body");
    BasicBlock* stepBB = newBlock("for.step");
    BasicBlock* mergeBB = newBlock("for.merge");

    if (stmt->getInit()) lower(const_cast<Stmt*>(stmt->getInit()));
    currentBlock->appendInstruction(std::make_unique<BrInst>(condBB->getName()));

    loopStack.push_back({mergeBB->getName(), stepBB->getName()});

    currentBlock = condBB;
    if (stmt->getCondition()) {
        std::string cond = lowerExpr(const_cast<Expr*>(stmt->getCondition()));
        currentBlock->appendInstruction(std::make_unique<CondBrInst>(cond, bodyBB->getName(), mergeBB->getName()));
    } else {
        currentBlock->appendInstruction(std::make_unique<BrInst>(bodyBB->getName()));
    }

    currentBlock = bodyBB;
    lowerBlock(const_cast<Block*>(stmt->getBody()));
    if (!currentBlock->hasTerminator()) {
        currentBlock->appendInstruction(std::make_unique<BrInst>(stepBB->getName()));
    }

    currentBlock = stepBB;
    if (stmt->getStep()) lowerExpr(const_cast<Expr*>(stmt->getStep()));
    currentBlock->appendInstruction(std::make_unique<BrInst>(condBB->getName()));

    loopStack.pop_back();
    currentBlock = mergeBB;
}

void MIRBuilder::lowerSwitchStmt(SwitchStmt* stmt) {
    auto condExpr = const_cast<Expr*>(stmt->getCondition());
    std::string condAddr = lowerAddr(condExpr);
    auto condType = condExpr->getType();

    BasicBlock* endBB = newBlock("switch.end");
    std::string endLabel = endBB->getName();
    std::string nextCaseLabel;

    loopStack.push_back({endLabel, ""});

    for (size_t i = 0; i < stmt->getCases().size(); ++i) {
        const auto& c = stmt->getCases()[i];
        BasicBlock* bodyBB = newBlock("case.body");
        std::string bodyLabel = bodyBB->getName();
        
        BasicBlock* nextCaseBB = nullptr;
        if (i < stmt->getCases().size() - 1) {
            nextCaseBB = newBlock("case.next");
            nextCaseLabel = nextCaseBB->getName();
        } else {
            nextCaseLabel = endLabel;
        }

        if (c->isDefaultCase()) {
            currentBlock->appendInstruction(std::make_unique<BrInst>(bodyLabel));
        } else {
            // Lower pattern matching logic
            if (condType->isEnum()) {
                auto kind = c->getPattern()->getKind();
                if (kind == ASTNodeKind::VariantPattern) {
                    auto varPat = static_cast<const VariantPattern*>(c->getPattern());
                    auto enumTy = std::dynamic_pointer_cast<EnumType>(condType);
                    int tag = enumTy->findVariantIndex(varPat->getVariantName());

                    std::string actualTag = newTemp();
                    currentBlock->appendInstruction(std::make_unique<VariantTagInst>(actualTag, condAddr));
                    
                    std::string expectedTag = newTemp();
                    currentBlock->appendInstruction(std::make_unique<ConstIntInst>(expectedTag, tag));
                    
                    std::string cmp = newTemp();
                    currentBlock->appendInstruction(std::make_unique<BinOpInst>(cmp, actualTag, expectedTag, TokenType::EqualEqual));
                    
                    currentBlock->appendInstruction(std::make_unique<CondBrInst>(cmp, bodyLabel, nextCaseLabel));
                } else if (kind == ASTNodeKind::WildcardPattern || kind == ASTNodeKind::IdentifierPattern) {
                    currentBlock->appendInstruction(std::make_unique<BrInst>(bodyLabel));
                } else {
                    currentBlock->appendInstruction(std::make_unique<BrInst>(nextCaseLabel));
                }
            } else {
                auto kind = c->getPattern()->getKind();
                if (kind == ASTNodeKind::LiteralPattern) {
                    auto litPat = static_cast<const LiteralPattern*>(c->getPattern());
                    std::string val = lowerExpr(const_cast<LiteralExpr*>(const_cast<LiteralExpr*>(litPat->getLiteral())));
                    std::string condVal = newTemp();
                    currentBlock->appendInstruction(std::make_unique<LoadInst>(condVal, condAddr));
                    
                    std::string cmp = newTemp();
                    currentBlock->appendInstruction(std::make_unique<BinOpInst>(cmp, condVal, val, TokenType::EqualEqual));
                    currentBlock->appendInstruction(std::make_unique<CondBrInst>(cmp, bodyLabel, nextCaseLabel));
                } else if (kind == ASTNodeKind::WildcardPattern || kind == ASTNodeKind::IdentifierPattern) {
                    currentBlock->appendInstruction(std::make_unique<BrInst>(bodyLabel));
                } else {
                    currentBlock->appendInstruction(std::make_unique<BrInst>(nextCaseLabel));
                }
            }
        }

        // Case Body
        currentBlock = bodyBB;
        
        if (!c->isDefaultCase() && condType->isEnum()) {
            if (c->getPattern()->getKind() == ASTNodeKind::VariantPattern) {
                auto varPat = static_cast<const VariantPattern*>(c->getPattern());
                auto enumTy = std::dynamic_pointer_cast<EnumType>(condType);
                auto* variant = enumTy->findVariant(varPat->getVariantName());
                const auto& subPats = varPat->getSubPatterns();
                
                for (size_t j = 0; j < subPats.size(); ++j) {
                    if (subPats[j]->getKind() == ASTNodeKind::IdentifierPattern) {
                        auto idPat = static_cast<const IdentifierPattern*>(subPats[j].get());
                        std::string fieldVal = newTemp();
                        std::shared_ptr<Type> fieldType;
                        if (variant->kind == EnumType::Variant::Kind::Tuple) fieldType = variant->tupleTypes[j];
                        else fieldType = variant->structFields[j].type;

                        currentBlock->appendInstruction(std::make_unique<VariantExtractInst>(fieldVal, condAddr, (int)enumTy->findVariantIndex(variant->name), (int)j, fieldType));
                        
                        // Bind to local variable
                        std::string localAddr = newTemp();
                        currentBlock->appendInstruction(std::make_unique<AllocaInst>(localAddr, fieldType));
                        currentBlock->appendInstruction(std::make_unique<StoreInst>(fieldVal, localAddr));
                        varMap[idPat->getName()] = localAddr;
                    }
                }
            }
        }

        lowerBlock(const_cast<Block*>(c->getBody()));
        if (!currentBlock->hasTerminator()) {
            currentBlock->appendInstruction(std::make_unique<BrInst>(endLabel));
        }

        if (nextCaseBB) {
            currentBlock = nextCaseBB;
        }
    }

    currentBlock = endBB;
    loopStack.pop_back();
}

void MIRBuilder::lowerBreakStmt(BreakStmt* stmt) {
    if (loopStack.empty()) throw std::runtime_error("Break outside of loop/switch context");
    currentBlock->appendInstruction(std::make_unique<BrInst>(loopStack.back().breakLabel));
}

void MIRBuilder::lowerContinueStmt(ContinueStmt* stmt) {
    if (loopStack.empty() || loopStack.back().continueLabel.empty()) 
        throw std::runtime_error("Continue outside of loop context");
    currentBlock->appendInstruction(std::make_unique<BrInst>(loopStack.back().continueLabel));
}

std::string MIRBuilder::lowerAddr(Expr* expr) {
    if (!expr) return "";
    switch (expr->getKind()) {
        case ASTNodeKind::IdentifierExpr: {
            auto id = static_cast<IdentifierExpr*>(expr);
            if (varMap.count(id->toString())) return varMap[id->toString()];
            throw std::runtime_error("Undefined variable: " + id->toString());
        }
        case ASTNodeKind::MemberAccessExpr: {
            auto memAccess = static_cast<MemberAccessExpr*>(expr);
            std::string baseAddr = lowerAddr(const_cast<Expr*>(memAccess->getBase()));
            std::string structName = ptrTypeMap[baseAddr];
            std::string result = newTemp();
            currentBlock->appendInstruction(std::make_unique<StructElementPtrInst>(result, baseAddr, structName, memAccess->getMemberName()));
            return result;
        }
        case ASTNodeKind::IndexingExpr: {
            auto indexing = static_cast<IndexingExpr*>(expr);
            std::string baseAddr = lowerAddr(const_cast<Expr*>(indexing->getBase()));
            std::string indexVal = lowerExpr(const_cast<Expr*>(indexing->getIndex()));
            std::string result = newTemp();
            auto arrayTy = std::dynamic_pointer_cast<ArrayType>(const_cast<Expr*>(indexing->getBase())->getType());
            currentBlock->appendInstruction(std::make_unique<ArrayElementPtrInst>(result, baseAddr, indexVal, arrayTy->getBaseType()));
            return result;
        }
        case ASTNodeKind::DereferenceExpr: {
            auto deref = static_cast<DereferenceExpr*>(expr);
            return lowerExpr(const_cast<Expr*>(deref->getOperand()));
        }
        default:
            return "";
    }
}

std::string MIRBuilder::lowerLiteralExpr(LiteralExpr* expr) {
    std::string temp = newTemp();
    const auto& value = expr->getValue();

    if (std::holds_alternative<bool>(value)) {
        currentBlock->appendInstruction(std::make_unique<ConstIntInst>(temp, std::get<bool>(value) ? 1 : 0));
    } else if (std::holds_alternative<int64_t>(value)) {
        currentBlock->appendInstruction(std::make_unique<ConstIntInst>(temp, std::get<int64_t>(value)));
    } else if (std::holds_alternative<std::string>(value)) {
        currentBlock->appendInstruction(std::make_unique<ConstStringInst>(temp, std::get<std::string>(value)));
    } else if (std::holds_alternative<double>(value)) {
        currentBlock->appendInstruction(std::make_unique<ConstDoubleInst>(temp, std::get<double>(value)));
    } else if (std::holds_alternative<std::nullptr_t>(value)) {
        currentBlock->appendInstruction(std::make_unique<ConstIntInst>(temp, 0));
    }
    
    return temp;
}

std::string MIRBuilder::lowerBinaryExpr(BinaryExpr* expr) {
    if (expr->getOp() == TokenType::Equal) {
        std::string dest = lowerAddr(const_cast<Expr*>(expr->getLeft()));
        std::string src = lowerExpr(const_cast<Expr*>(expr->getRight()));
        currentBlock->appendInstruction(std::make_unique<StoreInst>(src, dest));
        return src;
    }

    std::string left = lowerExpr(const_cast<Expr*>(expr->getLeft()));
    std::string right = lowerExpr(const_cast<Expr*>(expr->getRight()));
    std::string dest = newTemp();
    currentBlock->appendInstruction(std::make_unique<BinOpInst>(dest, left, right, expr->getOp()));
    return dest;
}

std::string MIRBuilder::lowerUnaryExpr(UnaryExpr* expr) {
    std::string operand = lowerExpr(const_cast<Expr*>(expr->getOperand()));
    std::string dest = newTemp();
    currentBlock->appendInstruction(std::make_unique<UnaryOpInst>(dest, operand, expr->getOp()));
    return dest;
}

std::string MIRBuilder::lowerIdentifierExpr(IdentifierExpr* id) {
    std::string name = id->toString();
    if (varMap.find(name) != varMap.end()) {
        std::string src = varMap[name];
        std::string dest = newTemp();
        currentBlock->appendInstruction(std::make_unique<LoadInst>(dest, src));
        return dest;
    }

    for (auto const& [enumName, enumTy] : enumTypes) {
        int tag = enumTy->findVariantIndex(name);
        if (tag != -1) {
            std::string enumPtr = newTemp();
            currentBlock->appendInstruction(std::make_unique<AllocaInst>(enumPtr, enumTy));
            
            std::string voidDest = newTemp();
            currentBlock->appendInstruction(std::make_unique<VariantDataInst>(voidDest, enumPtr, tag, std::vector<std::string>{}));
            
            std::string result = newTemp();
            currentBlock->appendInstruction(std::make_unique<LoadInst>(result, enumPtr));
            return result;
        }
    }

    throw std::runtime_error("Undefined identifier in MIR lowering: " + name);
}

std::string MIRBuilder::lowerCallExpr(CallExpr* call) {
    std::string calleeName;
    std::vector<std::string> args;

    if (call->getCallee()->getKind() == ASTNodeKind::IdentifierExpr) {
        auto id = static_cast<const IdentifierExpr*>(call->getCallee());
        calleeName = id->toString();
        
        if (structTypes.find(calleeName) != structTypes.end()) {
             auto& info = structTypes[calleeName];
             std::string objPtr = newTemp();
             currentBlock->appendInstruction(std::make_unique<AllocaInst>(objPtr, info.type));
             ptrTypeMap[objPtr] = calleeName;
             
             std::vector<std::string> ctorArgs;
             ctorArgs.push_back(objPtr);
             for (const auto& arg : call->getArgs()) {
                 ctorArgs.push_back(lowerExpr(arg.get()));
             }
             
             std::string ctorName = calleeName + "_" + calleeName;
             std::string voidDest = newTemp();
             currentBlock->appendInstruction(std::make_unique<CallInst>(voidDest, ctorName, std::move(ctorArgs)));
             
             std::string result = newTemp();
             currentBlock->appendInstruction(std::make_unique<LoadInst>(result, objPtr));
             return result;
        }

        for (auto const& [name, enumTy] : enumTypes) {
            int tag = enumTy->findVariantIndex(calleeName);
            if (tag != -1) {
                std::string enumPtr = newTemp();
                currentBlock->appendInstruction(std::make_unique<AllocaInst>(enumPtr, enumTy));
                
                std::vector<std::string> variantArgs;
                for (const auto& arg : call->getArgs()) {
                    variantArgs.push_back(lowerExpr(arg.get()));
                }
                
                std::string voidDest = newTemp();
                currentBlock->appendInstruction(std::make_unique<VariantDataInst>(voidDest, enumPtr, tag, std::move(variantArgs)));
                
                std::string result = newTemp();
                currentBlock->appendInstruction(std::make_unique<LoadInst>(result, enumPtr));
                return result;
            }
        }

        for (const auto& arg : call->getArgs()) {
            args.push_back(lowerExpr(arg.get()));
        }
    } else if (call->getCallee()->getKind() == ASTNodeKind::SpecializationExpr) {
        auto spec = static_cast<const SpecializationExpr*>(call->getCallee());
        calleeName = spec->getMangledName();
        if (calleeName.empty()) throw std::runtime_error("Specialization name not resolved by Sema: " + spec->toString());
        
        if (spec->getBase()->getKind() == ASTNodeKind::MemberAccessExpr) {
            auto memAccess = static_cast<const MemberAccessExpr*>(spec->getBase());
            // It's a method call!
            std::string selfAddr = lowerAddr(const_cast<Expr*>(memAccess->getBase()));
            args.push_back(selfAddr);
        }

        for (const auto& arg : call->getArgs()) {
            args.push_back(lowerExpr(arg.get()));
        }
    } else if (call->getCallee()->getKind() == ASTNodeKind::MemberAccessExpr) {
        auto memAccess = static_cast<const MemberAccessExpr*>(call->getCallee());
        std::string baseName;
        bool isVariable = false;
        
        if (memAccess->getBase()->getKind() == ASTNodeKind::IdentifierExpr) {
            auto id = static_cast<const IdentifierExpr*>(memAccess->getBase());
            baseName = id->toString();
            if (varMap.count(baseName)) isVariable = true;
        } else if (memAccess->getBase()->getKind() == ASTNodeKind::SpecializationExpr) {
            auto spec = static_cast<const SpecializationExpr*>(memAccess->getBase());
            baseName = spec->getMangledName();
        }

        if (!baseName.empty() && !isVariable) {
             if (moduleNames.count(baseName)) {
                 calleeName = baseName + "_" + memAccess->getMemberName();
                 for (const auto& arg : call->getArgs()) {
                     args.push_back(lowerExpr(arg.get()));
                 }
             } else if (enumTypes.find(baseName) != enumTypes.end()) {
                 auto enumTy = enumTypes[baseName];
                 int tag = enumTy->findVariantIndex(memAccess->getMemberName());
                 if (tag != -1) {
                     std::string enumPtr = newTemp();
                     currentBlock->appendInstruction(std::make_unique<AllocaInst>(enumPtr, enumTy));
                     
                     std::vector<std::string> variantArgs;
                     for (const auto& arg : call->getArgs()) {
                         variantArgs.push_back(lowerExpr(arg.get()));
                     }
                     
                     std::string voidDest = newTemp();
                     currentBlock->appendInstruction(std::make_unique<VariantDataInst>(voidDest, enumPtr, tag, std::move(variantArgs)));
                     
                     std::string result = newTemp();
                     currentBlock->appendInstruction(std::make_unique<LoadInst>(result, enumPtr));
                     return result;
                 }
             } else if (structTypes.find(baseName) != structTypes.end()) {
                 // Static method on specialized class
                 calleeName = baseName + "_" + memAccess->getMemberName();
                 for (const auto& arg : call->getArgs()) {
                     args.push_back(lowerExpr(arg.get()));
                 }
             }
        } else if (isVariable) {
             std::string allocaPtr = varMap[baseName];
             if (ptrTypeMap.find(allocaPtr) == ptrTypeMap.end()) {
                 throw std::runtime_error("Unknown type for variable: " + baseName);
             }
             std::string className = ptrTypeMap[allocaPtr];
             
             calleeName = className + "_" + memAccess->getMemberName();
             args.push_back(allocaPtr);
             
             for (const auto& arg : call->getArgs()) {
                args.push_back(lowerExpr(arg.get()));
             }
        } else {
             throw std::runtime_error("Complex base in method call not supported yet");
        }
    } else {
        throw std::runtime_error("Complex callee not supported in MIRBuilder yet: " + call->getCallee()->toString());
    }

    std::string dest = newTemp();
    currentBlock->appendInstruction(std::make_unique<CallInst>(dest, calleeName, std::move(args)));
    return dest;
}

std::string MIRBuilder::lowerMemberAccessExpr(MemberAccessExpr* expr) {
    if (expr->getBase()->getKind() == ASTNodeKind::IdentifierExpr) {
        auto id = static_cast<const IdentifierExpr*>(expr->getBase());
        std::string baseName = id->toString();
        if (enumTypes.find(baseName) != enumTypes.end()) {
            auto enumTy = enumTypes[baseName];
            int tag = enumTy->findVariantIndex(expr->getMemberName());
            if (tag != -1) {
                std::string enumPtr = newTemp();
                currentBlock->appendInstruction(std::make_unique<AllocaInst>(enumPtr, enumTy));
                
                std::string voidDest = newTemp();
                currentBlock->appendInstruction(std::make_unique<VariantDataInst>(voidDest, enumPtr, tag, std::vector<std::string>{}));
                
                std::string result = newTemp();
                currentBlock->appendInstruction(std::make_unique<LoadInst>(result, enumPtr));
                return result;
            }
        }
    } else if (expr->getBase()->getKind() == ASTNodeKind::SpecializationExpr) {
        auto spec = static_cast<const SpecializationExpr*>(expr->getBase());
        std::string mangledName = spec->getMangledName();
        if (enumTypes.find(mangledName) != enumTypes.end()) {
            auto enumTy = enumTypes[mangledName];
            int tag = enumTy->findVariantIndex(expr->getMemberName());
            if (tag != -1) {
                std::string enumPtr = newTemp();
                currentBlock->appendInstruction(std::make_unique<AllocaInst>(enumPtr, enumTy));
                
                std::vector<std::string> variantArgs; // Unit variant has no args
                std::string voidDest = newTemp();
                currentBlock->appendInstruction(std::make_unique<VariantDataInst>(voidDest, enumPtr, tag, std::move(variantArgs)));
                
                std::string result = newTemp();
                currentBlock->appendInstruction(std::make_unique<LoadInst>(result, enumPtr));
                return result;
            }
        }
    }

    std::string basePtr = lowerAddr(const_cast<Expr*>(expr->getBase()));
    std::string structName = ptrTypeMap[basePtr]; 

    std::string fieldPtr = newTemp();
    currentBlock->appendInstruction(std::make_unique<StructElementPtrInst>(fieldPtr, basePtr, structName, expr->getMemberName()));
    
    std::string dest = newTemp();
    currentBlock->appendInstruction(std::make_unique<LoadInst>(dest, fieldPtr));
    return dest;
}

std::string MIRBuilder::lowerStructLiteralExpr(StructLiteralExpr* expr) {
    std::string name;
    if (expr->getBase()->getKind() == ASTNodeKind::SpecializationExpr) {
        auto spec = static_cast<const SpecializationExpr*>(expr->getBase());
        name = spec->getMangledName();
    } else {
        name = expr->getBase()->toString();
    }

    auto it = structTypes.find(name);
    if (it == structTypes.end()) {
        size_t pos = name.find("::");
        if (pos != std::string::npos) {
            std::string enumName = name.substr(0, pos);
            std::string variantName = name.substr(pos + 2);
            auto enumIt = enumTypes.find(enumName);
            if (enumIt != enumTypes.end()) {
                auto enumTy = enumIt->second;
                int tag = enumTy->findVariantIndex(variantName);
                if (tag != -1) {
                    const auto& variant = enumTy->getVariants()[tag];
                    std::string enumPtr = newTemp();
                    currentBlock->appendInstruction(std::make_unique<AllocaInst>(enumPtr, enumTy));
                    ptrTypeMap[enumPtr] = enumName;

                    std::vector<std::string> args;
                    for (const auto& f : variant.structFields) {
                        for (const auto& init : expr->getFields()) {
                            if (init.name == f.name) {
                                args.push_back(lowerExpr(const_cast<Expr*>(init.value.get())));
                                break;
                            }
                        }
                    }

                    std::string voidDest = newTemp();
                    currentBlock->appendInstruction(std::make_unique<VariantDataInst>(voidDest, enumPtr, tag, std::move(args)));
                    
                    std::string result = newTemp();
                    currentBlock->appendInstruction(std::make_unique<LoadInst>(result, enumPtr));
                    return result;
                }
            }
        }
        throw std::runtime_error("Unknown struct type in MIR lowering: " + name);
    }
    const auto& info = it->second;

    std::string structPtr = newTemp();
    currentBlock->appendInstruction(std::make_unique<AllocaInst>(structPtr, info.type));
    ptrTypeMap[structPtr] = name;
    
    for (const auto& init : expr->getFields()) {
        std::string val = lowerExpr(const_cast<Expr*>(init.value.get()));
        std::string fieldPtr = newTemp();
        currentBlock->appendInstruction(std::make_unique<StructElementPtrInst>(fieldPtr, structPtr, name, init.name));
        currentBlock->appendInstruction(std::make_unique<StoreInst>(val, fieldPtr));
    }
    
    std::string dest = newTemp();
    currentBlock->appendInstruction(std::make_unique<LoadInst>(dest, structPtr));
    return dest;
}

std::string MIRBuilder::lowerArrayLiteralExpr(ArrayLiteralExpr* expr) {
    auto arrayType = std::make_shared<ArrayType>(Type::getI32(), (int)expr->getElements().size());
    
    std::string arrayPtr = newTemp();
    currentBlock->appendInstruction(std::make_unique<AllocaInst>(arrayPtr, arrayType));
    
    for (size_t i = 0; i < expr->getElements().size(); ++i) {
        std::string val = lowerExpr(expr->getElements()[i].get());
        std::string indexTemp = newTemp();
        currentBlock->appendInstruction(std::make_unique<ConstIntInst>(indexTemp, i));
        std::string elPtr = newTemp();
        currentBlock->appendInstruction(std::make_unique<ArrayElementPtrInst>(elPtr, arrayPtr, indexTemp, arrayType->getBaseType()));
        currentBlock->appendInstruction(std::make_unique<StoreInst>(val, elPtr));
    }
    
    std::string dest = newTemp();
    currentBlock->appendInstruction(std::make_unique<LoadInst>(dest, arrayPtr));
    return dest;
}

std::string MIRBuilder::lowerIndexingExpr(IndexingExpr* expr) {
    std::string elPtr = lowerAddr(expr);
    std::string dest = newTemp();
    currentBlock->appendInstruction(std::make_unique<LoadInst>(dest, elPtr));
    return dest;
}

std::string MIRBuilder::lowerAddressOfExpr(AddressOfExpr* expr) {
    return lowerAddr(const_cast<Expr*>(expr->getOperand()));
}

std::string MIRBuilder::lowerDereferenceExpr(DereferenceExpr* expr) {
    std::string ptr = lowerExpr(const_cast<Expr*>(expr->getOperand()));
    std::string dest = newTemp();
    currentBlock->appendInstruction(std::make_unique<LoadInst>(dest, ptr));
    return dest;
}

std::string MIRBuilder::lowerQuestionExpr(QuestionExpr* expr) {
    std::string resVal = lowerExpr(const_cast<Expr*>(expr->getOperand()));
    auto enumTy = std::dynamic_pointer_cast<EnumType>(const_cast<Expr*>(expr->getOperand())->getType());
    
    // Allocate space for the Result to extract tag and data
    std::string resAddr = newTemp();
    currentBlock->appendInstruction(std::make_unique<AllocaInst>(resAddr, enumTy));
    currentBlock->appendInstruction(std::make_unique<StoreInst>(resVal, resAddr));

    std::string tag = newTemp();
    currentBlock->appendInstruction(std::make_unique<VariantTagInst>(tag, resAddr));

    BasicBlock* okBB = newBlock("q.ok");
    BasicBlock* errBB = newBlock("q.err");
    BasicBlock* mergeBB = newBlock("q.merge");

    // 0 is Ok, 1 is Err
    std::string isErr = newTemp();
    std::string zero = newTemp();
    currentBlock->appendInstruction(std::make_unique<ConstIntInst>(zero, 0));
    currentBlock->appendInstruction(std::make_unique<BinOpInst>(isErr, tag, zero, TokenType::NotEqual));
    currentBlock->appendInstruction(std::make_unique<CondBrInst>(isErr, errBB->getName(), okBB->getName()));

    // Err block: early return
    currentBlock = errBB;
    emitAllDestructors();
    currentBlock->appendInstruction(std::make_unique<ReturnInst>(resVal));

    // Ok block: extract value
    currentBlock = okBB;
    std::string okVal = newTemp();
    auto okType = enumTy->getVariants()[0].tupleTypes[0];
    currentBlock->appendInstruction(std::make_unique<VariantExtractInst>(okVal, resAddr, 0, 0, okType));
    currentBlock->appendInstruction(std::make_unique<BrInst>(mergeBB->getName()));

    currentBlock = mergeBB;
    return okVal;
}

std::string MIRBuilder::lowerIntrinsicExpr(IntrinsicExpr* expr) {
    std::string result = newTemp();
    switch (expr->getIntrinsicKind()) {
        case IntrinsicExpr::IntrinsicKind::Sizeof:
            currentBlock->appendInstruction(std::make_unique<SizeofInst>(result, expr->getTypeArg()));
            break;
        case IntrinsicExpr::IntrinsicKind::Alignof:
            currentBlock->appendInstruction(std::make_unique<AlignofInst>(result, expr->getTypeArg()));
            break;
        case IntrinsicExpr::IntrinsicKind::Offsetof: {
            std::string memberName = static_cast<IdentifierExpr*>(expr->getArgs()[0].get())->toString();
            currentBlock->appendInstruction(std::make_unique<OffsetofInst>(result, expr->getTypeArg(), memberName));
            break;
        }
        case IntrinsicExpr::IntrinsicKind::Malloc: {
            std::string sizeVal;
            if (expr->getArgs().empty()) {
                std::string sizeTemp = newTemp();
                currentBlock->appendInstruction(std::make_unique<SizeofInst>(sizeTemp, expr->getTypeArg()));
                sizeVal = sizeTemp;
            } else {
                sizeVal = lowerExpr(expr->getArgs()[0].get());
            }
            currentBlock->appendInstruction(std::make_unique<CallInst>(result, "malloc", std::vector<std::string>{sizeVal}));
            break;
        }
        case IntrinsicExpr::IntrinsicKind::Alloca: {
            currentBlock->appendInstruction(std::make_unique<AllocaInst>(result, expr->getTypeArg()));
            break;
        }
        case IntrinsicExpr::IntrinsicKind::Free: {
            std::string ptrVal = lowerExpr(expr->getArgs()[0].get());
            currentBlock->appendInstruction(std::make_unique<CallInst>("", "free", std::vector<std::string>{ptrVal}));
            break;
        }
    }
    return result;
}

void MIRBuilder::lowerStructDecl(StructDecl* decl) {
    std::cout << "MIRBuilder: Registering struct " << decl->getName() << std::endl;
    std::vector<StructType::Field> structFields;
    std::vector<std::string> fieldNames;
    for (const auto& member : decl->getMembers()) {
        structFields.push_back({member->getName(), member->getType(), member->isPublic()});
        fieldNames.push_back(member->getName());
    }
    auto structType = std::make_shared<StructType>(decl->getName(), std::move(structFields));
    structTypes[decl->getName()] = {decl->getName(), structType, std::move(fieldNames)};
}

void MIRBuilder::lowerEnumDecl(EnumDecl* decl) {
    if (auto type = decl->getType()) {
        if (auto enumTy = std::dynamic_pointer_cast<EnumType>(type)) {
            enumTypes[decl->getName()] = enumTy;
        }
    }
}

std::string MIRBuilder::newTemp() {
    return "%t" + std::to_string(tempCount++);
}

BasicBlock* MIRBuilder::newBlock(std::string name) {
    if (!currentFunction) {
        throw std::runtime_error("No current function to add block to");
    }
    std::string uniqueName = name + "." + std::to_string(blockCount++);
    auto block = std::make_unique<BasicBlock>(uniqueName);
    BasicBlock* ptr = block.get();
    currentFunction->appendBlock(std::move(block));
    return ptr;
}

void MIRBuilder::lowerClassDecl(ClassDecl* decl) {
    StructInfo info;
    info.name = decl->getName();
    std::vector<StructType::Field> fields;
    
    for (const auto& member : decl->getMembers()) {
        if (member->getKind() == ASTNodeKind::VarDecl) {
            auto varDecl = static_cast<VarDecl*>(member.get());
            info.fieldNames.push_back(varDecl->getName());
            fields.push_back({varDecl->getName(), varDecl->getType(), varDecl->isPublic()});
        }
    }
    
    info.type = std::make_shared<StructType>(decl->getName(), std::move(fields));
    
    // Check for destructor
    for (const auto& member : decl->getMembers()) {
        if (member->getKind() == ASTNodeKind::MethodDecl) {
            auto method = static_cast<MethodDecl*>(member.get());
            if (method->getName() == "~" + decl->getName()) {
                info.hasDestructor = true;
                break;
            }
        }
    }

    structTypes[decl->getName()] = info;

    for (const auto& member : decl->getMembers()) {
        if (member->getKind() == ASTNodeKind::MethodDecl) {
            lowerMethodDecl(static_cast<MethodDecl*>(member.get()), decl->getName());
        } else if (member->getKind() == ASTNodeKind::ConstructorDecl) {
            lowerConstructorDecl(static_cast<ConstructorDecl*>(member.get()), decl->getName());
        }
    }
}

void MIRBuilder::lowerMethodDecl(MethodDecl* decl, const std::string& className) {
    std::string mangledName = className + "_" + decl->getName();
    auto func = std::make_unique<MIRFunction>(mangledName, decl->getReturnType());
    currentFunction = func.get();
    
    auto entry = std::make_unique<BasicBlock>("entry");
    currentBlock = entry.get();
    currentFunction->appendBlock(std::move(entry));
    
    for (const auto& param : decl->getParams()) {
        std::string argName = "%" + param->getName();
        func->addParameter(param->getName(), param->getType());
        
        std::string stackName = "%" + param->getName() + ".addr";
        currentBlock->appendInstruction(std::make_unique<AllocaInst>(stackName, param->getType()));
        currentBlock->appendInstruction(std::make_unique<StoreInst>(argName, stackName));
        
        varMap[param->getName()] = stackName;
        if (param->getType()->isStruct()) {
             ptrTypeMap[stackName] = std::dynamic_pointer_cast<StructType>(param->getType())->getName();
        } else if (param->getType()->isPointer()) {
             auto ptr = std::dynamic_pointer_cast<PointerType>(param->getType());
             if (ptr->getBaseType()->isStruct()) {
                 ptrTypeMap[stackName] = std::dynamic_pointer_cast<StructType>(ptr->getBaseType())->getName();
             }
        }
    }
    
    lowerBlock(const_cast<Block*>(decl->getBody()));
    
    if (!currentBlock->hasTerminator()) {
        if (func->getReturnType()->isVoid()) {
            currentBlock->appendInstruction(std::make_unique<ReturnInst>());
        }
        // Methods might return values, but for now we assume implicit return is only for void
    }

    module.appendFunction(std::move(func));
    currentFunction = nullptr;
    currentBlock = nullptr;
}

void MIRBuilder::lowerConstructorDecl(ConstructorDecl* decl, const std::string& className) {
    std::string mangledName = className + "_" + decl->getName(); 
    
    auto func = std::make_unique<MIRFunction>(mangledName, Type::getVoid());
    currentFunction = func.get();
    
    auto entry = std::make_unique<BasicBlock>("entry");
    currentBlock = entry.get();
    currentFunction->appendBlock(std::move(entry));
    
    auto classType = structTypes[className].type;
    auto selfPtrType = std::make_shared<PointerType>(classType);
    
    std::string selfArgName = "%self";
    func->addParameter("self", selfPtrType);
    
    std::string selfStackName = "%self.addr";
    currentBlock->appendInstruction(std::make_unique<AllocaInst>(selfStackName, selfPtrType));
    currentBlock->appendInstruction(std::make_unique<StoreInst>(selfArgName, selfStackName));
    
    varMap["self"] = selfStackName;
    ptrTypeMap[selfStackName] = className;

    for (const auto& param : decl->getParams()) {
        std::string argName = "%" + param->getName();
        func->addParameter(param->getName(), param->getType());
        
        std::string stackName = "%" + param->getName() + ".addr";
        currentBlock->appendInstruction(std::make_unique<AllocaInst>(stackName, param->getType()));
        currentBlock->appendInstruction(std::make_unique<StoreInst>(argName, stackName));
        
        varMap[param->getName()] = stackName;
        if (param->getType()->isStruct()) {
             ptrTypeMap[stackName] = std::dynamic_pointer_cast<StructType>(param->getType())->getName();
        } else if (param->getType()->isPointer()) {
             auto ptr = std::dynamic_pointer_cast<PointerType>(param->getType());
             if (ptr->getBaseType()->isStruct()) {
                 ptrTypeMap[stackName] = std::dynamic_pointer_cast<StructType>(ptr->getBaseType())->getName();
             }
        }
    }
    
    lowerBlock(const_cast<Block*>(decl->getBody()));
    
    if (!currentBlock->hasTerminator()) {
        currentBlock->appendInstruction(std::make_unique<ReturnInst>());
    }

    module.appendFunction(std::move(func));
    currentFunction = nullptr;
    currentBlock = nullptr;
}

void MIRBuilder::pushScope() {
    scopeStack.push_back(Scope());
}

void MIRBuilder::popScope() {
    if (scopeStack.empty()) return;
    
    Scope& scope = scopeStack.back();
    for (auto it = scope.variables.rbegin(); it != scope.variables.rend(); ++it) {
        if (it->type->isClass()) {
            auto classType = std::dynamic_pointer_cast<ClassType>(it->type);
            if (structTypes.count(classType->getName()) && structTypes[classType->getName()].hasDestructor) {
                std::string dtorName = classType->getName() + "_~" + classType->getName();
                std::string addr = varMap[it->name];
                currentBlock->appendInstruction(std::make_unique<CallInst>("", dtorName, std::vector<std::string>{addr}));
            }
        }
        bool wasShadowing = false;
        for (const auto& s : scope.shadowed) {
            if (s.name == it->name) {
                wasShadowing = true;
                break;
            }
        }
        if (!wasShadowing) {
            std::string mirName = varMap[it->name];
            varMap.erase(it->name);
            ptrTypeMap.erase(mirName);
        }
    }

    for (const auto& s : scope.shadowed) {
        varMap[s.name] = s.oldMirName;
        if (!s.oldPtrType.empty()) {
            ptrTypeMap[s.oldMirName] = s.oldPtrType;
        }
    }

    scopeStack.pop_back();
}

void MIRBuilder::emitAllDestructors() {
    for (auto scopeIt = scopeStack.rbegin(); scopeIt != scopeStack.rend(); ++scopeIt) {
        for (auto it = scopeIt->variables.rbegin(); it != scopeIt->variables.rend(); ++it) {
            if (it->type->isClass()) {
                auto classType = std::dynamic_pointer_cast<ClassType>(it->type);
                if (structTypes.count(classType->getName()) && structTypes[classType->getName()].hasDestructor) {
                    std::string dtorName = classType->getName() + "_~" + classType->getName();
                    if (varMap.count(it->name)) {
                        std::string addr = varMap[it->name];
                        currentBlock->appendInstruction(std::make_unique<CallInst>("", dtorName, std::vector<std::string>{addr}));
                    }
                }
            }
        }
    }
}

} // namespace chtholly