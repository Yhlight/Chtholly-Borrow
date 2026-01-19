#include "AST/Declarations.h"
#include "AST/Expressions.h"
#include "AST/Types.h"
#include <cassert>
#include <iostream>

using namespace chtholly;

void testVarDecl() {
    auto type = Type::getI32();
    auto init = std::make_unique<LiteralExpr>(10);
    auto varDecl = std::make_unique<VarDecl>("x", type, std::move(init), false);
    
    assert(varDecl->getName() == "x");
    assert(varDecl->getType()->toString() == "i32");
    assert(varDecl->isMutable() == false);
    assert(varDecl->toString() == "let x: i32 = 10;");

    auto varDeclMut = std::make_unique<VarDecl>("y", Type::getF64(), nullptr, true);
    assert(varDeclMut->getName() == "y");
    assert(varDeclMut->isMutable() == true);
    assert(varDeclMut->toString() == "let mut y: f64;");

    std::cout << "testVarDecl passed!" << std::endl;
}

void testFunctionDecl() {
    auto i32Type = Type::getI32();
    auto param1 = std::make_unique<Param>("a", i32Type);
    auto param2 = std::make_unique<Param>("b", i32Type);
    
    std::vector<std::unique_ptr<Param>> params;
    params.push_back(std::move(param1));
    params.push_back(std::move(param2));
    
    auto body = std::make_unique<Block>(std::vector<std::unique_ptr<Stmt>>());
    auto funcDecl = std::make_unique<FunctionDecl>("add", i32Type, std::move(params), std::move(body));
    
    assert(funcDecl->getName() == "add");
    assert(funcDecl->getReturnType() == i32Type);
    assert(funcDecl->getParams().size() == 2);
    assert(funcDecl->toString() == "fn add(a: i32, b: i32): i32 {\n}");

    std::cout << "testFunctionDecl passed!" << std::endl;
}

int main() {
    testVarDecl();
    testFunctionDecl();
    return 0;
}
