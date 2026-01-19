#include "Sema/Sema.h"
#include "AST/Statements.h"
#include <cassert>
#include <iostream>

void testControlFlowSema() {
    using namespace chtholly;
    
    // 1. Valid Break in While
    {
        Sema sema;
        std::vector<std::unique_ptr<Stmt>> stmts;
        stmts.push_back(std::make_unique<BreakStmt>());
        auto body = std::make_unique<Block>(std::move(stmts));
        auto whileStmt = std::make_unique<WhileStmt>(std::make_unique<LiteralExpr>(true), std::move(body));
        sema.analyze(whileStmt.get()); // Should pass
    }

    // 2. Invalid Break outside
    {
        Sema sema;
        auto breakStmt = std::make_unique<BreakStmt>();
        try {
            sema.analyze(breakStmt.get());
            assert(false && "Should have thrown for break outside loop");
        } catch (const std::runtime_error& e) { 
            assert(std::string(e.what()).find("Break statement outside") != std::string::npos);
        }
    }

    // 3. Switch type check
    {
        Sema sema;
        std::vector<std::unique_ptr<CaseStmt>> cases;
        
        std::vector<std::unique_ptr<Stmt>> thenStmts;
        std::vector<std::unique_ptr<Stmt>> elseStmts;

        cases.push_back(std::make_unique<CaseStmt>(
            std::make_unique<LiteralPattern>(std::make_unique<LiteralExpr>(int64_t(1))), 
            std::make_unique<Block>(std::move(thenStmts))));
        cases.push_back(std::make_unique<CaseStmt>(nullptr, std::make_unique<Block>(std::move(elseStmts)), true));
        
        auto switchStmt = std::make_unique<SwitchStmt>(std::make_unique<LiteralExpr>(true), std::move(cases));
        try {
            sema.analyze(switchStmt.get());
            assert(false && "Should have thrown for switch type mismatch (bool vs i32)");
        } catch (const std::runtime_error& e) {
            assert(std::string(e.what()).find("Pattern type mismatch") != std::string::npos);
        }
    }

    std::cout << "testControlFlowSema passed!" << std::endl;
}

int main() {
    testControlFlowSema();
    return 0;
}