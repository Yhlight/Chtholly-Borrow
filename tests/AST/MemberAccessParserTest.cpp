#include "Parser.h"
#include "AST/Expressions.h"
#include <cassert>
#include <iostream>

void testParseMemberAccess() {
    using namespace chtholly;
    
    std::string source = "p.x + p.y";
    Parser parser(source);
    
    auto expr = parser.parseExpression();
    assert(expr->getKind() == ASTNodeKind::BinaryExpr);
    auto binExpr = static_cast<BinaryExpr*>(expr.get());
    
    assert(binExpr->getLeft()->getKind() == ASTNodeKind::MemberAccessExpr);
    auto left = static_cast<const MemberAccessExpr*>(binExpr->getLeft());
    assert(binExpr->getRight()->getKind() == ASTNodeKind::MemberAccessExpr);
    auto right = static_cast<const MemberAccessExpr*>(binExpr->getRight());
    
    assert(left->getMemberName() == "x");
    assert(left->getBase()->getKind() == ASTNodeKind::IdentifierExpr);
    assert(static_cast<const IdentifierExpr*>(left->getBase())->toString() == "p");
    
    assert(right->getMemberName() == "y");
    assert(right->getBase()->getKind() == ASTNodeKind::IdentifierExpr);
    assert(static_cast<const IdentifierExpr*>(right->getBase())->toString() == "p");
    
    std::cout << "testParseMemberAccess passed!" << std::endl;
}

int main() {
    testParseMemberAccess();
    return 0;
}
