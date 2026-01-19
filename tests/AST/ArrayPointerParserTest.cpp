#include "Parser.h"
#include "AST/Expressions.h"
#include "AST/Statements.h"
#include <cassert>
#include <iostream>

void testArrayPointerParser() {
    using namespace chtholly;
    
    // 1. Array Type T[N]
    {
        std::string source = "let a: i32[10];";
        Parser parser(source);
        auto stmt = parser.parseStatement();
        assert(stmt->getKind() == ASTNodeKind::VarDecl);
        auto varDecl = static_cast<VarDecl*>(stmt.get());
        assert(varDecl->getType()->isArray());
        auto arrayTy = std::dynamic_pointer_cast<ArrayType>(varDecl->getType());
        assert(arrayTy->getSize() == 10);
    }

    // 2. Array Literal
    {
        std::string source = "let a = [1, 2, 3];";
        Parser parser(source);
        auto stmt = parser.parseStatement();
        assert(stmt->getKind() == ASTNodeKind::VarDecl);
        auto varDecl = static_cast<VarDecl*>(stmt.get());
        assert(varDecl->getInitializer() != nullptr);
        assert(varDecl->getInitializer()->getKind() == ASTNodeKind::ArrayLiteralExpr);
    }

    // 3. Indexing
    {
        std::string source = "a[0] = 1;";
        Parser parser(source);
        auto stmt = parser.parseStatement();
        assert(stmt->getKind() == ASTNodeKind::ExprStmt);
        auto exprStmt = static_cast<ExprStmt*>(stmt.get());
        assert(exprStmt->getExpression()->getKind() == ASTNodeKind::BinaryExpr);
        auto bin = static_cast<const BinaryExpr*>(exprStmt->getExpression());
        assert(bin->getLeft()->getKind() == ASTNodeKind::IndexingExpr);
    }

    // 4. Unary & and *
    {
        std::string source = "let p = &x; let v = *p;";
        Parser parser(source);
        auto stmt1 = parser.parseStatement();
        assert(stmt1->getKind() == ASTNodeKind::VarDecl);
        auto var1 = static_cast<VarDecl*>(stmt1.get());
        assert(var1->getInitializer()->getKind() == ASTNodeKind::AddressOfExpr);
        
        auto stmt2 = parser.parseStatement();
        assert(stmt2->getKind() == ASTNodeKind::VarDecl);
        auto var2 = static_cast<VarDecl*>(stmt2.get());
        assert(var2->getInitializer()->getKind() == ASTNodeKind::DereferenceExpr);
    }

    // 5. Memory Intrinsics
    {
        std::string source = "let s = sizeof[i32](); let p = malloc[i32](10); free(p);";
        Parser parser(source);
        
        auto stmt1 = parser.parseStatement();
        assert(stmt1->getKind() == ASTNodeKind::VarDecl);
        auto var1 = static_cast<VarDecl*>(stmt1.get());
        assert(var1->getInitializer()->getKind() == ASTNodeKind::IntrinsicExpr);
        auto intrinsic1 = static_cast<const IntrinsicExpr*>(var1->getInitializer());
        assert(intrinsic1->getIntrinsicKind() == IntrinsicExpr::IntrinsicKind::Sizeof);
        
        auto stmt2 = parser.parseStatement();
        assert(stmt2->getKind() == ASTNodeKind::VarDecl);
        auto var2 = static_cast<VarDecl*>(stmt2.get());
        assert(var2->getInitializer()->getKind() == ASTNodeKind::IntrinsicExpr);
        auto intrinsic2 = static_cast<const IntrinsicExpr*>(var2->getInitializer());
        assert(intrinsic2->getIntrinsicKind() == IntrinsicExpr::IntrinsicKind::Malloc);
        assert(intrinsic2->getTypeArg() != nullptr);
        
        auto stmt3 = parser.parseStatement();
        assert(stmt3->getKind() == ASTNodeKind::ExprStmt);
        auto exprStmt = static_cast<ExprStmt*>(stmt3.get());
        assert(exprStmt->getExpression()->getKind() == ASTNodeKind::IntrinsicExpr);
        auto intrinsic3 = static_cast<const IntrinsicExpr*>(exprStmt->getExpression());
        assert(intrinsic3->getIntrinsicKind() == IntrinsicExpr::IntrinsicKind::Free);
    }

    std::cout << "testArrayPointerParser passed!" << std::endl;
}

int main() {
    testArrayPointerParser();
    return 0;
}