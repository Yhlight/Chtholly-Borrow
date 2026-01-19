#include "AST/Declarations.h"
#include "AST/Types.h"
#include <cassert>
#include <iostream>

using namespace chtholly;

void testVarDeclAsStmt() {
    auto type = Type::getI32();
    auto varDecl = std::make_unique<VarDecl>("x", type, nullptr, false);
    
    // VarDecl should be castable to Stmt
    Stmt* stmt = static_cast<Stmt*>(varDecl.get());
    assert(stmt != nullptr);
    
    assert(varDecl->toString() == "let x: i32;");
    std::cout << "testVarDeclAsStmt passed!" << std::endl;
}

void testBlock() {
    auto type = Type::getI32();
    auto varDecl = std::make_unique<VarDecl>("x", type, nullptr, false);
    
    std::vector<std::unique_ptr<Stmt>> statements;
    statements.push_back(std::move(varDecl));
    
    auto block = std::make_unique<Block>(std::move(statements));
    assert(block->toString() == "{\n  let x: i32;\n}");
    std::cout << "testBlock passed!" << std::endl;
}

void testIfStmt() {
    auto cond = std::make_unique<LiteralExpr>(true);
    
    std::vector<std::unique_ptr<Stmt>> thenStmts;
    thenStmts.push_back(std::make_unique<VarDecl>("x", Type::getI32(), nullptr, false));
    auto thenBlock = std::make_unique<Block>(std::move(thenStmts));
    
    std::vector<std::unique_ptr<Stmt>> elseStmts;
    elseStmts.push_back(std::make_unique<VarDecl>("y", Type::getI32(), nullptr, false));
    auto elseBlock = std::make_unique<Block>(std::move(elseStmts));
    
    auto ifStmt = std::make_unique<IfStmt>(std::move(cond), std::move(thenBlock), std::move(elseBlock));
    
    assert(ifStmt->toString() == "if true {\n  let x: i32;\n} else {\n  let y: i32;\n}");
    std::cout << "testIfStmt passed!" << std::endl;
}

void testWhileStmt() {
    auto cond = std::make_unique<LiteralExpr>(true);
    
    std::vector<std::unique_ptr<Stmt>> bodyStmts;
    bodyStmts.push_back(std::make_unique<VarDecl>("x", Type::getI32(), nullptr, false));
    auto bodyBlock = std::make_unique<Block>(std::move(bodyStmts));
    
    auto whileStmt = std::make_unique<WhileStmt>(std::move(cond), std::move(bodyBlock));
    
    assert(whileStmt->toString() == "while true {\n  let x: i32;\n}");
    std::cout << "testWhileStmt passed!" << std::endl;
}

void testReturnStmt() {
    auto expr = std::make_unique<LiteralExpr>(42);
    auto retStmt = std::make_unique<ReturnStmt>(std::move(expr));
    assert(retStmt->toString() == "return 42;");

    auto voidRet = std::make_unique<ReturnStmt>(nullptr);
    assert(voidRet->toString() == "return;");
    
    std::cout << "testReturnStmt passed!" << std::endl;
}

int main() {
    testVarDeclAsStmt();
    testBlock();
    testIfStmt();
    testWhileStmt();
    testReturnStmt();
    return 0;
}
