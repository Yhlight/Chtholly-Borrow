#include "AST/Expressions.h"
#include <cassert>
#include <iostream>

using namespace chtholly;

void testLiteralExpr() {
    auto intLiteral = std::make_unique<LiteralExpr>(42);
    assert(intLiteral->toString() == "42");
    
    auto floatLiteral = std::make_unique<LiteralExpr>(3.14);
    assert(floatLiteral->toString() == "3.140000"); // Standard double to string

    auto boolLiteral = std::make_unique<LiteralExpr>(true);
    assert(boolLiteral->toString() == "true");

    std::cout << "testLiteralExpr passed!" << std::endl;
}

void testIdentifierExpr() {
    auto idExpr = std::make_unique<IdentifierExpr>("x");
    assert(idExpr->toString() == "x");
    std::cout << "testIdentifierExpr passed!" << std::endl;
}

void testBinaryExpr() {
    auto left = std::make_unique<LiteralExpr>(1);
    auto right = std::make_unique<LiteralExpr>(2);
    auto binExpr = std::make_unique<BinaryExpr>(std::move(left), TokenType::Plus, std::move(right));
    
    assert(binExpr->toString() == "(1 + 2)");
    std::cout << "testBinaryExpr passed!" << std::endl;
}

void testCallExpr() {
    std::vector<std::unique_ptr<Expr>> args;
    args.push_back(std::make_unique<LiteralExpr>(1));
    args.push_back(std::make_unique<IdentifierExpr>("x"));
    
    auto callExpr = std::make_unique<CallExpr>("add", std::move(args));
    assert(callExpr->toString() == "add(1, x)");
    std::cout << "testCallExpr passed!" << std::endl;
}

int main() {
    testLiteralExpr();
    testIdentifierExpr();
    testBinaryExpr();
    testCallExpr();
    return 0;
}
