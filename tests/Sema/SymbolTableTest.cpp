#include "Sema/SymbolTable.h"
#include "AST/Types.h"
#include <cassert>
#include <iostream>

using namespace chtholly;

void testSymbolTable() {
    SymbolTable symbolTable;
    auto i32Type = Type::getI32();
    
    // Test global scope
    assert(symbolTable.insert("x", i32Type, false));
    assert(!symbolTable.insert("x", i32Type, false)); // Duplicate
    
    auto symbol = symbolTable.lookup("x");
    assert(symbol != nullptr);
    assert(symbol->type == i32Type);
    assert(!symbol->isMutable);

    // Test nested scope
    symbolTable.pushScope();
    assert(symbolTable.insert("y", Type::getF64(), true));
    assert(symbolTable.lookup("y") != nullptr);
    assert(symbolTable.lookup("x") != nullptr); // Should find in outer scope

    // Shadowing
    assert(symbolTable.insert("x", Type::getBool(), true));
    auto shadowed = symbolTable.lookup("x");
    assert(shadowed->type == Type::getBool());

    symbolTable.popScope();
    auto original = symbolTable.lookup("x");
    assert(original->type == i32Type);
    assert(symbolTable.lookup("y") == nullptr);

    std::cout << "testSymbolTable passed!" << std::endl;
}

int main() {
    testSymbolTable();
    return 0;
}
