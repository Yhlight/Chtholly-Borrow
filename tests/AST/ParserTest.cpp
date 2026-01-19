#include "Parser.h"
#include <cassert>
#include <iostream>

using namespace chtholly;

void testParseVarDecl() {
    std::string source = "let x: i32 = 10;";
    Parser parser(source);
    auto decl = parser.parseVarDecl();
    
    assert(decl != nullptr);
    assert(decl->getName() == "x");
    assert(decl->getType()->toString() == "i32");
    assert(decl->isMutable() == false);
    assert(decl->getInitializer() != nullptr);
    assert(decl->getInitializer()->toString() == "10");

    std::string sourceMut = "let mut y: f64 = 1.0;";
    Parser parserMut(sourceMut);
    auto declMut = parserMut.parseVarDecl();
    assert(declMut != nullptr);
    assert(declMut->getName() == "y");
    assert(declMut->getType()->toString() == "f64");
    assert(declMut->isMutable() == true);

    std::cout << "testParseVarDecl passed!" << std::endl;
}

void testParseArithmeticExpr() {
    std::string source = "1 + 2 * 3";
    Parser parser(source);
    auto expr = parser.parseExpression();
    
    assert(expr != nullptr);
    assert(expr->toString() == "(1 + (2 * 3))");

    std::string source2 = "1 * 2 + 3";
    Parser parser2(source2);
    auto expr2 = parser2.parseExpression();
    assert(expr2 != nullptr);
    assert(expr2->toString() == "((1 * 2) + 3)");

    std::cout << "testParseArithmeticExpr passed!" << std::endl;
}

void testParseIfStmt() {
    std::string source = "if true { let x: i32 = 1; } else { let y: i32 = 2; }";
    Parser parser(source);
    auto stmt = parser.parseIfStmt();
    
    assert(stmt != nullptr);
    assert(stmt->toString() == "if true {\n  let x: i32 = 1;\n} else {\n  let y: i32 = 2;\n}");

    std::string sourceNoElse = "if false { let z: i32 = 3; }";
    Parser parserNoElse(sourceNoElse);
    auto stmtNoElse = parserNoElse.parseIfStmt();
    assert(stmtNoElse != nullptr);
    assert(stmtNoElse->toString() == "if false {\n  let z: i32 = 3;\n}");

    std::cout << "testParseIfStmt passed!" << std::endl;
}

void testParseWhileStmt() {
    std::string source = "while true { let x: i32 = 1; }";
    Parser parser(source);
    auto stmt = parser.parseWhileStmt();
    
    assert(stmt != nullptr);
    assert(stmt->toString() == "while true {\n  let x: i32 = 1;\n}");

    std::cout << "testParseWhileStmt passed!" << std::endl;
}

void testParseFunctionDecl() {
    std::string source = "fn add(a: i32, b: i32): i32 { return a + b; }";
    Parser parser(source);
    auto decl = parser.parseFunctionDecl();
    
    assert(decl != nullptr);
    assert(decl->getName() == "add");
    assert(decl->getParams().size() == 2);
    assert(decl->getParams()[0]->getName() == "a");
    assert(decl->getParams()[1]->getName() == "b");
    assert(decl->getReturnType()->toString() == "i32");
    
    std::cout << "testParseFunctionDecl passed!" << std::endl;
}

void testParseFunctionCall() {
    std::string source = "add(1, x)";
    Parser parser(source);
    auto expr = parser.parseExpression();
    
    assert(expr != nullptr);
    assert(expr->toString() == "add(1, x)");
    
    std::cout << "testParseFunctionCall passed!" << std::endl;
}

int main() {
    testParseVarDecl();
    testParseArithmeticExpr();
    testParseIfStmt();
    testParseWhileStmt();
    testParseFunctionDecl();
    testParseFunctionCall();
    return 0;
}
