#include "MIR/MIRBuilder.h"
#include <cassert>
#include <iostream>

void testMIRStruct() {
    using namespace chtholly;
    
    MIRModule module;
    MIRBuilder builder(module);
    
    // 1. Lower struct Point { let x: i32; let y: i32; }
    std::vector<std::unique_ptr<VarDecl>> members;
    members.push_back(std::make_unique<VarDecl>("x", Type::getI32(), nullptr, false));
    members.push_back(std::make_unique<VarDecl>("y", Type::getI32(), nullptr, false));
    auto structDecl = std::make_unique<StructDecl>("Point", std::move(members));
    
    builder.lower(structDecl.get());
    
    // 2. Lower fn foo(): i32 { let p = Point { x: 1, y: 2 }; return p.x; }
    std::vector<StructLiteralExpr::FieldInit> inits;
    inits.push_back({"x", std::make_unique<LiteralExpr>(int64_t(1))});
    inits.push_back({"y", std::make_unique<LiteralExpr>(int64_t(2))});
    auto structLiteral = std::make_unique<StructLiteralExpr>(std::make_unique<IdentifierExpr>("Point"), std::move(inits));
    
    auto pDecl = std::make_unique<VarDecl>("p", nullptr, std::move(structLiteral), false);
    
    auto pExpr = std::make_unique<IdentifierExpr>("p");
    auto memberAccess = std::make_unique<MemberAccessExpr>(std::move(pExpr), "x");
    auto retStmt = std::make_unique<ReturnStmt>(std::move(memberAccess));
    
    std::vector<std::unique_ptr<Stmt>> bodyStmts;
    bodyStmts.push_back(std::move(pDecl));
    bodyStmts.push_back(std::move(retStmt));
    auto body = std::make_unique<Block>(std::move(bodyStmts));
    
    auto funcDecl = std::make_unique<FunctionDecl>("foo", Type::getI32(), std::vector<std::unique_ptr<Param>>(), std::move(body));
    
    builder.lower(funcDecl.get());
    
    std::string mir = module.toString();
    std::cout << mir << std::endl;
    
    // Verify MIR content
    assert(mir.find("gep") != std::string::npos);
    assert(mir.find("alloca Point") != std::string::npos);
    
    std::cout << "testMIRStruct passed!" << std::endl;
}

int main() {
    testMIRStruct();
    return 0;
}