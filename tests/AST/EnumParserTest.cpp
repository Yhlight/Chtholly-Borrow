#include "Parser.h"
#include "AST/Declarations.h"
#include <cassert>
#include <iostream>

void testEnumParser() {
    using namespace chtholly;

    // 1. Basic Enum
    {
        std::string source = "enum Color { Red, Green, Blue }";
        Parser parser(source);
        auto program = parser.parseProgram();
        assert(program.size() == 1);
        auto enumDecl = static_cast<EnumDecl*>(program[0].get());
        assert(enumDecl->getKind() == ASTNodeKind::EnumDecl);
        assert(enumDecl->getName() == "Color");
        assert(enumDecl->getVariants().size() == 3);
        assert(enumDecl->getVariants()[0]->getName() == "Red");
        assert(enumDecl->getVariants()[0]->getVariantKind() == EnumVariant::VariantKind::Unit);
    }

    // 2. Enum with Tuple and Struct variants
    {
        std::string source = R"(
            enum Message {
                Quit,
                Move(i32, i32),
                Write(string),
                ChangeColor { r: i32, g: i32, b: i32 }
            }
        )";
        Parser parser(source);
        auto program = parser.parseProgram();
        assert(program.size() == 1);
        auto enumDecl = static_cast<EnumDecl*>(program[0].get());
        assert(enumDecl->getKind() == ASTNodeKind::EnumDecl);
        assert(enumDecl->getVariants().size() == 4);

        // Move(i32, i32)
        auto v1 = enumDecl->getVariants()[1].get();
        assert(v1->getName() == "Move");
        assert(v1->getVariantKind() == EnumVariant::VariantKind::Tuple);
        assert(v1->getTupleTypes().size() == 2);

        // ChangeColor { ... }
        auto v3 = enumDecl->getVariants()[3].get();
        assert(v3->getName() == "ChangeColor");
        assert(v3->getVariantKind() == EnumVariant::VariantKind::Struct);
        assert(v3->getStructFields().size() == 3);
        assert(v3->getStructFields()[0]->getName() == "r");
    }

    std::cout << "testEnumParser passed!" << std::endl;
}

int main() {
    testEnumParser();
    return 0;
}
