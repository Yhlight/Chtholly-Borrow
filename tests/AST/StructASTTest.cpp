#include "AST/Declarations.h"
#include "AST/Expressions.h"
#include <cassert>
#include <iostream>
#include <vector>

void testStructAST() {
    using namespace chtholly;

    // Test StructDecl
    // struct Point { let x: i32; let y: i32; }
    std::vector<std::unique_ptr<VarDecl>> members;
    members.push_back(std::make_unique<VarDecl>("x", Type::getI32(), nullptr, false));
    members.push_back(std::make_unique<VarDecl>("y", Type::getI32(), nullptr, false));
    
    // Assuming StructDecl exists
    auto structDecl = std::make_unique<StructDecl>("Point", std::move(members));
    assert(structDecl->getName() == "Point");
    assert(structDecl->getMembers().size() == 2);
    
    // Test MemberAccessExpr
    // p.x
    auto obj = std::make_unique<IdentifierExpr>("p");
    auto memberAccess = std::make_unique<MemberAccessExpr>(std::move(obj), "x");
    assert(memberAccess->getMember() == "x");
    
    std::cout << "testStructAST passed!" << std::endl;
}

int main() {
    testStructAST();
    return 0;
}
