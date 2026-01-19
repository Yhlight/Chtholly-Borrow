#include "AST/Types.h"
#include <cassert>
#include <iostream>

using namespace chtholly;

void testPrimitiveTypes() {
    auto i32Type = Type::getI32();
    auto f64Type = Type::getF64();
    auto boolType = Type::getBool();

    assert(i32Type->isInteger());
    assert(f64Type->isFloatingPoint());
    assert(boolType->isBoolean());

    assert(i32Type->toString() == "i32");
    assert(f64Type->toString() == "f64");
    assert(boolType->toString() == "bool");

    std::cout << "testPrimitiveTypes passed!" << std::endl;
}

int main() {
    testPrimitiveTypes();
    return 0;
}
