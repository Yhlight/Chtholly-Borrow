#include "Sema/Sema.h"
#include "AST/Declarations.h"
#include <cassert>
#include <iostream>
#include <vector>

void testAnalyzeStructDecl() {
    using namespace chtholly;
    
    Sema sema;
    auto& st = sema.getSymbolTable();
    
    // struct Point { let x: i32; let y: i32; }
    std::vector<std::unique_ptr<VarDecl>> members;
    members.push_back(std::make_unique<VarDecl>("x", Type::getI32(), nullptr, false));
    members.push_back(std::make_unique<VarDecl>("y", Type::getI32(), nullptr, false));
    
    auto structDecl = std::make_unique<StructDecl>("Point", std::move(members));
    
    sema.analyze(structDecl.get());
    
    auto registeredType = st.lookupType("Point");
    assert(registeredType != nullptr);
    assert(registeredType->isStruct());
    
    auto structType = std::dynamic_pointer_cast<StructType>(registeredType);
    assert(structType != nullptr);
    assert(structType->getFields().size() == 2);
    assert(structType->getFields()[0].name == "x");
    
    std::cout << "testAnalyzeStructDecl passed!" << std::endl;
}

int main() {
    testAnalyzeStructDecl();
    return 0;
}