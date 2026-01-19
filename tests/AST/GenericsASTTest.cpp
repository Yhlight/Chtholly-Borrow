#include "AST/Types.h"
#include <cassert>
#include <iostream>

using namespace chtholly;

void testTypeParameterType() {
    auto t = std::make_shared<TypeParameterType>("T");
    assert(t->getName() == "T");
    assert(t->toString() == "T");
    assert(t->getKind() == TypeKind::TypeParameter);
    std::cout << "testTypeParameterType passed!" << std::endl;
}

int main() {
    try {
        testTypeParameterType();
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}