#include "MIR/MIRBuilder.h"
#include "AST/Statements.h"
#include <cassert>
#include <iostream>

void testControlFlowMIR() {
    using namespace chtholly;
    
    // 1. Test For Loop MIR
    {
        MIRModule module;
        MIRBuilder builder(module);
        
        // for (let i = 0; i < 10; i = i + 1) { }
        auto init = std::make_unique<VarDecl>("i", Type::getI32(), std::make_unique<LiteralExpr>(int64_t(0)), false);
        auto cond = std::make_unique<BinaryExpr>(std::make_unique<IdentifierExpr>("i"), TokenType::Less, std::make_unique<LiteralExpr>(int64_t(10)));
        auto step = std::make_unique<BinaryExpr>(std::make_unique<IdentifierExpr>("i"), TokenType::Equal, 
                        std::make_unique<BinaryExpr>(std::make_unique<IdentifierExpr>("i"), TokenType::Plus, std::make_unique<LiteralExpr>(int64_t(1))));
        auto body = std::make_unique<Block>(std::vector<std::unique_ptr<Stmt>>());
        
        auto forStmt = std::make_unique<ForStmt>(std::move(init), std::move(cond), std::move(step), std::move(body));
        
        std::vector<std::unique_ptr<Stmt>> stmts;
        stmts.push_back(std::move(forStmt));
        auto mainBody = std::make_unique<Block>(std::move(stmts));
        auto funcDecl = std::make_unique<FunctionDecl>("test_for", Type::getVoid(), std::vector<std::unique_ptr<Param>>(), std::move(mainBody));
        
        builder.lower(funcDecl.get());
        
        std::string mir = module.toString();
        assert(mir.find("for.cond") != std::string::npos);
        assert(mir.find("for.step") != std::string::npos);
        assert(mir.find("br label %for.cond") != std::string::npos);
    }

    std::cout << "testControlFlowMIR passed!" << std::endl;
}

int main() {
    testControlFlowMIR();
    return 0;
}