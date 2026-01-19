#include "MIR/MIRBuilder.h"
#include "AST/Declarations.h"
#include "AST/Statements.h"
#include <iostream>
#include <vector>

void testClassCodeGen() {
    using namespace chtholly;
    
    // Class Point { let x: i32; let y: i32; Point(a: i32, b: i32) { self.x = a; self.y = b; } fn sum() { return self.x + self.y; } }
    
    std::vector<std::unique_ptr<ASTNode>> members;
    members.push_back(std::make_unique<VarDecl>("x", Type::getI32(), nullptr, false, true));
    members.push_back(std::make_unique<VarDecl>("y", Type::getI32(), nullptr, false, true));
    
    // Constructor
    std::vector<std::unique_ptr<Param>> ctorParams;
    ctorParams.push_back(std::make_unique<Param>("a", Type::getI32()));
    ctorParams.push_back(std::make_unique<Param>("b", Type::getI32()));
    
    std::vector<std::unique_ptr<Stmt>> ctorBodyStmts;
    // self.x = a;
    auto selfExpr1 = std::make_unique<IdentifierExpr>("self");
    auto memberX = std::make_unique<MemberAccessExpr>(std::move(selfExpr1), "x");
    auto assignX = std::make_unique<BinaryExpr>(std::move(memberX), TokenType::Equal, std::make_unique<IdentifierExpr>("a"));
    ctorBodyStmts.push_back(std::make_unique<ExprStmt>(std::move(assignX))); 
    
    auto selfExpr2 = std::make_unique<IdentifierExpr>("self");
    auto memberY = std::make_unique<MemberAccessExpr>(std::move(selfExpr2), "y");
    auto assignY = std::make_unique<BinaryExpr>(std::move(memberY), TokenType::Equal, std::make_unique<IdentifierExpr>("b"));
    ctorBodyStmts.push_back(std::make_unique<ExprStmt>(std::move(assignY)));
    
    members.push_back(std::make_unique<ConstructorDecl>("Point", std::move(ctorParams), std::make_unique<Block>(std::move(ctorBodyStmts)), true));
    
    // Method sum
    std::vector<std::unique_ptr<Param>> methodParams;
    auto selfType = std::make_shared<StructType>("Point", std::vector<StructType::Field>{}); // Placeholder name
    auto selfPtrType = std::make_shared<PointerType>(selfType);
    // Note: MIRBuilder relies on ptrTypeMap populated from type info. 
    // Here we construct AST manually.
    // MIRBuilder::lowerConstructorDecl will inject "self" into varMap and ptrTypeMap.
    // MIRBuilder::lowerMethodDecl expects manual "self" param handling or implicit.
    // In current implementation, lowerMethodDecl iterates params and adds them.
    // So we MUST add "self" param.
    // And its type name must match.
    methodParams.push_back(std::make_unique<Param>("self", selfPtrType));
    
    std::vector<std::unique_ptr<Stmt>> methodBodyStmts;
    auto selfExpr3 = std::make_unique<IdentifierExpr>("self");
    auto memX = std::make_unique<MemberAccessExpr>(std::move(selfExpr3), "x");
    auto selfExpr4 = std::make_unique<IdentifierExpr>("self");
    auto memY = std::make_unique<MemberAccessExpr>(std::move(selfExpr4), "y");
    auto sumExpr = std::make_unique<BinaryExpr>(std::move(memX), TokenType::Plus, std::move(memY));
    methodBodyStmts.push_back(std::make_unique<ReturnStmt>(std::move(sumExpr)));
    
    members.push_back(std::make_unique<MethodDecl>("sum", Type::getI32(), std::move(methodParams), std::make_unique<Block>(std::move(methodBodyStmts)), true));
    
    auto classDecl = std::make_unique<ClassDecl>("Point", std::move(members));
    
    // Main function
    MIRModule module;
    MIRBuilder builder(module);
    
    try {
        builder.lower(classDecl.get());
        
        std::vector<std::unique_ptr<Stmt>> mainStmts;
        
        // let p = Point(10, 20)
            std::vector<std::unique_ptr<Expr>> args;
            args.push_back(std::make_unique<LiteralExpr>(10));
            args.push_back(std::make_unique<LiteralExpr>(20));
            auto ctorCall = std::make_unique<CallExpr>(std::make_unique<IdentifierExpr>("Point"), std::move(args));
            
            // Explicitly set type for 'p' so MIRBuilder knows it's a struct
            auto pointType = std::make_shared<StructType>("Point", std::vector<StructType::Field>{});
            mainStmts.push_back(std::make_unique<VarDecl>("p", pointType, std::move(ctorCall), false));
            
            // return p.sum()
            auto pExpr = std::make_unique<IdentifierExpr>("p");
        auto methodAccess = std::make_unique<MemberAccessExpr>(std::move(pExpr), "sum");
        auto methodCall = std::make_unique<CallExpr>(std::move(methodAccess), std::vector<std::unique_ptr<Expr>>{});
        mainStmts.push_back(std::make_unique<ReturnStmt>(std::move(methodCall)));
        
        auto mainFunc = std::make_unique<FunctionDecl>("main", Type::getI32(), std::vector<std::unique_ptr<Param>>{}, std::make_unique<Block>(std::move(mainStmts)));
        
        builder.lower(mainFunc.get());
        
        std::cout << module.toString() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "CodeGen error: " << e.what() << std::endl;
        throw;
    }
}

int main() {
    testClassCodeGen();
    return 0;
}