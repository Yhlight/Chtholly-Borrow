#include "Parser.h"
#include "AST/Statements.h"
#include <cassert>
#include <iostream>

void testParseControlFlow() {
    using namespace chtholly;
    
    try {
        // Test Break/Continue
        std::cout << "Testing Break/Continue..." << std::endl;
        {
            std::string source = "{ break; continue; }";
            Parser parser(source);
            auto block = parser.parseBlock();
            assert(block->getStatements()[0]->getKind() == ASTNodeKind::BreakStmt);
            assert(block->getStatements()[1]->getKind() == ASTNodeKind::ContinueStmt);
        }

        // Test Do-While
        std::cout << "Testing Do-While..." << std::endl;
        {
            std::string source = "do { } while (true);";
            Parser parser(source);
            auto stmt = parser.parseStatement();
            assert(stmt->getKind() == ASTNodeKind::DoWhileStmt);
            auto doWhile = static_cast<DoWhileStmt*>(stmt.get());
            assert(doWhile->getCondition() != nullptr);
        }

        // Test For
        std::cout << "Testing For..." << std::endl;
        {
            std::string source = "for (let i = 0; i < 10; i = i + 1) { }";
            Parser parser(source);
            auto stmt = parser.parseStatement();
            assert(stmt->getKind() == ASTNodeKind::ForStmt);
            auto forStmt = static_cast<ForStmt*>(stmt.get());
            assert(forStmt->getInit() != nullptr);
            assert(forStmt->getCondition() != nullptr);
            assert(forStmt->getStep() != nullptr);
        }

        // Test Switch
        std::cout << "Testing Switch..." << std::endl;
        {
            std::string source = "switch (x) { case 1: break; default: break; }";
            Parser parser(source);
            auto stmt = parser.parseStatement();
            assert(stmt->getKind() == ASTNodeKind::SwitchStmt);
            auto switchStmt = static_cast<SwitchStmt*>(stmt.get());
            assert(switchStmt->getCases().size() == 2);
            assert(!switchStmt->getCases()[0]->isDefaultCase());
            assert(switchStmt->getCases()[1]->isDefaultCase());
        }

        std::cout << "testParseControlFlow passed!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        exit(1);
    }
}

int main() {
    testParseControlFlow();
    return 0;
}