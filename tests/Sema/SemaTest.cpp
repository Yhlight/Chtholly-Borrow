#include "Sema/Sema.h"
#include "Parser.h"
#include <cassert>
#include <iostream>

using namespace chtholly;

void testSemaVarDecl() {
    std::string source = "let x: i32 = 10;";
    Parser parser(source);
    auto decl = parser.parseVarDecl();
    
    Sema sema;
    sema.analyze(decl.get());
    
    auto symbol = sema.getSymbolTable().lookup("x");
    assert(symbol != nullptr);
    assert(symbol->type == Type::getI32());

    std::cout << "testSemaVarDecl passed!" << std::endl;
}

void testSemaTypeMismatch() {
    // Explicit mismatch: let x: i32 = 1.0;
    std::string source = "let x: i32 = 1.0;";
    Parser parser(source);
    auto decl = parser.parseVarDecl();
    
    Sema sema;
    try {
        sema.analyze(decl.get());
        assert(false && "Should have thrown type mismatch error");
    } catch (const std::runtime_error& e) {
        std::cout << "Caught expected error: " << e.what() << std::endl;
    }

    std::cout << "testSemaTypeMismatch passed!" << std::endl;
}

void testSemaBlockScoping() {
    // let x: i32 = 1;
    // {
    //   let x: f64 = 2.0; // Shadowing
    // }
    // x should be i32 here
    std::string source = "let x: i32 = 1; { let x: f64 = 2.0; }";
    Parser parser(source);
    
    Sema sema;
    sema.analyze(parser.parseVarDecl().get()); // let x: i32 = 1;
    sema.analyze(parser.parseBlock().get());   // { let x: f64 = 2.0; }
    
    auto symbol = sema.getSymbolTable().lookup("x");
    assert(symbol != nullptr);
    assert(symbol->type == Type::getI32()); // Should be outer x

    std::cout << "testSemaBlockScoping passed!" << std::endl;
}

void testSemaConditionType() {
    // if 10 { ... } -> Should fail because 10 is i32, not bool
    std::string source = "if 10 { let x: i32 = 1; }";
    Parser parser(source);
    auto stmt = parser.parseIfStmt();
    
    Sema sema;
    try {
        sema.analyze(stmt.get());
        assert(false && "Should have thrown condition type mismatch error");
    } catch (const std::runtime_error& e) {
        std::cout << "Caught expected error: " << e.what() << std::endl;
    }

    // while 1.0 { ... } -> Should fail
    std::string sourceWhile = "while 1.0 { let x: i32 = 1; }";
    Parser parserWhile(sourceWhile);
    auto stmtWhile = parserWhile.parseWhileStmt();
    try {
        sema.analyze(stmtWhile.get());
        assert(false && "Should have thrown condition type mismatch error");
    } catch (const std::runtime_error& e) {
        std::cout << "Caught expected error: " << e.what() << std::endl;
    }

    std::cout << "testSemaConditionType passed!" << std::endl;
}

void testSemaFunctionDecl() {
    std::string source = "fn add(a: i32, b: i32): i32 { let x: i32 = a + b; return x; }";
    Parser parser(source);
    auto decl = parser.parseFunctionDecl();
    
    Sema sema;
    sema.analyze(decl.get());
    
    auto symbol = sema.getSymbolTable().lookup("add");
    assert(symbol != nullptr);
    // Function type checking will be refined in the next task
    
    std::cout << "testSemaFunctionDecl passed!" << std::endl;
}

void testSemaFunctionCallAndReturn() {
    Sema sema;
    
    // Register function: fn add(a: i32, b: i32): i32
    std::string sourceFunc = "fn add(a: i32, b: i32): i32 { return a + b; }";
    Parser parserFunc(sourceFunc);
    sema.analyze(parserFunc.parseFunctionDecl().get());

    // Test valid call: add(1, 2)
    std::string sourceCall = "add(1, 2)";
    Parser parserCall(sourceCall);
    auto callExpr = parserCall.parseExpression();
    assert(sema.checkExpr(callExpr.get()) == Type::getI32());

    // Test invalid call (type mismatch): add(1, 2.0)
    std::string sourceInvalidCall = "add(1, 2.0)";
    Parser parserInvalidCall(sourceInvalidCall);
    auto invalidCallExpr = parserInvalidCall.parseExpression();
    try {
        sema.checkExpr(invalidCallExpr.get());
        assert(false && "Should have thrown arg type mismatch error");
    } catch (const std::runtime_error& e) {
        std::cout << "Caught expected error: " << e.what() << std::endl;
    }

    // Test invalid call (arg count mismatch): add(1)
    std::string sourceCountMismatch = "add(1)";
    Parser parserCountMismatch(sourceCountMismatch);
    auto countMismatchExpr = parserCountMismatch.parseExpression();
    try {
        sema.checkExpr(countMismatchExpr.get());
        assert(false && "Should have thrown arg count mismatch error");
    } catch (const std::runtime_error& e) {
        std::cout << "Caught expected error: " << e.what() << std::endl;
    }

    std::cout << "testSemaFunctionCallAndReturn passed!" << std::endl;
}

int main() {
    testSemaVarDecl();
    testSemaTypeMismatch();
    testSemaBlockScoping();
    testSemaConditionType();
    testSemaFunctionDecl();
    testSemaFunctionCallAndReturn();
    return 0;
}
