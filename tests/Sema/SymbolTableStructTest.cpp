#include "Sema/SymbolTable.h"
#include "AST/Types.h"
#include <cassert>
#include <iostream>
#include <vector>

void testSymbolTableStruct() {
    using namespace chtholly;
    
    SymbolTable st;
    
    // Test Type registration
    std::vector<StructType::Field> fields;
    fields.push_back({"x", Type::getI32()});
    fields.push_back({"y", Type::getI32()});
    
    auto structType = std::make_shared<StructType>("Point", std::move(fields));
    bool inserted = st.insertType("Point", structType);
    assert(inserted == true);
    
    auto lookedUp = st.lookupType("Point");
    assert(lookedUp != nullptr);
    assert(lookedUp->isStruct());
    assert(lookedUp->toString() == "Point");
    
    auto lookedUpStruct = std::dynamic_pointer_cast<StructType>(lookedUp);
    assert(lookedUpStruct != nullptr);
    assert(lookedUpStruct->getFields().size() == 2);
    assert(lookedUpStruct->getFields()[0].name == "x");
    
    // Test re-insertion failure
    bool reInserted = st.insertType("Point", structType);
    assert(reInserted == false);
    
    // Test lookup in nested scope
    st.pushScope();
    auto lookedUpNested = st.lookupType("Point");
    assert(lookedUpNested == structType);
    st.popScope();
    
    std::cout << "testSymbolTableStruct passed!" << std::endl;
}

int main() {
    testSymbolTableStruct();
    return 0;
}