#include "AST/Declarations.h"
#include "AST/Types.h"
#include "Parser.h"
#include <cassert>
#include <iostream>

void testClassAST() {
    using namespace chtholly;
    
    // Test MethodDecl
    auto method = std::make_unique<MethodDecl>("show", Type::getVoid(), std::vector<std::unique_ptr<Param>>{}, nullptr);
    assert(method->getName() == "show");
    assert(method->getReturnType()->isVoid());
    
    // Test ClassDecl
    std::vector<std::unique_ptr<ASTNode>> members;
    members.push_back(std::make_unique<VarDecl>("age", Type::getI32(), nullptr, false));
    members.push_back(std::move(method));
    
    auto classDecl = std::make_unique<ClassDecl>("Person", std::move(members));
    assert(classDecl->getName() == "Person");
    assert(classDecl->getMembers().size() == 2);
    
    std::cout << "testClassAST passed!" << std::endl;
}

void testParseClassDecl() {
    using namespace chtholly;
    
    std::string source = "class Person { let age: i32; fn show() {} }";
    Parser parser(source);
    
    auto stmt = parser.parseStatement();
    assert(stmt->getKind() == ASTNodeKind::ClassDecl);
    auto classDecl = static_cast<ClassDecl*>(stmt.get());
    
    assert(classDecl->getName() == "Person");
    assert(classDecl->getMembers().size() == 2);
    
    // Check members
    assert(classDecl->getMembers()[0]->getKind() == ASTNodeKind::VarDecl);
    auto varDecl = static_cast<VarDecl*>(classDecl->getMembers()[0].get());
    assert(varDecl->getName() == "age");
    
    assert(classDecl->getMembers()[1]->getKind() == ASTNodeKind::MethodDecl);
    auto methodDecl = static_cast<MethodDecl*>(classDecl->getMembers()[1].get());
    assert(methodDecl->getName() == "show");
    
    std::cout << "testParseClassDecl passed!" << std::endl;
}

void testParseConstructor() {
    using namespace chtholly;

    std::string source = "class Point { Point(x: i32, y: i32) {} }";
    Parser parser(source);
    
    auto stmt = parser.parseStatement();
    assert(stmt->getKind() == ASTNodeKind::ClassDecl);
    auto classDecl = static_cast<ClassDecl*>(stmt.get());
    
    assert(classDecl->getMembers().size() == 1);
    assert(classDecl->getMembers()[0]->getKind() == ASTNodeKind::ConstructorDecl);
    auto ctor = static_cast<ConstructorDecl*>(classDecl->getMembers()[0].get());
    
    assert(ctor != nullptr);
    assert(ctor->getName() == "Point");
    assert(ctor->getParams().size() == 2);
    assert(ctor->getParams()[0]->getName() == "x");
    assert(ctor->getParams()[1]->getName() == "y");
    
    std::cout << "testParseConstructor passed!" << std::endl;
}

void testParseMethodReceivers() {
    using namespace chtholly;

    // 1. Test &self
    {
        std::string source = "class A { fn f(&self) {} }";
        Parser parser(source);
        auto stmt = parser.parseStatement();
        assert(stmt->getKind() == ASTNodeKind::ClassDecl);
        auto classDecl = static_cast<ClassDecl*>(stmt.get());
        assert(classDecl->getMembers()[0]->getKind() == ASTNodeKind::MethodDecl);
        auto method = static_cast<MethodDecl*>(classDecl->getMembers()[0].get());
        
        assert(method->getParams().size() == 1);
        assert(method->getParams()[0]->getName() == "self");
        // Verify type is pointer to Self (struct named Self)
        auto type = method->getParams()[0]->getType();
        assert(type->isPointer());
    }

    // 2. Test self (value)
    {
        std::string source = "class B { fn g(self) {} }";
        Parser parser(source);
        auto stmt = parser.parseStatement();
        assert(stmt->getKind() == ASTNodeKind::ClassDecl);
        auto classDecl = static_cast<ClassDecl*>(stmt.get());
        assert(classDecl->getMembers()[0]->getKind() == ASTNodeKind::MethodDecl);
        auto method = static_cast<MethodDecl*>(classDecl->getMembers()[0].get());
        
        assert(method->getParams().size() == 1);
        assert(method->getParams()[0]->getName() == "self");
        auto type = method->getParams()[0]->getType();
        assert(type->isStruct());
        assert(std::dynamic_pointer_cast<StructType>(type)->getName() == "Self");
    }

    // 3. Test &mut self
    {
        std::string source = "class C { fn h(&mut self) {} }";
        Parser parser(source);
        auto stmt = parser.parseStatement();
        assert(stmt->getKind() == ASTNodeKind::ClassDecl);
        auto classDecl = static_cast<ClassDecl*>(stmt.get());
        assert(classDecl->getMembers()[0]->getKind() == ASTNodeKind::MethodDecl);
        auto method = static_cast<MethodDecl*>(classDecl->getMembers()[0].get());
        
        assert(method->getParams().size() == 1);
        assert(method->getParams()[0]->getName() == "self");
        auto type = method->getParams()[0]->getType();
        assert(type->isPointer());
    }
    
    // 4. Test self with other params
    {
        std::string source = "class D { fn i(&self, x: i32) {} }";
        Parser parser(source);
        auto stmt = parser.parseStatement();
        assert(stmt->getKind() == ASTNodeKind::ClassDecl);
        auto classDecl = static_cast<ClassDecl*>(stmt.get());
        assert(classDecl->getMembers()[0]->getKind() == ASTNodeKind::MethodDecl);
        auto method = static_cast<MethodDecl*>(classDecl->getMembers()[0].get());
        
        assert(method->getParams().size() == 2);
        assert(method->getParams()[0]->getName() == "self");
        assert(method->getParams()[1]->getName() == "x");
    }

    std::cout << "testParseMethodReceivers passed!" << std::endl;
}

int main() {
    testClassAST();
    testParseClassDecl();
    testParseConstructor();
    testParseMethodReceivers();
    return 0;
}
