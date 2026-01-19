#include "Sema/Sema.h"
#include "Parser.h"
#include "AST/Statements.h"
#include <cassert>
#include <iostream>

void testArrayPointerSema() {
    using namespace chtholly;

    // 1. Array Literal and Indexing
    {
        std::string source = "let a = [1, 2, 3]; let b = a[0];";
        Parser parser(source);
        auto stmt1 = parser.parseStatement();
        auto stmt2 = parser.parseStatement();
        
        Sema sema;
        sema.analyze(stmt1.get());
        sema.analyze(stmt2.get());
        
        assert(stmt2->getKind() == ASTNodeKind::VarDecl);
        auto varB = static_cast<VarDecl*>(stmt2.get());
        assert(varB->getType()->isI32());
    }

    // 2. Address-of and Dereference
    {
        std::string source = "let x = 10; let p = &x; let y = *p;";
        Parser parser(source);
        auto stmt1 = parser.parseStatement();
        auto stmt2 = parser.parseStatement();
        auto stmt3 = parser.parseStatement();
        
        Sema sema;
        sema.analyze(stmt1.get());
        sema.analyze(stmt2.get());
        sema.analyze(stmt3.get());
        
        assert(stmt2->getKind() == ASTNodeKind::VarDecl);
        auto varP = static_cast<VarDecl*>(stmt2.get());
        assert(varP->getType()->isPointer());
        assert(std::static_pointer_cast<PointerType>(varP->getType())->getBaseType()->isI32());
        
        assert(stmt3->getKind() == ASTNodeKind::VarDecl);
        auto varY = static_cast<VarDecl*>(stmt3.get());
        assert(varY->getType()->isI32());
    }

    // 3. Intrinsics
    {
        std::string source = "let p = malloc[i32](10); let s = sizeof[i32](); free(p);";
        Parser parser(source);
        auto stmt1 = parser.parseStatement();
        auto stmt2 = parser.parseStatement();
        auto stmt3 = parser.parseStatement();
        
        Sema sema;
        sema.analyze(stmt1.get());
        sema.analyze(stmt2.get());
        sema.analyze(stmt3.get());
        
        assert(stmt1->getKind() == ASTNodeKind::VarDecl);
        auto varP = static_cast<VarDecl*>(stmt1.get());
        assert(varP->getType()->isPointer());
        assert(std::static_pointer_cast<PointerType>(varP->getType())->getBaseType()->isI32());
    }

    std::cout << "testArrayPointerSema passed!" << std::endl;
}

int main() {
    testArrayPointerSema();
    return 0;
}