#include "Sema/Sema.h"
#include "AST/Declarations.h"
#include <cassert>
#include <iostream>
#include <vector>

void testAnalyzeClassDecl() {
    using namespace chtholly;
    
    Sema sema;
    auto& st = sema.getSymbolTable();
    
    // class Person { let age: i32; fn show(self) {} }
    std::vector<std::unique_ptr<ASTNode>> members;
    members.push_back(std::make_unique<VarDecl>("age", Type::getI32(), nullptr, false));
    
    // Method show(self)
    std::vector<std::unique_ptr<Param>> params;
    // Parser creates self with StructType("Self")
    auto placeholderSelf = std::make_shared<StructType>("Self", std::vector<StructType::Field>{});
    params.push_back(std::make_unique<Param>("self", placeholderSelf));
    
    auto methodBody = std::make_unique<Block>(std::vector<std::unique_ptr<Stmt>>{});
    auto methodDecl = std::make_unique<MethodDecl>("show", Type::getVoid(), std::move(params), std::move(methodBody));
    members.push_back(std::move(methodDecl));
    
    auto classDecl = std::make_unique<ClassDecl>("Person", std::move(members));
    
    try {
        sema.analyze(classDecl.get());
    } catch (const std::exception& e) {
        std::cerr << "Sema error: " << e.what() << std::endl;
        throw;
    }
    
    // Verify registration
    auto registeredType = st.lookupType("Person");
    assert(registeredType != nullptr);
    assert(registeredType->isStruct());
    
    auto structType = std::dynamic_pointer_cast<StructType>(registeredType);
    assert(structType->getName() == "Person");
    assert(structType->getFields().size() == 1);
    assert(structType->getFields()[0].name == "age");
    
    assert(structType->getMethods().size() == 1);
    assert(structType->getMethods()[0].name == "show");
    
    // Verify Self resolution
    auto methodType = std::dynamic_pointer_cast<FunctionType>(structType->getMethods()[0].type);
    assert(methodType != nullptr);
    assert(methodType->getParamTypes().size() == 1);
    // Should point to Person struct
    assert(methodType->getParamTypes()[0] == registeredType);
    
    std::cout << "testAnalyzeClassDecl passed!" << std::endl;
}

void testAccessControl() {
    using namespace chtholly;
    
    Sema sema;
    auto& st = sema.getSymbolTable();
    
    // class A { pub let x: i32; let y: i32; }
    std::vector<std::unique_ptr<ASTNode>> members;
    members.push_back(std::make_unique<VarDecl>("x", Type::getI32(), nullptr, false, true)); // public
    members.push_back(std::make_unique<VarDecl>("y", Type::getI32(), nullptr, false, false)); // private
    
    auto classDecl = std::make_unique<ClassDecl>("A", std::move(members));
    sema.analyze(classDecl.get());
    
    // Mocking an external function trying to access members
    auto aType = st.lookupType("A");
    st.insert("a", aType, false);
    
    // a.x (public) -> Should pass
    auto aExpr = std::make_unique<IdentifierExpr>("a");
    auto accX = std::make_unique<MemberAccessExpr>(std::move(aExpr), "x");
    sema.checkExpr(accX.get());
    
    // a.y (private) -> Should fail
    auto aExpr2 = std::make_unique<IdentifierExpr>("a");
    auto accY = std::make_unique<MemberAccessExpr>(std::move(aExpr2), "y");
    
    try {
        sema.checkExpr(accY.get());
        assert(false && "Should have failed to access private member");
    } catch (const std::runtime_error& e) {
        std::string msg = e.what();
        assert(msg.find("Cannot access private field 'y'") != std::string::npos);
    }
    
    std::cout << "testAccessControl passed!" << std::endl;
}

int main() {
    testAnalyzeClassDecl();
    testAccessControl();
    return 0;
}