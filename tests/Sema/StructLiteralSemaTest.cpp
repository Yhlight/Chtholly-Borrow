#include "Sema/Sema.h"
#include "AST/Declarations.h"
#include "AST/Expressions.h"
#include <cassert>
#include <iostream>
#include <vector>

void testStructLiteralSema() {
    using namespace chtholly;
    
    Sema sema;
    auto& st = sema.getSymbolTable();
    
    // 1. Register struct Point { let x: i32; let y: i32; }
    std::vector<StructType::Field> fields;
    fields.push_back({"x", Type::getI32()});
    fields.push_back({"y", Type::getI32()});
    auto pointType = std::make_shared<StructType>("Point", std::move(fields));
    st.insertType("Point", pointType);
    
    // 2. Test Point { x: 1, y: 2 }
    std::vector<StructLiteralExpr::FieldInit> inits;
    inits.push_back({"x", std::make_unique<LiteralExpr>(int64_t(1))});
    inits.push_back({"y", std::make_unique<LiteralExpr>(int64_t(2))});
    auto structLiteral = std::make_unique<StructLiteralExpr>(std::make_unique<IdentifierExpr>("Point"), std::move(inits));
    
    auto type = sema.checkExpr(structLiteral.get());
    assert(type == pointType);
    
    // 3. Test Type Mismatch: Point { x: 1.0, y: 2 }
    std::vector<StructLiteralExpr::FieldInit> initsMismatch;
    initsMismatch.push_back({"x", std::make_unique<LiteralExpr>(1.0)});
    initsMismatch.push_back({"y", std::make_unique<LiteralExpr>(int64_t(2))});
    auto structLiteralMismatch = std::make_unique<StructLiteralExpr>(std::make_unique<IdentifierExpr>("Point"), std::move(initsMismatch));
    try {
        sema.checkExpr(structLiteralMismatch.get());
        assert(false && "Should have thrown type mismatch error");
    } catch (const std::runtime_error& e) {
        std::cout << "Caught expected error: " << e.what() << std::endl;
    }
    
    // 4. Test Missing Field: Point { x: 1 }
    std::vector<StructLiteralExpr::FieldInit> initsMissing;
    initsMissing.push_back({"x", std::make_unique<LiteralExpr>(int64_t(1))});
    auto structLiteralMissing = std::make_unique<StructLiteralExpr>(std::make_unique<IdentifierExpr>("Point"), std::move(initsMissing));
    try {
        sema.checkExpr(structLiteralMissing.get());
        assert(false && "Should have thrown missing field error");
    } catch (const std::runtime_error& e) {
        std::cout << "Caught expected error: " << e.what() << std::endl;
    }
    
    std::cout << "testStructLiteralSema passed!" << std::endl;
}

int main() {
    testStructLiteralSema();
    return 0;
}