#include "Sema/Sema.h"
#include "Parser.h"
#include "AST/Declarations.h"
#include <cassert>
#include <iostream>

using namespace chtholly;

void testGenericEnumMonomorphization() {
    std::string source = R"(
enum Result[T, E] {
    Ok(T),
    Err(E)
}

fn main() {
    let x = Result[i32, bool]::Ok(42);
    let y = Result[i32, bool]::Err(true);
}
)";
    Parser parser(source);
    auto program = parser.parseProgram();
    
    Sema sema;
    for (const auto& node : program) {
        sema.analyze(node.get());
    }
    
    // Check if Result_i32_bool was created
    auto type = sema.getSymbolTable().lookupType("Result_i32_bool");
    assert(type != nullptr);
    assert(type->isEnum());
    auto enumTy = std::dynamic_pointer_cast<EnumType>(type);
    assert(enumTy->getVariants().size() == 2);
    
    auto okVariant = enumTy->findVariant("Ok");
    assert(okVariant != nullptr);
    assert(okVariant->kind == EnumType::Variant::Kind::Tuple);
    assert(okVariant->tupleTypes[0]->isI32());
    
    auto errVariant = enumTy->findVariant("Err");
    assert(errVariant != nullptr);
    assert(errVariant->kind == EnumType::Variant::Kind::Tuple);
    assert(errVariant->tupleTypes[0]->isBoolean());
    
    std::cout << "testGenericEnumMonomorphization passed!" << std::endl;
}

int main() {
    try {
        testGenericEnumMonomorphization();
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
