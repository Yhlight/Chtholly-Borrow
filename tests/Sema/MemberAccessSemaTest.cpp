#include "Sema/Sema.h"
#include "AST/Declarations.h"
#include "AST/Expressions.h"
#include <cassert>
#include <iostream>
#include <vector>

void testMemberAccessSema() {
    using namespace chtholly;
    
    Sema sema;
    auto& st = sema.getSymbolTable();
    
    // 1. Register struct Point { let x: i32; let y: i32; }
    std::vector<StructType::Field> fields;
    fields.push_back({"x", Type::getI32()});
    fields.push_back({"y", Type::getI32()});
    auto pointType = std::make_shared<StructType>("Point", std::move(fields));
    st.insertType("Point", pointType);
    
    // 2. Register let p: Point;
    st.insert("p", pointType, false);
    
    // 3. Test p.x
    auto pExpr = std::make_unique<IdentifierExpr>("p");
    auto memberAccess = std::make_unique<MemberAccessExpr>(std::move(pExpr), "x");
    
    auto type = sema.checkExpr(memberAccess.get());
    assert(type == Type::getI32());
    
    // 4. Test p.z (non-existent member)
    auto pExpr2 = std::make_unique<IdentifierExpr>("p");
    auto invalidMemberAccess = std::make_unique<MemberAccessExpr>(std::move(pExpr2), "z");
    try {
        sema.checkExpr(invalidMemberAccess.get());
        assert(false && "Should have thrown error for non-existent member");
    } catch (const std::runtime_error& e) {
        std::cout << "Caught expected error: " << e.what() << std::endl;
    }
    
    std::cout << "testMemberAccessSema passed!" << std::endl;
}

int main() {
    testMemberAccessSema();
    return 0;
}
