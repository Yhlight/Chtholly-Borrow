#include "Sema/Sema.h"
#include "AST/Patterns.h"
#include "AST/ImportDecl.h"
#include "Parser.h"
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <sstream>

namespace chtholly {

Sema::Sema() {
    // Inject built-in Result enum: enum Result[T, E] { Ok(T), Err(E) }
    if (!symbolTable.lookupType("Result")) {
        std::string resultSource = "enum Result[T, E] { Ok(T), Err(E) }";
        Parser parser(resultSource);
        auto program = parser.parseProgram();
        for (auto& node : program) {
            analyze(node.get());
            analyzedNodes.push_back(std::move(node));
        }
    }
}

std::shared_ptr<Type> Sema::resolveType(std::shared_ptr<Type> type) {
    if (!type) return nullptr;
    std::string name = type->toString();
    
    // Fast path for primitives and common pointer types
    if (name == "i8" || name == "i16" || name == "i32" || name == "i64" ||
        name == "u8" || name == "u16" || name == "u32" || name == "u64" ||
        name == "f32" || name == "f64" || name == "bool" || name == "void" || 
        name == "i8*" || name == "i16*" || name == "i32*" || name == "i64*" ||
        name == "u8*" || name == "u16*" || name == "u32*" || name == "u64*" ||
        name == "f32*" || name == "f64*" || name == "bool*" || name == "char" || name == "char*") {
        return type;
    }

    if (type->isPointer()) {
        auto ptr = std::static_pointer_cast<PointerType>(type);
        return std::make_shared<PointerType>(resolveType(ptr->getBaseType()));
    }
    if (type->isArray()) {
        auto arr = std::static_pointer_cast<ArrayType>(type);
        return std::make_shared<ArrayType>(resolveType(arr->getBaseType()), arr->getSize());
    }

    if (type->isStruct() || type->isEnum() || type->getKind() == TypeKind::TypeParameter) {
        // Only try to resolve if it looks like a generic specialization "Base[...]"
        size_t bracketPos = name.find('[');
        if (bracketPos != std::string::npos && name.back() == ']') {
            // Check if it's already monomorphized
            auto existing = symbolTable.lookupType(name);
            if (existing && !existing->isTypeParameter()) {
                if (existing->isStruct()) {
                    auto st = std::static_pointer_cast<StructType>(existing);
                    if (!st->getFields().empty() || st->isClass()) return existing;
                } else if (existing->isEnum()) {
                    auto en = std::static_pointer_cast<EnumType>(existing);
                    if (!en->getVariants().empty()) return existing;
                }
            }

            std::string baseName = name.substr(0, bracketPos);
            std::string argsStr = name.substr(bracketPos + 1, name.size() - bracketPos - 2);
            
            auto baseDecl = symbolTable.lookupTypeDecl(baseName);
            if (baseDecl) {
                std::vector<std::shared_ptr<Type>> typeArgs;
                std::stringstream ss(argsStr);
                std::string arg;
                while (std::getline(ss, arg, ',')) {
                    arg.erase(0, arg.find_first_not_of(' '));
                    arg.erase(arg.find_last_not_of(' ') + 1);
                    

                    std::shared_ptr<Type> argTy;
                    if (arg == "i8") argTy = Type::getI8();
                    else if (arg == "i16") argTy = Type::getI16();
                    else if (arg == "i32") argTy = Type::getI32();
                    else if (arg == "i64") argTy = Type::getI64();
                    else if (arg == "u8") argTy = Type::getU8();
                    else if (arg == "u16") argTy = Type::getU16();
                    else if (arg == "u32") argTy = Type::getU32();
                    else if (arg == "u64") argTy = Type::getU64();
                    else if (arg == "f32") argTy = Type::getF32();
                    else if (arg == "f64") argTy = Type::getF64();
                    else if (arg == "bool") argTy = Type::getBool();
                    else if (arg == "void") argTy = Type::getVoid();
                    else if (arg == "i8*") argTy = Type::getI8Ptr();
                    else if (arg.back() == '*') {
                         std::string base = arg.substr(0, arg.size()-1);
                         argTy = std::make_shared<PointerType>(resolveType(std::make_shared<StructType>(base, std::vector<StructType::Field>{})));
                    } else {
                         argTy = resolveType(std::make_shared<StructType>(arg, std::vector<StructType::Field>{}));
                    }
                    typeArgs.push_back(argTy);
                }


                if (baseDecl->getKind() == ASTNodeKind::StructDecl) {
                    return monomorphizeStruct(static_cast<StructDecl*>(baseDecl), typeArgs);
                } else if (baseDecl->getKind() == ASTNodeKind::EnumDecl) {
                    return monomorphizeEnum(static_cast<EnumDecl*>(baseDecl), typeArgs);
                } else if (baseDecl->getKind() == ASTNodeKind::ClassDecl) {
                    return monomorphizeClass(static_cast<ClassDecl*>(baseDecl), typeArgs);
                }
            }
        }
        
        // If not a specialized name, or base not found, just return existing type if it's already resolved in symbol table
        auto existing = symbolTable.lookupType(name);
        if (existing) return existing;
    }

    return type;
}

void Sema::analyze(ASTNode* node) {
    if (!node) return;
    try {
        switch (node->getKind()) {
            case ASTNodeKind::VarDecl:
                analyzeVarDecl(static_cast<VarDecl*>(node));
                break;
            case ASTNodeKind::StructDecl:
                analyzeStructDecl(static_cast<StructDecl*>(node));
                break;
            case ASTNodeKind::EnumDecl:
                analyzeEnumDecl(static_cast<EnumDecl*>(node));
                break;
            case ASTNodeKind::ClassDecl:
                analyzeClassDecl(static_cast<ClassDecl*>(node));
                break;
            case ASTNodeKind::ImportDecl:
                analyzeImportDecl(static_cast<ImportDecl*>(node));
                break;
            case ASTNodeKind::PackageDecl:
                analyzePackageDecl(static_cast<PackageDecl*>(node));
                break;
            case ASTNodeKind::UseDecl:
                analyzeUseDecl(static_cast<UseDecl*>(node));
                break;
            case ASTNodeKind::RequestDecl:
                analyzeRequestDecl(static_cast<RequestDecl*>(node));
                break;
            case ASTNodeKind::Block:
                analyzeBlock(static_cast<Block*>(node));
                break;
            case ASTNodeKind::FunctionDecl:
                analyzeFunctionDecl(static_cast<FunctionDecl*>(node));
                break;
            case ASTNodeKind::IfStmt:
                analyzeIfStmt(static_cast<IfStmt*>(node));
                break;
            case ASTNodeKind::WhileStmt:
                analyzeWhileStmt(static_cast<WhileStmt*>(node));
                break;
            case ASTNodeKind::DoWhileStmt:
                analyzeDoWhileStmt(static_cast<DoWhileStmt*>(node));
                break;
            case ASTNodeKind::ForStmt:
                analyzeForStmt(static_cast<ForStmt*>(node));
                break;
            case ASTNodeKind::SwitchStmt:
                analyzeSwitchStmt(static_cast<SwitchStmt*>(node));
                break;
            case ASTNodeKind::ReturnStmt:
                analyzeReturnStmt(static_cast<ReturnStmt*>(node));
                break;
            case ASTNodeKind::BreakStmt:
                analyzeBreakStmt(static_cast<BreakStmt*>(node));
                break;
            case ASTNodeKind::ContinueStmt:
                analyzeContinueStmt(static_cast<ContinueStmt*>(node));
                break;
            case ASTNodeKind::ExprStmt:
                analyzeExprStmt(static_cast<ExprStmt*>(node));
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
            case ASTNodeKind::QuestionExpr:
                checkExpr(static_cast<Expr*>(node));
                break;
            default:
                break;
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception in Sema::analyze for node: " << node->toString().substr(0, 50) << " : " << e.what() << std::endl;
        throw;
    }
}

void Sema::analyzeIfStmt(IfStmt* stmt) {
    auto condType = checkExpr(const_cast<Expr*>(stmt->getCondition()));
    if (!condType->equals(*Type::getBool())) {
        throw std::runtime_error("Condition must be of type bool, got " + condType->toString());
    }
    analyzeBlock(const_cast<Block*>(stmt->getThenBlock()));
    if (stmt->getElseBlock()) {
        analyzeBlock(const_cast<Block*>(stmt->getElseBlock()));
    }
}

void Sema::analyzeWhileStmt(WhileStmt* stmt) {
    auto condType = checkExpr(const_cast<Expr*>(stmt->getCondition()));
    if (!condType->equals(*Type::getBool())) {
        throw std::runtime_error("Condition must be of type bool, got " + condType->toString());
    }
    loopDepth++;
    analyzeBlock(const_cast<Block*>(stmt->getBody()));
    loopDepth--;
}

void Sema::analyzeDoWhileStmt(DoWhileStmt* stmt) {
    loopDepth++;
    analyzeBlock(const_cast<Block*>(stmt->getBody()));
    loopDepth--;
    auto condType = checkExpr(const_cast<Expr*>(stmt->getCondition()));
    if (!condType->equals(*Type::getBool())) {
        throw std::runtime_error("Condition must be of type bool, got " + condType->toString());
    }
}

void Sema::analyzeForStmt(ForStmt* stmt) {
    symbolTable.pushScope();
    if (stmt->getInit()) analyze(const_cast<Stmt*>(stmt->getInit()));
    
    if (stmt->getCondition()) {
        auto condType = checkExpr(const_cast<Expr*>(stmt->getCondition()));
        if (!condType->equals(*Type::getBool())) {
            throw std::runtime_error("Condition must be of type bool, got " + condType->toString());
        }
    }
    
    if (stmt->getStep()) checkExpr(const_cast<Expr*>(stmt->getStep()));
    
    loopDepth++;
    analyzeBlock(const_cast<Block*>(stmt->getBody()));
    loopDepth--;
    
    symbolTable.popScope();
}

void Sema::analyzeSwitchStmt(SwitchStmt* stmt) {
    auto condType = checkExpr(const_cast<Expr*>(stmt->getCondition()));
    
    switchDepth++;
    for (const auto& c : stmt->getCases()) {
        symbolTable.pushScope(); // Each case has its own scope
        if (!c->isDefaultCase()) {
            analyzePattern(const_cast<Pattern*>(c->getPattern()), condType);
        }
        analyzeBlock(const_cast<Block*>(c->getBody()));
        symbolTable.popScope();
    }
    switchDepth--;
}

void Sema::analyzePattern(Pattern* pattern, std::shared_ptr<Type> matchType) {
    if (!pattern) return;
    switch (pattern->getKind()) {
        case ASTNodeKind::LiteralPattern: { 
            auto litPat = static_cast<LiteralPattern*>(pattern);
            auto litType = checkExpr(const_cast<LiteralExpr*>(litPat->getLiteral()));
            if (!litType->equals(*matchType)) {
                throw std::runtime_error("Pattern type mismatch: expected " + matchType->toString() + ", got " + litType->toString());
            }
            break;
        }
        case ASTNodeKind::IdentifierPattern: {
            auto idPat = static_cast<IdentifierPattern*>(pattern);
            if (!symbolTable.insert(idPat->getName(), matchType, false)) {
                throw std::runtime_error("Variable '" + idPat->getName() + "' already defined in this scope");
            }
            break;
        }
        case ASTNodeKind::VariantPattern: {
            auto varPat = static_cast<VariantPattern*>(pattern);
            if (!matchType->isEnum()) {
                throw std::runtime_error("Cannot match variant against non-enum type: " + matchType->toString());
            }
            auto enumTy = std::dynamic_pointer_cast<EnumType>(matchType);
            
            // Resolve enum name if empty (short-hand)
            std::string enumName = varPat->getEnumName();
            if (enumName.empty()) enumName = enumTy->getName();
            else if (enumName != enumTy->getName()) {
                // Heuristic: check if enumName (e.g. Option[i32]) matches enumTy->getName() (e.g. Option_i32)
                std::string canonical = enumName;
                size_t p;
                while ((p = canonical.find('*')) != std::string::npos) {
                    canonical.replace(p, 1, "Ptr");
                }
                for (char& c : canonical) {
                    if (c == '[' || c == ']' || c == ',' || c == ' ') c = '_';
                }
                // Collapse multiple underscores and strip trailing ones
                std::string collapsed;
                for (char c : canonical) {
                    if (c == '_' && !collapsed.empty() && collapsed.back() == '_') continue;
                    collapsed += c;
                }
                canonical = collapsed;
                while (!canonical.empty() && canonical.back() == '_') canonical.pop_back();
                
                std::string mangled = enumTy->getName();
                while (!mangled.empty() && mangled.back() == '_') mangled.pop_back();

                if (canonical != mangled && enumTy->getName().find(enumName + "_") != 0) {
                    throw std::runtime_error("Enum type mismatch: expected " + enumTy->getName() + ", got " + enumName);
                }
            }

            auto* variant = enumTy->findVariant(varPat->getVariantName());
            if (!variant) {
                throw std::runtime_error("Variant '" + varPat->getVariantName() + "' not found in enum '" + enumName + "'");
            }

            const auto& subPats = varPat->getSubPatterns();
            if (variant->kind == EnumType::Variant::Kind::Unit) {
                if (!subPats.empty()) throw std::runtime_error("Unit variant '" + variant->name + "' expected 0 patterns, got " + std::to_string(subPats.size()));
            } else if (variant->kind == EnumType::Variant::Kind::Tuple) {
                if (subPats.size() != variant->tupleTypes.size()) {
                    throw std::runtime_error("Tuple variant '" + variant->name + "' expected " + std::to_string(variant->tupleTypes.size()) + " patterns, got " + std::to_string(subPats.size()));
                }
                for (size_t i = 0; i < subPats.size(); ++i) {
                    analyzePattern(subPats[i].get(), variant->tupleTypes[i]);
                }
            } else { // Struct
                if (subPats.size() != variant->structFields.size()) {
                    throw std::runtime_error("Struct variant '" + variant->name + "' expected " + std::to_string(variant->structFields.size()) + " patterns, got " + std::to_string(subPats.size()));
                }
                for (size_t i = 0; i < subPats.size(); ++i) {
                    analyzePattern(subPats[i].get(), variant->structFields[i].type);
                }
            }
            break;
        }
        case ASTNodeKind::WildcardPattern:
            // Wildcard matches anything, no bindings
            break;
        default:
            break;
    }
}

void Sema::analyzeReturnStmt(ReturnStmt* stmt) {
    std::shared_ptr<Type> retType = Type::getVoid();
    if (stmt->getExpression()) {
        retType = checkExpr(const_cast<Expr*>(stmt->getExpression()));
    }
    
    std::shared_ptr<Type> expectedTy = nullptr;
    if (currentFunction) {
        expectedTy = currentFunction->getReturnType();
    } else if (currentMethod) {
        expectedTy = currentMethod->getReturnType();
    }

    if (expectedTy && !retType->equals(*expectedTy)) {
        throw std::runtime_error("Return type mismatch: expected " +
                                 expectedTy->toString() + ", got " + retType->toString());
    }
}

void Sema::analyzeBreakStmt(BreakStmt* stmt) {
    if (loopDepth == 0 && switchDepth == 0) {
        throw std::runtime_error("Break statement outside of loop or switch");
    }
}

void Sema::analyzeContinueStmt(ContinueStmt* stmt) {
    if (loopDepth == 0) {
        throw std::runtime_error("Continue statement outside of loop");
    }
}

void Sema::analyzeExprStmt(ExprStmt* stmt) {
    checkExpr(const_cast<Expr*>(stmt->getExpression()));
}

void Sema::analyzeBlock(Block* block) {
    symbolTable.pushScope();
    for (auto& stmt : block->getStatements()) {
        analyze(stmt.get());
    }
    symbolTable.popScope();
}

void Sema::analyzeRequestDecl(RequestDecl* decl) {
    if (!decl) return;

    // Register the request in the symbol table
    if (!symbolTable.insertTypeGlobal(decl->getName(), nullptr, decl->isPublic(), decl)) {
        throw std::runtime_error("Request '" + decl->getName() + "' already defined");
    }

    symbolTable.pushScope();

    // Register generic params
    for (const auto& param : decl->getGenericParams()) {
        auto paramType = std::make_shared<TypeParameterType>(param.name);
        symbolTable.insertType(param.name, paramType);
    }

    // Process members (prototypes)
    for (const auto& member : decl->getMembers()) {
        if (member.decl->getKind() == ASTNodeKind::MethodDecl) {
            auto method = static_cast<MethodDecl*>(member.decl.get());
            // Resolve types in method signature
            for (const auto& param : method->getParams()) {
                param->setType(resolveType(param->getType()));
            }
            method->setReturnType(resolveType(method->getReturnType()));
        }
    }

    symbolTable.popScope();
}

void Sema::analyzeStructDecl(StructDecl* decl) {
    std::vector<StructType::Field> fields;
    for (const auto& member : decl->getMembers()) {
        if (member->getKind() == ASTNodeKind::VarDecl) {
            auto varDecl = static_cast<VarDecl*>(member.get());
            varDecl->setType(resolveType(varDecl->getType()));
            fields.push_back({varDecl->getName(), varDecl->getType(), varDecl->isPublic()});
        }
    }
    
    auto structType = std::make_shared<StructType>(decl->getName(), std::move(fields));
    if (!symbolTable.insertTypeGlobal(decl->getName(), structType, decl->isPublic(), decl)) {
        auto existing = symbolTable.lookupType(decl->getName());
        if (!existing || !existing->equals(*structType)) {
            throw std::runtime_error("Redefinition of type '" + decl->getName() + "'");
        }
    }

    if (!decl->getGenericParams().empty()) {
        return;
    }
}

void Sema::analyzeEnumDecl(EnumDecl* decl) {
    std::vector<EnumType::Variant> variants;
    for (const auto& v : decl->getVariants()) {
        EnumType::Variant variant;
        variant.name = v->getName();
        if (v->getVariantKind() == EnumVariant::VariantKind::Unit) {
            variant.kind = EnumType::Variant::Kind::Unit;
        } else if (v->getVariantKind() == EnumVariant::VariantKind::Tuple) {
            variant.kind = EnumType::Variant::Kind::Tuple;
            std::vector<std::shared_ptr<Type>> resolvedTypes;
            for (auto t : v->getTupleTypes()) resolvedTypes.push_back(resolveType(t));
            v->setTupleTypes(resolvedTypes);
            variant.tupleTypes = resolvedTypes;
        } else {
            variant.kind = EnumType::Variant::Kind::Struct;
            for (const auto& f : v->getStructFields()) {
                f->setType(resolveType(f->getType()));
                variant.structFields.push_back({f->getName(), f->getType(), true});
            }
        }
        variants.push_back(std::move(variant));
    }

    auto enumType = std::make_shared<EnumType>(decl->getName(), std::move(variants));
    decl->setType(enumType);
    registeredEnums.push_back(enumType);
    if (!symbolTable.insertTypeGlobal(decl->getName(), enumType, decl->isPublic(), decl)) {
        throw std::runtime_error("Redefinition of type '" + decl->getName() + "'");
    }

    if (!decl->getGenericParams().empty()) {
        return;
    }
}

void Sema::analyzeFunctionDecl(FunctionDecl* decl) {
    if (!decl) return;
    std::vector<std::shared_ptr<Type>> paramTypes;
    for (const auto& param : decl->getParams()) {
        param->setType(resolveType(param->getType()));
        paramTypes.push_back(param->getType());
    }
    decl->setReturnType(resolveType(decl->getReturnType()));
    auto funcType = std::make_shared<FunctionType>(std::move(paramTypes), decl->getReturnType(), decl->getVarArg());
    
    if (!symbolTable.insertGlobal(decl->getName(), funcType, false, decl->getIsPublic(), decl)) {
        // If it's already there and has the same type, it's fine (monomorphization cache)
        auto existing = symbolTable.lookup(decl->getName());
        if (!existing || !existing->type->equals(*funcType)) {
            throw std::runtime_error("Redefinition of function '" + decl->getName() + "'");
        }
    }

    // Skip generic functions - they will be analyzed when monomorphized
    if (!decl->getGenericParams().empty()) {
        return;
    }

    auto oldFunc = currentFunction;
    currentFunction = decl;
    symbolTable.pushScope();
    for (const auto& param : decl->getParams()) {
        if (!symbolTable.insert(param->getName(), param->getType(), false)) {
            throw std::runtime_error("Redefinition of parameter '" + param->getName() + "'");
        }
    }
    
    if (!decl->getIsExtern()) {
        analyzeBlock(const_cast<Block*>(decl->getBody()));
    }
    symbolTable.popScope();
    currentFunction = oldFunc;
}

void Sema::analyzeVarDecl(VarDecl* decl) {
    if (!decl) return;
    std::shared_ptr<Type> type = resolveType(decl->getType());
    decl->setType(type);
    if (decl->getInitializer()) {
        auto initType = checkExpr(const_cast<Expr*>(decl->getInitializer()));
        if (!type) {
            type = initType;
            decl->setType(type);
        } else if (!type->equals(*initType)) {
            throw std::runtime_error("Type mismatch in variable declaration: expected " +
                                     type->toString() + ", got " + initType->toString());
        }

        // Move semantics: if RHS is an identifier and type is not Copy, mark moved
        if (!type->isCopyType()) {
            if (decl->getInitializer()->getKind() == ASTNodeKind::IdentifierExpr) {
                auto id = static_cast<const IdentifierExpr*>(decl->getInitializer());
                symbolTable.markMoved(id->toString());
            }
        }
    }

    if (!symbolTable.insert(decl->getName(), type, decl->isMutable(), decl->isPublic())) {
        throw std::runtime_error("Redefinition of variable '" + decl->getName() + "'");
    }
}

std::shared_ptr<Type> Sema::checkExpr(Expr* expr) {
    if (!expr) return Type::getVoid();
    std::shared_ptr<Type> res = nullptr;
    switch (expr->getKind()) {
        case ASTNodeKind::LiteralExpr:
            res = checkLiteralExpr(static_cast<LiteralExpr*>(expr));
            break;
        case ASTNodeKind::IdentifierExpr:
            res = checkIdentifierExpr(static_cast<IdentifierExpr*>(expr));
            break;
        case ASTNodeKind::BinaryExpr:
            res = checkBinaryExpr(static_cast<BinaryExpr*>(expr));
            break;
        case ASTNodeKind::UnaryExpr:
            res = checkUnaryExpr(static_cast<UnaryExpr*>(expr));
            break;
        case ASTNodeKind::CallExpr:
            res = checkCallExpr(static_cast<CallExpr*>(expr));
            break;
        case ASTNodeKind::MemberAccessExpr:
            res = checkMemberAccess(static_cast<MemberAccessExpr*>(expr));
            break;
        case ASTNodeKind::StructLiteralExpr:
            res = checkStructLiteral(static_cast<StructLiteralExpr*>(expr));
            break;
        case ASTNodeKind::ArrayLiteralExpr:
            res = checkArrayLiteral(static_cast<ArrayLiteralExpr*>(expr));
            break;
        case ASTNodeKind::IndexingExpr:
            res = checkIndexing(static_cast<IndexingExpr*>(expr));
            break;
        case ASTNodeKind::AddressOfExpr:
            res = checkAddressOf(static_cast<AddressOfExpr*>(expr));
            break;
        case ASTNodeKind::DereferenceExpr:
            res = checkDereference(static_cast<DereferenceExpr*>(expr));
            break;
        case ASTNodeKind::IntrinsicExpr:
            res = checkIntrinsic(static_cast<IntrinsicExpr*>(expr));
            break;
        case ASTNodeKind::QuestionExpr:
            res = checkQuestionExpr(static_cast<QuestionExpr*>(expr));
            break;
        case ASTNodeKind::SpecializationExpr:
            res = checkSpecializationExpr(static_cast<SpecializationExpr*>(expr));
            break;
        default:
            res = Type::getVoid();
            break;
    }
    
    if (res) {
        expr->setType(res);
    }
    return res;
}

std::shared_ptr<Type> Sema::checkArrayLiteral(ArrayLiteralExpr* expr) {
    if (expr->getElements().empty()) {
        throw std::runtime_error("Empty array literals are not supported yet");
    }
    auto baseType = checkExpr(expr->getElements()[0].get());
    for (size_t i = 1; i < expr->getElements().size(); ++i) {
        auto elType = checkExpr(expr->getElements()[i].get());
        if (!elType->equals(*baseType)) {
            throw std::runtime_error("Array literal elements must have the same type");
        }
    }
    return std::make_shared<ArrayType>(baseType, (int)expr->getElements().size());
}

std::shared_ptr<Type> Sema::checkIndexing(IndexingExpr* expr) {
    auto baseType = checkExpr(const_cast<Expr*>(expr->getBase()));
    auto indexType = checkExpr(const_cast<Expr*>(expr->getIndex()));

    if (!indexType->isInteger()) {
        throw std::runtime_error("Array index must be an integer");
    }

    if (baseType->isArray()) {
        return std::static_pointer_cast<ArrayType>(baseType)->getBaseType();
    } else if (baseType->isPointer()) {
        return std::static_pointer_cast<PointerType>(baseType)->getBaseType();
    } else {
        throw std::runtime_error("Cannot index non-array/pointer type: " + baseType->toString());
    }
}

std::shared_ptr<Type> Sema::checkAddressOf(AddressOfExpr* expr) {
    auto opType = checkExpr(const_cast<Expr*>(expr->getOperand()));
    return std::make_shared<PointerType>(opType);
}

std::shared_ptr<Type> Sema::checkDereference(DereferenceExpr* expr) {
    auto opType = checkExpr(const_cast<Expr*>(expr->getOperand()));
    if (!opType->isPointer()) {
        throw std::runtime_error("Cannot dereference non-pointer type: " + opType->toString());
    }
    return std::static_pointer_cast<PointerType>(opType)->getBaseType();
}

std::shared_ptr<Type> Sema::checkIntrinsic(IntrinsicExpr* expr) {
    if (expr->getTypeArg()) {
        auto resolved = symbolTable.lookupType(expr->getTypeArg()->toString());
        if (resolved) expr->setTypeArg(resolved);
    }
    switch (expr->getIntrinsicKind()) {
        case IntrinsicExpr::IntrinsicKind::Sizeof:
        case IntrinsicExpr::IntrinsicKind::Alignof:
        case IntrinsicExpr::IntrinsicKind::Offsetof:
            return Type::getI64();
        case IntrinsicExpr::IntrinsicKind::Malloc:
        case IntrinsicExpr::IntrinsicKind::Alloca:
            return std::make_shared<PointerType>(expr->getTypeArg());
        case IntrinsicExpr::IntrinsicKind::Free:
            return Type::getVoid();
    }
    return Type::getVoid();
}

std::shared_ptr<Type> Sema::checkLiteralExpr(LiteralExpr* expr) {
    if (expr->getExplicitType()) return expr->getExplicitType();
    return std::visit([](auto&& arg) -> std::shared_ptr<Type> {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, bool>) return Type::getBool();
        else if constexpr (std::is_same_v<T, std::string>) return Type::getI8Ptr();
        else if constexpr (std::is_same_v<T, double>) return Type::getF64();
        else if constexpr (std::is_same_v<T, std::nullptr_t>) return std::make_shared<PointerType>(Type::getVoid());
        else return Type::getI32();
    }, expr->getValue());
}

std::shared_ptr<Type> Sema::checkIdentifierExpr(IdentifierExpr* expr) {
    auto symbol = symbolTable.lookupIgnoreMoved(expr->toString());
    if (symbol) {
        if (symbol->isMoved) {
            throw std::runtime_error("Use of moved variable: " + expr->toString());
        }
        return symbol->type;
    }

    // Check if it's a type name
    auto type = symbolTable.lookupType(expr->toString());
    if (type) {
        return type;
    }

    // Check if it's an enum variant
    for (const auto& enumTy : registeredEnums) {
        if (enumTy->findVariant(expr->toString())) {
            return enumTy;
        }
    }

    throw std::runtime_error("Undefined identifier: " + expr->toString());
}

std::shared_ptr<Type> Sema::checkBinaryExpr(BinaryExpr* expr) {
    if (expr->getOp() == TokenType::Equal) {
        // Special case for assignment: evaluate RHS first
        auto rightType = checkExpr(const_cast<Expr*>(expr->getRight()));
        
        std::shared_ptr<Type> leftType = nullptr;
        // If LHS is a simple identifier, allow it to be moved
        if (expr->getLeft()->getKind() == ASTNodeKind::IdentifierExpr) {
            auto id = static_cast<const IdentifierExpr*>(expr->getLeft());
            auto sym = symbolTable.lookupIgnoreMoved(id->toString());
            if (!sym) {
                throw std::runtime_error("Undefined identifier: " + id->toString());
            }
            leftType = sym->type;
            // Mark as accessed (re-initialized)
            symbolTable.markAccessed(id->toString());
        } else {
            leftType = checkExpr(const_cast<Expr*>(expr->getLeft()));
        }

        if (!leftType->equals(*rightType)) {
            throw std::runtime_error("Type mismatch in assignment: expected " +
                                     leftType->toString() + ", got " + rightType->toString());
        }

        // Move the RHS if it's an identifier and not Copy
        if (!rightType->isCopyType()) {
            if (expr->getRight()->getKind() == ASTNodeKind::IdentifierExpr) {
                auto id = static_cast<const IdentifierExpr*>(expr->getRight());
                symbolTable.markMoved(id->toString());
            }
        }
        return leftType;
    }

    auto leftType = checkExpr(const_cast<Expr*>(expr->getLeft()));
    auto rightType = checkExpr(const_cast<Expr*>(expr->getRight()));

    if (!leftType->equals(*rightType)) {
        throw std::runtime_error("Type mismatch in binary expression: " +
                                 leftType->toString() + " and " + rightType->toString());
    }

    switch (expr->getOp()) {
        case TokenType::EqualEqual:
        case TokenType::NotEqual:
        case TokenType::Greater:
        case TokenType::GreaterEqual:
        case TokenType::Less:
        case TokenType::LessEqual:
        case TokenType::AndAnd:
        case TokenType::OrOr:
            return Type::getBool();
        default:
            return leftType;
    }
}

std::shared_ptr<Type> Sema::checkUnaryExpr(UnaryExpr* expr) {
    auto operandType = checkExpr(const_cast<Expr*>(expr->getOperand()));

    switch (expr->getOp()) {
        case TokenType::Minus:
            if (!operandType->isInteger() && !operandType->isFloat()) {
                throw std::runtime_error("Unary minus requires numeric type, got: " + operandType->toString());
            }
            return operandType;
        case TokenType::Not:
            if (!operandType->isBool()) {
                throw std::runtime_error("Unary not requires boolean type, got: " + operandType->toString());
            }
            return operandType;
        case TokenType::Tilde:
            if (!operandType->isInteger()) {
                throw std::runtime_error("Unary tilde requires integer type, got: " + operandType->toString());
            }
            return operandType;
        case TokenType::Plus:
            if (!operandType->isInteger() && !operandType->isFloat()) {
                throw std::runtime_error("Unary plus requires numeric type, got: " + operandType->toString());
            }
            return operandType;
        default:
            throw std::runtime_error("Unknown unary operator: " + std::string(tokenTypeToString(expr->getOp())));
    }
}

std::shared_ptr<Type> Sema::checkCallExpr(CallExpr* expr) {
    auto calleeExpr = expr->getCallee();
    
    // Special handling for enum variant construction
    if (calleeExpr->getKind() == ASTNodeKind::IdentifierExpr) {
        auto id = static_cast<const IdentifierExpr*>(calleeExpr);
        if (auto type = symbolTable.lookupType(id->toString())) {
            if (type->isStruct() || type->isClass()) {
                // Class or Struct constructor
                for (const auto& arg : expr->getArgs()) {
                    checkExpr(arg.get());
                }
                return type;
            }
        }
        for (const auto& enumTy : registeredEnums) {
            if (auto* variant = enumTy->findVariant(id->toString())) {
                // It's a variant constructor. 
                // TODO: Verify argument types for Tuple and Struct variants.
                for (const auto& arg : expr->getArgs()) {
                    checkExpr(arg.get());
                }
                return enumTy;
            }
        }
    } else if (calleeExpr->getKind() == ASTNodeKind::MemberAccessExpr) {
        auto memAccess = static_cast<const MemberAccessExpr*>(calleeExpr);
        auto baseType = checkExpr(const_cast<Expr*>(memAccess->getBase()));
        if (baseType->isPointer()) baseType = std::static_pointer_cast<PointerType>(baseType)->getBaseType();

        if (auto enumTy = std::dynamic_pointer_cast<EnumType>(baseType)) {
            if (enumTy->findVariant(memAccess->getMemberName())) {
                for (const auto& arg : expr->getArgs()) {
                    checkExpr(arg.get());
                }
                return enumTy;
            }
        }
    }

    auto calleeType = checkExpr(const_cast<Expr*>(calleeExpr));
    
    if (calleeType->getKind() != TypeKind::Function) {
        throw std::runtime_error("Called object is not a function: " + calleeType->toString());
    }
    
    auto funcType = std::static_pointer_cast<FunctionType>(calleeType);
    const auto& paramTypes = funcType->getParamTypes();
    const auto& args = expr->getArgs();

    bool isMethodCall = false;
    if (calleeExpr->getKind() == ASTNodeKind::MemberAccessExpr) {
        // If it's a member access, and the function expects one more argument than provided,
        // we assume it's a method call with implicit self.
        // TODO: This heuristic might be weak if we have function fields in structs.
        // But for now, we assume member access returning function is a method.
        // We should check if the MemberAccessExpr actually resolved to a method in checkMemberAccess.
        // But checkMemberAccess just returns Type.
        // We can check if paramTypes.size() == args.size() + 1.
        if (paramTypes.size() == args.size() + 1) {
            isMethodCall = true;
        }
    }

    size_t expectedArgs = paramTypes.size();
    size_t providedArgs = args.size();
    
    if (isMethodCall) {
        providedArgs++; // Implicit self
    }

    if (providedArgs != expectedArgs) {
        if (!funcType->isVarArg() || providedArgs < expectedArgs) {
            std::cout << "DEBUG: Call to " << calleeExpr->toString() << " failed. isVarArg=" << funcType->isVarArg() 
                      << " expected=" << expectedArgs << " provided=" << providedArgs << std::endl;
            throw std::runtime_error("Argument count mismatch: expected " + std::to_string(expectedArgs) +
                                     ", got " + std::to_string(providedArgs));
        }
    }

    size_t paramIdx = 0;
    if (isMethodCall) {
        // Check self type
        auto memAccess = static_cast<const MemberAccessExpr*>(calleeExpr);
        auto baseType = checkExpr(const_cast<Expr*>(memAccess->getBase()));
        auto expectedSelfType = paramTypes[0];
        
        // Handle pointer/value mismatch for self
        // If expected is PointerType(Self) and base is StructType(Self), it's OK (implicit ref).
        // If expected is StructType(Self) and base is StructType(Self), OK (copy).
        // If expected is StructType(Self) and base is PointerType(Self), OK (implicit deref).
        
        // Simplified check:
        // TODO: Strict check
        paramIdx++;
    }

    for (size_t i = 0; i < args.size(); ++i) {
        auto argType = checkExpr(args[i].get());
        if (paramIdx < paramTypes.size()) {
            if (!argType->equals(*paramTypes[paramIdx])) {
                 // Allow strict checks later
                 // throw std::runtime_error("Argument type mismatch...");
            }
            paramIdx++;
        }
    }

    return funcType->getReturnType();
}

std::shared_ptr<Type> Sema::checkMemberAccess(MemberAccessExpr* expr) {
    // Check if it's a module access like lib::add
    if (expr->getBase()->getKind() == ASTNodeKind::IdentifierExpr) {
        auto id = static_cast<const IdentifierExpr*>(expr->getBase());
        std::string baseName = id->toString();
        if (modules.count(baseName)) {
            auto moduleTable = modules.at(baseName);
            auto publicSymbols = moduleTable->getPublicSymbols();
            if (publicSymbols.count(expr->getMemberName())) {
                return publicSymbols.at(expr->getMemberName())->type;
            }
            auto publicTypes = moduleTable->getPublicTypes();
            if (publicTypes.count(expr->getMemberName())) {
                return publicTypes.at(expr->getMemberName());
            }
            throw std::runtime_error("Module '" + baseName + "' has no public member named '" + expr->getMemberName() + "'");
        }
    }

    auto baseType = checkExpr(const_cast<Expr*>(expr->getBase()));
    
    if (baseType->isPointer()) {
        baseType = std::dynamic_pointer_cast<PointerType>(baseType)->getBaseType();
    }

    if (auto enumTy = std::dynamic_pointer_cast<EnumType>(baseType)) {
        if (enumTy->findVariant(expr->getMemberName())) {
            return enumTy;
        }
    }

    if (!baseType->isStruct()) {
        throw std::runtime_error("Member access on non-struct type: " + baseType->toString());
    }
    
    auto structType = std::dynamic_pointer_cast<StructType>(baseType);
    for (const auto& field : structType->getFields()) {
        if (field.name == expr->getMemberName()) {
            if (!field.isPublic && currentClass != structType) {
                throw std::runtime_error("Cannot access private field '" + field.name + "' of class '" + structType->getName() + "'");
            }
            return field.type;
        }
    }
    
    for (const auto& method : structType->getMethods()) {
        if (method.name == expr->getMemberName()) {
            if (!method.isPublic && currentClass != structType) {
                throw std::runtime_error("Cannot access private method '" + method.name + "' of class '" + structType->getName() + "'");
            }
            return method.type;
        }
    }
    
    throw std::runtime_error("Struct '" + structType->getName() + "' has no member named '" + expr->getMemberName() + "'");
}

std::shared_ptr<Type> Sema::checkStructLiteral(StructLiteralExpr* expr) {
    auto baseType = checkExpr(const_cast<Expr*>(expr->getBase()));
    
    if (!baseType) {
        throw std::runtime_error("Undefined type in struct literal");
    }

    if (auto enumTy = std::dynamic_pointer_cast<EnumType>(baseType)) {
        // Handle Enum::Variant syntax if it resolved to an EnumType via checkMemberAccess
        // This usually happens when the base is a MemberAccessExpr like Result::Ok
        // We need to find which variant was used.
        if (expr->getBase()->getKind() == ASTNodeKind::MemberAccessExpr) {
            auto memAccess = static_cast<const MemberAccessExpr*>(expr->getBase());
            std::string variantName = memAccess->getMemberName();
            if (auto variant = enumTy->findVariant(variantName)) {
                if (variant->kind != EnumType::Variant::Kind::Struct) {
                    throw std::runtime_error("Variant '" + variantName + "' is not a struct variant");
                }
                
                // Verify fields
                const auto& inits = expr->getFields();
                if (inits.size() != variant->structFields.size()) {
                     throw std::runtime_error("Field count mismatch for variant '" + variantName + "'");
                }
                for (const auto& f : variant->structFields) {
                    bool found = false;
                    for (const auto& init : inits) {
                        if (init.name == f.name) {
                            found = true;
                            auto initType = checkExpr(const_cast<Expr*>(init.value.get()));
                            if (!initType->equals(*f.type)) {
                                 throw std::runtime_error("Type mismatch for field '" + f.name + "': expected " + f.type->toString() + ", got " + initType->toString());
                            }
                            break;
                        }
                    }
                    if (!found) throw std::runtime_error("Missing field '" + f.name + "'");
                }
                return enumTy;
            }
        }
        return enumTy;
    }

    if (!baseType->isStruct()) {
        throw std::runtime_error("'" + expr->getBase()->toString() + "' is not a struct type");
    }
    
    auto structType = std::dynamic_pointer_cast<StructType>(baseType);
    const auto& fields = structType->getFields();
    const auto& inits = expr->getFields();
    
    if (inits.size() != fields.size()) {
        throw std::runtime_error("Field count mismatch for struct '" + structType->getName() +
                                 "': expected " + std::to_string(fields.size()) +
                                 ", got " + std::to_string(inits.size()));
    }
    
    for (const auto& field : fields) {
        bool found = false;
        for (const auto& init : inits) {
            if (init.name == field.name) {
                found = true;
                auto initType = checkExpr(const_cast<Expr*>(init.value.get()));
                if (!initType->equals(*field.type)) {
                    throw std::runtime_error("Type mismatch for field '" + field.name + "' in struct '" +
                                             structType->getName() + "': expected " + field.type->toString() +
                                             ", got " + initType->toString());
                }
                break;
            }
        }
        if (!found) {
            throw std::runtime_error("Missing field '" + field.name + "' in initializer for struct '" +
                                     structType->getName() + "'");
        }
    }
    
    return structType;
}

void Sema::analyzeClassDecl(ClassDecl* decl) {
    // 1. Gather fields
    std::vector<StructType::Field> fields;
    for (const auto& member : decl->getMembers()) {
        if (auto varDecl = dynamic_cast<VarDecl*>(member.get())) {
             if (!varDecl->getType()) {
                 throw std::runtime_error("Field '" + varDecl->getName() + "' must have an explicit type");
             }
             varDecl->setType(resolveType(varDecl->getType()));
             fields.push_back({varDecl->getName(), varDecl->getType(), varDecl->isPublic()});
        }
    }
    
    // 2. Create StructType (methods empty initially)
    auto classType = std::make_shared<StructType>(decl->getName(), std::move(fields));
    classType->setInternalIsClass(true);
    
    // 3. Register type
    if (!symbolTable.insertTypeGlobal(decl->getName(), classType, decl->isPublic(), decl)) {
        throw std::runtime_error("Redefinition of type '" + decl->getName() + "'");
    }

    if (!decl->getGenericParams().empty()) {
        return;
    }

    // 4. Gather methods and fix types
    std::vector<StructType::Method> methods;
    
    for (const auto& member : decl->getMembers()) {
        if (member->getKind() == ASTNodeKind::MethodDecl) {
             auto method = static_cast<MethodDecl*>(member.get());
             std::vector<std::shared_ptr<Type>> paramTypes;
             for(const auto& p : method->getParams()) {
                 auto pType = resolveType(p->getType());
                 // Check if it's "Self" placeholder
                 if (pType->isStruct() && std::static_pointer_cast<StructType>(pType)->getName() == "Self") {
                     pType = classType;
                     p->setType(pType); // Update AST
                 } else if (pType->isPointer()) {
                     auto ptrType = std::static_pointer_cast<PointerType>(pType);
                     auto base = ptrType->getBaseType();
                     if (base->isStruct() && std::static_pointer_cast<StructType>(base)->getName() == "Self") {
                         pType = std::make_shared<PointerType>(classType);
                         p->setType(pType); // Update AST
                     }
                 }
                 paramTypes.push_back(pType);
             }
             method->setReturnType(resolveType(method->getReturnType()));
             auto funcType = std::make_shared<FunctionType>(std::move(paramTypes), method->getReturnType());
             methods.push_back({method->getName(), funcType, method->isPublic()});
             std::cout << "Added method " << method->getName() << " to class " << decl->getName() << std::endl;
        }
    }
    classType->setMethods(std::move(methods));

    // 5. Analyze bodies
    auto oldClass = currentClass;
    currentClass = classType;

    for (const auto& member : decl->getMembers()) {
        auto kind = member->getKind();
        if (kind == ASTNodeKind::MethodDecl) {
            analyzeMethodDecl(static_cast<MethodDecl*>(member.get()));
        } else if (kind == ASTNodeKind::ConstructorDecl) {
            analyzeConstructorDecl(static_cast<ConstructorDecl*>(member.get()));
        } else if (kind == ASTNodeKind::VarDecl) {
             auto varDecl = static_cast<VarDecl*>(member.get());
             if (varDecl->getInitializer()) {
                 checkExpr(const_cast<Expr*>(varDecl->getInitializer()));
             }
        }
    }
    currentClass = oldClass;
}

void Sema::analyzeMethodDecl(MethodDecl* decl) {
    if (!decl->getGenericParams().empty()) {
        return;
    }

    auto oldFunc = currentFunction;
    auto oldMethod = currentMethod;
    currentFunction = nullptr;
    currentMethod = decl;

    symbolTable.pushScope();
    for (const auto& param : decl->getParams()) {
        if (!symbolTable.insert(param->getName(), param->getType(), false)) {
            throw std::runtime_error("Redefinition of parameter '" + param->getName() + "'");
        }
    }
    
    if (decl->getBody()) {
        analyzeBlock(const_cast<Block*>(decl->getBody()));
    }
    
    symbolTable.popScope();
    currentMethod = oldMethod;
    currentFunction = oldFunc;
}

void Sema::analyzeConstructorDecl(ConstructorDecl* decl) {
    if (decl->getName() != currentClass->getName()) {
        throw std::runtime_error("Constructor name must match class name");
    }
    
    auto oldFunc = currentFunction;
    auto oldMethod = currentMethod;
    currentFunction = nullptr;
    currentMethod = nullptr; // Constructors are special, but definitely not the outer function

    symbolTable.pushScope();
    // Add params
    for (const auto& param : decl->getParams()) {
        param->setType(resolveType(param->getType()));
        if (!symbolTable.insert(param->getName(), param->getType(), false)) {
             throw std::runtime_error("Redefinition of parameter '" + param->getName() + "'");
        }
    }
    
    // Add 'self' to constructor scope
    auto selfType = std::make_shared<PointerType>(currentClass);
    symbolTable.insert("self", selfType, false);
    
    if (decl->getBody()) {
        analyzeBlock(const_cast<Block*>(decl->getBody()));
    }
    
    symbolTable.popScope();
    currentMethod = oldMethod;
    currentFunction = oldFunc;
}

void Sema::analyzeImportDecl(ImportDecl* decl) {
    if (decl->isStd) {
        return;
    }

    std::string filePath = decl->path;
    if (loadedModules.count(filePath)) return;
    loadedModules.insert(filePath);

    std::string moduleName;
    if (!decl->alias.empty()) {
        moduleName = decl->alias;
    } else {
        size_t lastSlash = filePath.find_last_of("/\\");
        std::string filename = (lastSlash == std::string::npos) ? filePath : filePath.substr(lastSlash + 1);
        size_t dot = filename.find_last_of(".");
        moduleName = (dot == std::string::npos) ? filename : filename.substr(0, dot);
    }


    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open imported file: " + filePath);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();

    Parser parser(source);
    auto nodes = parser.parseProgram();

    Sema subSema;
    subSema.loadedModules = this->loadedModules;
    for (auto& node : nodes) {
        subSema.analyze(node.get());
    }
    this->loadedModules = subSema.loadedModules;

    // Collect all nodes from sub-analysis
    for (auto& node : subSema.analyzedNodes) {
        this->analyzedNodes.push_back(std::move(node));
    }
    // Collect top-level nodes from this module and mangle their names
    for (auto& node : nodes) {
        switch (node->getKind()) {
            case ASTNodeKind::FunctionDecl:
                static_cast<FunctionDecl*>(node.get())->setName(moduleName + "_" + static_cast<FunctionDecl*>(node.get())->getName());
                break;
            case ASTNodeKind::StructDecl:
                static_cast<StructDecl*>(node.get())->setName(moduleName + "_" + static_cast<StructDecl*>(node.get())->getName());
                break;
            case ASTNodeKind::EnumDecl:
                static_cast<EnumDecl*>(node.get())->setName(moduleName + "_" + static_cast<EnumDecl*>(node.get())->getName());
                break;
            case ASTNodeKind::ClassDecl:
                static_cast<ClassDecl*>(node.get())->setName(moduleName + "_" + static_cast<ClassDecl*>(node.get())->getName());
                break;
            default:
                break;
        }
        this->analyzedNodes.push_back(std::move(node));
    }

    // Store the symbol table
    modules[moduleName] = std::make_shared<SymbolTable>(std::move(subSema.symbolTable));
}

void Sema::analyzePackageDecl(PackageDecl* decl) {
}

void Sema::analyzeUseDecl(UseDecl* decl) {
    size_t pos = decl->path.rfind("::");
    if (pos == std::string::npos) {
        throw std::runtime_error("Invalid use path (expected module::member): " + decl->path);
    }
    
    std::string moduleName = decl->path.substr(0, pos);
    std::string memberName = decl->path.substr(pos + 2);
    
    if (!modules.count(moduleName)) {
        throw std::runtime_error("Unknown module: " + moduleName);
    }
    
    auto moduleTable = modules.at(moduleName);
    std::string alias = decl->alias.empty() ? memberName : decl->alias;
    
    auto publicSymbols = moduleTable->getPublicSymbols();
    if (publicSymbols.count(memberName)) {
        auto sym = publicSymbols.at(memberName);
        if (!symbolTable.insert(alias, sym->type, sym->isMutable, false)) {
            throw std::runtime_error("Collision bringing '" + memberName + "' into scope as '" + alias + "'");
        }
        return;
    }
    
    auto publicTypes = moduleTable->getPublicTypes();
    if (publicTypes.count(memberName)) {
        auto type = publicTypes.at(memberName);
        if (!symbolTable.insertType(alias, type, false)) {
            throw std::runtime_error("Collision bringing type '" + memberName + "' into scope as '" + alias + "'");
        }
        return;
    }
    
    throw std::runtime_error("Module '" + moduleName + "' has no public member named '" + memberName + "'");
}

std::string Sema::mangleGenericName(const std::string& baseName, const std::vector<std::shared_ptr<Type>>& typeArgs) {
    std::string mangled = baseName + "_";
    for (size_t i = 0; i < typeArgs.size(); ++i) {
        if (!typeArgs[i]) throw std::runtime_error("mangleGenericName: null typeArg at index " + std::to_string(i));
        std::string tyStr = typeArgs[i]->toString();
        // Consistently handle pointers in mangled names
        size_t pos;
        while ((pos = tyStr.find('*')) != std::string::npos) {
            tyStr.replace(pos, 1, "Ptr");
        }
        // Replace other special characters
        for (char& c : tyStr) {
            if (c == '[' || c == ']' || c == ' ' || c == ',' || c == '(' || c == ')' || c == ':') {
                c = '_';
            }
        }
        mangled += tyStr;
        if (i < typeArgs.size() - 1) mangled += "_";
    }
    return mangled;
}

FunctionDecl* Sema::monomorphizeFunction(FunctionDecl* decl, const std::vector<std::shared_ptr<Type>>& typeArgs) {
    std::string mangledName = mangleGenericName(decl->getName(), typeArgs);
    
    if (m_monomorphizedFunctions.count(mangledName)) {
        return m_monomorphizedFunctions[mangledName];
    }

    if (decl->getGenericParams().size() != typeArgs.size()) {
        throw std::runtime_error("Generic argument count mismatch for function " + decl->getName());
    }

    std::map<std::string, std::shared_ptr<Type>> mapping;
    for (size_t i = 0; i < decl->getGenericParams().size(); ++i) {
        mapping[decl->getGenericParams()[i].name] = typeArgs[i];
        if (decl->getGenericParams()[i].constraint) {
             for (const auto& item : decl->getGenericParams()[i].constraint->getItems()) {
                 checkConstraint(item.traitName, typeArgs[i]);
             }
        }
    }

    ASTSubstituter substituter(mapping);
    auto specialized = substituter.substituteDecl(decl);
    auto specFunc = static_cast<FunctionDecl*>(specialized.get());
    specFunc->setName(mangledName);
    specFunc->clearGenericParams();
    
    // Store in cache and transfer ownership
    m_monomorphizedFunctions[mangledName] = specFunc;
    analyzedNodes.push_back(std::move(specialized));

    // Analyze the specialized function
    analyzeFunctionDecl(specFunc);

    return specFunc;
}

FunctionDecl* Sema::monomorphizeMethod(ClassDecl* clDecl, MethodDecl* method, const std::vector<std::shared_ptr<Type>>& typeArgs) {
    std::string baseName = clDecl->getName() + "_" + method->getName();
    std::string mangledName = mangleGenericName(baseName, typeArgs);
    
    if (m_monomorphizedFunctions.count(mangledName)) {
        return m_monomorphizedFunctions[mangledName];
    }

    if (method->getGenericParams().size() != typeArgs.size()) {
        throw std::runtime_error("Generic argument count mismatch for method " + method->getName());
    }

    std::map<std::string, std::shared_ptr<Type>> mapping;
    for (size_t i = 0; i < method->getGenericParams().size(); ++i) {
        mapping[method->getGenericParams()[i].name] = typeArgs[i];
        if (method->getGenericParams()[i].constraint) {
             for (const auto& item : method->getGenericParams()[i].constraint->getItems()) {
                 checkConstraint(item.traitName, typeArgs[i]);
             }
        }
    }

    ASTSubstituter substituter(mapping);
    auto specialized = substituter.substituteStmt(method);
    auto specMethod = static_cast<MethodDecl*>(specialized.get());
    specMethod->clearGenericParams();
    
    // Create a FunctionDecl for CodeGen convenience
    // Monomorphized methods are essentially functions.
    // Mangled name is already unique.
    std::vector<std::unique_ptr<Param>> params;
    for (const auto& p : specMethod->getParams()) {
        params.push_back(std::unique_ptr<Param>(static_cast<Param*>(p->clone().release())));
    }
    
    auto specFunc = std::make_unique<FunctionDecl>(
        mangledName, 
        specMethod->getReturnType(), 
        std::move(params), 
        std::unique_ptr<Block>(static_cast<Block*>(specMethod->getBody()->cloneStmt().release())),
        false, 
        specMethod->isPublic()
    );

    auto resFunc = specFunc.get();
    m_monomorphizedFunctions[mangledName] = resFunc;
    analyzedNodes.push_back(std::move(specFunc));

    // Analyze the specialized function
    // But we need to set currentClass if we want to support member access inside
    auto oldClass = currentClass;
    std::string className = clDecl->getName(); // This should be mangled if caller passed mangled decl
    auto type = symbolTable.lookupType(className);
    if (type && type->isStruct()) currentClass = std::static_pointer_cast<StructType>(type);
    
    analyzeFunctionDecl(resFunc);
    
    currentClass = oldClass;

    return resFunc;
}

std::shared_ptr<StructType> Sema::monomorphizeStruct(StructDecl* decl, const std::vector<std::shared_ptr<Type>>& typeArgs) {
    std::string mangledName = mangleGenericName(decl->getName(), typeArgs);
    
    auto existing = symbolTable.lookupType(mangledName);
    if (existing && existing->isStruct()) {
        return std::static_pointer_cast<StructType>(existing);
    }

    if (decl->getGenericParams().size() != typeArgs.size()) {
        throw std::runtime_error("Generic argument count mismatch for struct " + decl->getName());
    }

    std::map<std::string, std::shared_ptr<Type>> mapping;
    for (size_t i = 0; i < decl->getGenericParams().size(); ++i) {
        mapping[decl->getGenericParams()[i].name] = typeArgs[i];
        if (decl->getGenericParams()[i].constraint) {
             for (const auto& item : decl->getGenericParams()[i].constraint->getItems()) {
                 checkConstraint(item.traitName, typeArgs[i]);
             }
        }
    }

    ASTSubstituter substituter(mapping);
    auto specialized = substituter.substituteDecl(decl);
    auto specStructDecl = static_cast<StructDecl*>(specialized.get());
    specStructDecl->setName(mangledName);
    specStructDecl->clearGenericParams();

    // Store in cache and transfer ownership
    m_monomorphizedStructs[mangledName] = specStructDecl;
    analyzedNodes.push_back(std::move(specialized));

    // Register and analyze
    analyzeStructDecl(specStructDecl);
    
    auto res = symbolTable.lookupType(mangledName);
    return std::static_pointer_cast<StructType>(res);
}

std::shared_ptr<StructType> Sema::monomorphizeClass(ClassDecl* decl, const std::vector<std::shared_ptr<Type>>& typeArgs) {
    std::string mangledName = mangleGenericName(decl->getName(), typeArgs);
    
    auto existing = symbolTable.lookupType(mangledName);
    if (existing && existing->isClass()) {
        return std::static_pointer_cast<StructType>(existing);
    }

    if (decl->getGenericParams().size() != typeArgs.size()) {
        throw std::runtime_error("Generic argument count mismatch for class " + decl->getName());
    }

    std::map<std::string, std::shared_ptr<Type>> mapping;
    for (size_t i = 0; i < decl->getGenericParams().size(); ++i) {
        mapping[decl->getGenericParams()[i].name] = typeArgs[i];
        if (decl->getGenericParams()[i].constraint) {
             for (const auto& item : decl->getGenericParams()[i].constraint->getItems()) {
                 checkConstraint(item.traitName, typeArgs[i]);
             }
        }
    }

    ASTSubstituter substituter(mapping);
    auto specialized = substituter.substituteDecl(decl);
    auto specClassDecl = static_cast<ClassDecl*>(specialized.get());
    specClassDecl->setName(mangledName);
    specClassDecl->clearGenericParams();

    // Store in cache and transfer ownership
    m_monomorphizedClasses[mangledName] = specClassDecl;
    analyzedNodes.push_back(std::move(specialized));

    // Register and analyze
    analyzeClassDecl(specClassDecl);
    
    auto res = symbolTable.lookupType(mangledName);
    return std::static_pointer_cast<StructType>(res);
}

std::shared_ptr<EnumType> Sema::monomorphizeEnum(EnumDecl* decl, const std::vector<std::shared_ptr<Type>>& typeArgs) {
    if (!decl) throw std::runtime_error("monomorphizeEnum: decl is null");
    std::string mangledName = mangleGenericName(decl->getName(), typeArgs);
    
    auto existing = symbolTable.lookupType(mangledName);
    if (existing && existing->isEnum()) {
        return std::static_pointer_cast<EnumType>(existing);
    }

    if (decl->getGenericParams().size() != typeArgs.size()) {
        throw std::runtime_error("Generic argument count mismatch for enum " + decl->getName());
    }

    std::map<std::string, std::shared_ptr<Type>> mapping;
    for (size_t i = 0; i < decl->getGenericParams().size(); ++i) {
        mapping[decl->getGenericParams()[i].name] = typeArgs[i];
        if (decl->getGenericParams()[i].constraint) {
             for (const auto& item : decl->getGenericParams()[i].constraint->getItems()) {
                 checkConstraint(item.traitName, typeArgs[i]);
             }
        }
    }

    ASTSubstituter substituter(mapping);
    auto specialized = substituter.substituteDecl(decl);
    auto specEnumDecl = static_cast<EnumDecl*>(specialized.get());
    specEnumDecl->setName(mangledName);
    specEnumDecl->clearGenericParams();

    // Store in cache and transfer ownership
    m_monomorphizedEnums[mangledName] = specEnumDecl;
    analyzedNodes.push_back(std::move(specialized));

    // Register and analyze
    analyzeEnumDecl(specEnumDecl);
    
    auto res = symbolTable.lookupType(mangledName);
    return std::static_pointer_cast<EnumType>(res);
}

std::shared_ptr<Type> Sema::checkSpecializationExpr(SpecializationExpr* expr) {
    if (!expr->getBase()) throw std::runtime_error("Specialization base is null");
    
    // Resolve type arguments first
    std::vector<std::shared_ptr<Type>> resolvedArgs;
    for (auto t : expr->getTypeArgs()) resolvedArgs.push_back(resolveType(t));
    expr->setTypeArgs(resolvedArgs);

    if (expr->getBase()->getKind() == ASTNodeKind::IdentifierExpr) {
        auto id = static_cast<const IdentifierExpr*>(expr->getBase());
        std::string baseName = id->toString();
        // Check if it's a function
        auto sym = symbolTable.lookup(baseName);
        if (sym && sym->decl) {
            if (sym->decl->getKind() == ASTNodeKind::FunctionDecl) {
                auto funcDecl = static_cast<FunctionDecl*>(sym->decl);
                auto monomorphized = monomorphizeFunction(funcDecl, resolvedArgs);
                expr->setMangledName(monomorphized->getName());
                return monomorphized->getType(); 
            }
        }
        // Check if it's a type (struct/class/enum)
        if (auto decl = symbolTable.lookupTypeDecl(baseName)) {
            if (decl->getKind() == ASTNodeKind::StructDecl) {
                auto res = monomorphizeStruct(static_cast<StructDecl*>(decl), resolvedArgs);
                expr->setMangledName(res->getName());
                return res;
            } else if (decl->getKind() == ASTNodeKind::ClassDecl) {
                auto res = monomorphizeClass(static_cast<ClassDecl*>(decl), resolvedArgs);
                expr->setMangledName(res->getName());
                return res;
            } else if (decl->getKind() == ASTNodeKind::EnumDecl) {
                auto res = monomorphizeEnum(static_cast<EnumDecl*>(decl), resolvedArgs);
                expr->setMangledName(res->getName());
                return res;
            }
        }
    } else if (expr->getBase()->getKind() == ASTNodeKind::MemberAccessExpr) {
        auto memAccess = static_cast<const MemberAccessExpr*>(expr->getBase());
        // Handle Enum[T]::Variant or Class[T]::Method
        // Actually, the parser might produce Specialization(MemberAccess(Enum, Variant), [T])
        // if the user writes Enum::Variant[T] (which is valid for generic methods)
        // But if they write Enum[T]::Variant, it's MemberAccess(Specialization(Enum, [T]), Variant)
        // This case handles the former.
        auto baseType = checkExpr(const_cast<Expr*>(memAccess->getBase()));
        if (baseType->isPointer()) baseType = std::static_pointer_cast<PointerType>(baseType)->getBaseType();
        
        if (baseType->isStruct()) {
            auto stTy = std::static_pointer_cast<StructType>(baseType);
            auto decl = symbolTable.lookupTypeDecl(stTy->getName());
            if (decl && decl->getKind() == ASTNodeKind::ClassDecl) {
                auto clDecl = static_cast<ClassDecl*>(decl);
                for (const auto& member : clDecl->getMembers()) {
                    if (member->getKind() == ASTNodeKind::MethodDecl) {
                        auto method = static_cast<MethodDecl*>(member.get());
                        if (method->getName() == memAccess->getMemberName()) {
                            auto monomorphized = monomorphizeMethod(clDecl, method, resolvedArgs);
                            expr->setMangledName(monomorphized->getName());
                            return monomorphized->getType();
                        }
                    }
                }
            }
        }
    }
    return Type::getVoid();
}

std::shared_ptr<Type> Sema::checkQuestionExpr(QuestionExpr* expr) {
    auto opType = checkExpr(const_cast<Expr*>(expr->getOperand()));
    if (!opType) throw std::runtime_error("? operator operand has no type");
    

    if (!opType->isEnum()) {
        throw std::runtime_error("? operator can only be used on Result enum, got " + opType->toString());
    }

    auto enumTy = std::dynamic_pointer_cast<EnumType>(opType);
    if (!enumTy) throw std::runtime_error("Internal error: EnumType cast failed for " + opType->toString());

    if (enumTy->getName().find("Result_") != 0) {
        throw std::runtime_error("? operator can only be used on Result enum, got " + enumTy->getName());
    }

    // currentFunction or currentMethod must return Result
    std::shared_ptr<Type> retTy = nullptr;
    if (currentFunction) retTy = currentFunction->getReturnType();
    else if (currentMethod) retTy = currentMethod->getReturnType();

    if (!retTy) throw std::runtime_error("? operator used outside of function/method context");
    

    if (!retTy->isEnum() || std::static_pointer_cast<EnumType>(retTy)->getName().find("Result_") != 0) {
        throw std::runtime_error("? operator can only be used in functions returning Result");
    }

    auto retEnumTy = std::static_pointer_cast<EnumType>(retTy);
    
    if (enumTy->getVariants().size() < 2) throw std::runtime_error("Result enum must have at least 2 variants");
    if (retEnumTy->getVariants().size() < 2) throw std::runtime_error("Return Result enum must have at least 2 variants");

    const auto& opErrVariant = enumTy->getVariants()[1];
    const auto& retErrVariant = retEnumTy->getVariants()[1];

    if (opErrVariant.tupleTypes.empty() || retErrVariant.tupleTypes.empty()) {
        throw std::runtime_error("Result Err variant must have 1 type argument");
    }

    if (!opErrVariant.tupleTypes[0]->equals(*retErrVariant.tupleTypes[0])) {
        throw std::runtime_error("? operator error type mismatch: " + 
                                 opErrVariant.tupleTypes[0]->toString() + " vs " + 
                                 retErrVariant.tupleTypes[0]->toString());
    }

    if (enumTy->getVariants()[0].tupleTypes.empty()) throw std::runtime_error("Result Ok variant must have 1 type argument");

    return enumTy->getVariants()[0].tupleTypes[0];
}

void Sema::checkConstraint(const std::string& requestName, std::shared_ptr<Type> type) {
    auto reqDecl = symbolTable.lookupTypeDecl(requestName);
    if (!reqDecl || reqDecl->getKind() != ASTNodeKind::RequestDecl) {
        throw std::runtime_error("Unknown request constraint '" + requestName + "'");
    }
    auto request = static_cast<RequestDecl*>(reqDecl);
    
    std::map<std::string, std::shared_ptr<Type>> reqMapping;
    reqMapping["Self"] = type;
    if (!request->getGenericParams().empty()) {
        reqMapping[request->getGenericParams()[0].name] = type;
    }

    ASTSubstituter substituter(reqMapping);

    for (const auto& member : request->getMembers()) {
        if (member.decl->getKind() == ASTNodeKind::MethodDecl) {
             auto reqMethod = static_cast<MethodDecl*>(member.decl.get());
             
             std::vector<std::shared_ptr<Type>> paramTypes;
             for(const auto& p : reqMethod->getParams()) paramTypes.push_back(p->getType());
             auto reqFuncType = std::make_shared<FunctionType>(paramTypes, reqMethod->getReturnType());
             
             auto expectedType = reqFuncType->substitute(reqMapping);
             
             std::shared_ptr<Type> actualMethodType = nullptr;
             
             if (type->isStruct()) {
                 auto st = std::static_pointer_cast<StructType>(type);
                 actualMethodType = st->findMethod(reqMethod->getName());
                 if (!actualMethodType) {
                     std::cout << "Looking for method " << reqMethod->getName() << " in struct " << st->getName() << std::endl;
                     std::cout << "Available methods:" << std::endl;
                     for (const auto& m : st->getMethods()) {
                         std::cout << " - " << m.name << std::endl;
                     }
                 }
             }
             
             if (!actualMethodType) {
                  throw std::runtime_error("Type '" + type->toString() + "' does not satisfy request '" + requestName + "': missing method '" + reqMethod->getName() + "'");
             }
             
             if (!actualMethodType->equals(*expectedType)) {
                 throw std::runtime_error("Type '" + type->toString() + "' does not satisfy request '" + requestName + "': method '" + reqMethod->getName() + "' has incorrect signature. Expected " + expectedType->toString() + ", got " + actualMethodType->toString());
             }
        }
    }
}

} // namespace chtholly
