#include "Sema/Sema.h"
#include "Parser.h"
#include "AST/Declarations.h"
#include <cassert>
#include <iostream>

void testEnumSema() {
    using namespace chtholly;

    // 1. Basic Enum Registration
    {
        std::string source = "enum Color { Red, Green, Blue }";
        Parser parser(source);
        auto program = parser.parseProgram();
        
        Sema sema;
        sema.analyze(program[0].get());
        
        auto type = sema.getSymbolTable().lookupType("Color");
        assert(type != nullptr);
        assert(type->isEnum());
        auto enumTy = std::dynamic_pointer_cast<EnumType>(type);
        assert(enumTy->getVariants().size() == 3);
        assert(enumTy->findVariant("Red") != nullptr);
    }

    // 2. Redefinition Error
    {
        std::string source = "enum Color { Red } enum Color { Blue }";
        Parser parser(source);
        auto program = parser.parseProgram();
        
        Sema sema;
        sema.analyze(program[0].get());
        try {
            sema.analyze(program[1].get());
            assert(false && "Should have thrown redefinition error");
        } catch (const std::exception& e) {
            std::cout << "Caught expected error: " << e.what() << std::endl;
        }
    }

    std::cout << "testEnumSema passed!" << std::endl;
}

int main() {
    testEnumSema();
    return 0;
}
