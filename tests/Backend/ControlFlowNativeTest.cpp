#include "MIR/MIRBuilder.h"
#include "Backend/CodeGenerator.h"
#include <iostream>
#include <filesystem>

void testControlFlowNative() {
    using namespace chtholly;
    
    std::cout << "Initializing MIR Module..." << std::endl;
    MIRModule module;
    MIRBuilder builder(module);
    
    std::cout << "Adding printf to module..." << std::endl;
    auto mirPrintf = std::make_unique<MIRFunction>("printf", Type::getI32());
    mirPrintf->addParameter("fmt", Type::getI8Ptr());
    mirPrintf->setVarArg(true);
    module.appendFunction(std::move(mirPrintf));

    std::cout << "Building main function AST..." << std::endl;
    std::vector<std::unique_ptr<Stmt>> mainStmts;
    
    // 1. For Loop
    {
        std::cout << "  Adding For Loop..." << std::endl;
        auto init = std::make_unique<VarDecl>("i", Type::getI32(), std::make_unique<LiteralExpr>(int64_t(0)), true);
        auto cond = std::make_unique<BinaryExpr>(std::make_unique<IdentifierExpr>("i"), TokenType::Less, std::make_unique<LiteralExpr>(int64_t(5)));
        auto step = std::make_unique<BinaryExpr>(std::make_unique<IdentifierExpr>("i"), TokenType::Equal,
                        std::make_unique<BinaryExpr>(std::make_unique<IdentifierExpr>("i"), TokenType::Plus, std::make_unique<LiteralExpr>(int64_t(1))));
        
        std::vector<std::unique_ptr<Stmt>> bodyStmts;
        
        // if (i == 2) continue;
        {
            auto cond2 = std::make_unique<BinaryExpr>(std::make_unique<IdentifierExpr>("i"), TokenType::EqualEqual, std::make_unique<LiteralExpr>(int64_t(2)));
            std::vector<std::unique_ptr<Stmt>> thenStmts;
            thenStmts.push_back(std::make_unique<ContinueStmt>());
            bodyStmts.push_back(std::make_unique<IfStmt>(std::move(cond2), std::make_unique<Block>(std::move(thenStmts))));
        }
        
        // if (i == 4) break;
        {
            auto cond4 = std::make_unique<BinaryExpr>(std::make_unique<IdentifierExpr>("i"), TokenType::EqualEqual, std::make_unique<LiteralExpr>(int64_t(4)));
            std::vector<std::unique_ptr<Stmt>> thenStmts;
            thenStmts.push_back(std::make_unique<BreakStmt>());
            bodyStmts.push_back(std::make_unique<IfStmt>(std::move(cond4), std::make_unique<Block>(std::move(thenStmts))));
        }
        
        std::vector<std::unique_ptr<Expr>> args;
        args.push_back(std::make_unique<LiteralExpr>("i=%d\n"));
        args.push_back(std::make_unique<IdentifierExpr>("i"));
        bodyStmts.push_back(std::make_unique<ExprStmt>(std::make_unique<CallExpr>(std::make_unique<IdentifierExpr>("printf"), std::move(args))));
        
        auto forStmt = std::make_unique<ForStmt>(std::move(init), std::move(cond), std::move(step), std::make_unique<Block>(std::move(bodyStmts)));
        mainStmts.push_back(std::move(forStmt));
    }

    // 2. Do-While
    {
        std::cout << "  Adding Do-While..." << std::endl;
        mainStmts.push_back(std::make_unique<VarDecl>("j", Type::getI32(), std::make_unique<LiteralExpr>(int64_t(0)), true));
        
        std::vector<std::unique_ptr<Stmt>> bodyStmts;
        std::vector<std::unique_ptr<Expr>> args;
        args.push_back(std::make_unique<LiteralExpr>("j=%d\n"));
        args.push_back(std::make_unique<IdentifierExpr>("j"));
        bodyStmts.push_back(std::make_unique<ExprStmt>(std::make_unique<CallExpr>(std::make_unique<IdentifierExpr>("printf"), std::move(args))));
        
        auto inc = std::make_unique<BinaryExpr>(std::make_unique<IdentifierExpr>("j"), TokenType::Equal,
                        std::make_unique<BinaryExpr>(std::make_unique<IdentifierExpr>("j"), TokenType::Plus, std::make_unique<LiteralExpr>(int64_t(1))));
        bodyStmts.push_back(std::make_unique<ExprStmt>(std::move(inc)));
        
        auto cond = std::make_unique<BinaryExpr>(std::make_unique<IdentifierExpr>("j"), TokenType::Less, std::make_unique<LiteralExpr>(int64_t(2)));
        mainStmts.push_back(std::make_unique<DoWhileStmt>(std::make_unique<Block>(std::move(bodyStmts)), std::move(cond)));
    }

    // 3. Switch
    {
        std::cout << "  Adding Switch..." << std::endl;
        mainStmts.push_back(std::make_unique<VarDecl>("x", Type::getI32(), std::make_unique<LiteralExpr>(int64_t(1)), false));
        
        std::vector<std::unique_ptr<CaseStmt>> cases;
        {
            std::vector<std::unique_ptr<Stmt>> stmts;
            std::vector<std::unique_ptr<Expr>> args;
            args.push_back(std::make_unique<LiteralExpr>("x=0\n"));
            stmts.push_back(std::make_unique<ExprStmt>(std::make_unique<CallExpr>(std::make_unique<IdentifierExpr>("printf"), std::move(args))));
            cases.push_back(std::make_unique<CaseStmt>(std::make_unique<LiteralPattern>(std::make_unique<LiteralExpr>(int64_t(0))), std::make_unique<Block>(std::move(stmts))));
        }
        {
            std::vector<std::unique_ptr<Stmt>> stmts;
            std::vector<std::unique_ptr<Expr>> args;
            args.push_back(std::make_unique<LiteralExpr>("x=1\n"));
            stmts.push_back(std::make_unique<ExprStmt>(std::make_unique<CallExpr>(std::make_unique<IdentifierExpr>("printf"), std::move(args))));
            cases.push_back(std::make_unique<CaseStmt>(std::make_unique<LiteralPattern>(std::make_unique<LiteralExpr>(int64_t(1))), std::make_unique<Block>(std::move(stmts))));
        }
        {
            std::vector<std::unique_ptr<Stmt>> stmts;
            std::vector<std::unique_ptr<Expr>> args;
            args.push_back(std::make_unique<LiteralExpr>("x=?\n"));
            stmts.push_back(std::make_unique<ExprStmt>(std::make_unique<CallExpr>(std::make_unique<IdentifierExpr>("printf"), std::move(args))));
            cases.push_back(std::make_unique<CaseStmt>(nullptr, std::make_unique<Block>(std::move(stmts)), true));
        }
        mainStmts.push_back(std::make_unique<SwitchStmt>(std::make_unique<IdentifierExpr>("x"), std::move(cases)));
    }

    mainStmts.push_back(std::make_unique<ReturnStmt>(std::make_unique<LiteralExpr>(int64_t(0))));
    
    auto mainFunc = std::make_unique<FunctionDecl>("main", Type::getI32(), std::vector<std::unique_ptr<Param>>(), std::make_unique<Block>(std::move(mainStmts)));
    
    std::cout << "Lowering AST to MIR..." << std::endl;
    builder.lower(mainFunc.get());
    
    std::cout << "Generating code..." << std::endl;
    CodeGenerator codeGen(module);
    codeGen.generate();
    
    std::cout << "Emitting object file..." << std::endl;
    codeGen.emitObjectFile("control_flow.obj");
    std::cout << "Done!" << std::endl;
}

int main() {
    try {
        testControlFlowNative();
    } catch (const std::exception& e) {
        std::cerr << "Native test error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
