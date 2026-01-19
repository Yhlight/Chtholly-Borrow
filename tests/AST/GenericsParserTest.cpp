#include "Parser.h"
#include <cassert>
#include <iostream>

using namespace chtholly;

void testParseGenericFunction() {
    std::string source = "fn add[T](a: T, b: T): T { return a + b; }";
    Parser parser(source);
    auto program = parser.parseProgram();
    
    assert(program.size() == 1);
    assert(program[0]->getKind() == ASTNodeKind::FunctionDecl);
    auto func = static_cast<FunctionDecl*>(program[0].get());
    assert(func->getName() == "add");
    assert(func->getGenericParams().size() == 1);
    assert(func->getGenericParams()[0].name == "T");

    // Verify parameter types are TypeParameterType
    assert(func->getParams().size() == 2);
    assert(func->getParams()[0]->getType()->isTypeParameter());
    assert(func->getParams()[0]->getType()->toString() == "T");
    assert(func->getReturnType()->isTypeParameter());
    assert(func->getReturnType()->toString() == "T");

    std::cout << "testParseGenericFunction passed!" << std::endl;
}

void testParseGenericStruct() {
    std::string source = "struct Point[T] { x: T, y: T }";
    Parser parser(source);
    auto program = parser.parseProgram();
    
    assert(program.size() == 1);
    assert(program[0]->getKind() == ASTNodeKind::StructDecl);
    auto st = static_cast<StructDecl*>(program[0].get());
    assert(st->getName() == "Point");
    assert(st->getGenericParams().size() == 1);
    assert(st->getGenericParams()[0].name == "T");

    // Verify field types
    assert(st->getMembers().size() == 2);
    assert(st->getMembers()[0]->getType()->isTypeParameter());
    assert(st->getMembers()[0]->getType()->toString() == "T");

    std::cout << "testParseGenericStruct passed!" << std::endl;
}

void testParseGenericCall() {
    std::string source = "add[i32](1, 2)";
    Parser parser(source);
    auto expr = parser.parseExpression();
    
    assert(expr->getKind() == ASTNodeKind::CallExpr);
    auto call = static_cast<CallExpr*>(expr.get());
    assert(call->getCallee()->getKind() == ASTNodeKind::SpecializationExpr);
    auto spec = static_cast<const SpecializationExpr*>(call->getCallee());
    assert(spec->getTypeArgs().size() == 1);
    assert(spec->getTypeArgs()[0]->toString() == "i32");
    std::cout << "testParseGenericCall passed!" << std::endl;
}

void testParseGenericStructLiteral() {
    std::string source = "Point[f64] { x: 1.0, y: 2.0 }";
    Parser parser(source);
    auto expr = parser.parseExpression();
    
    assert(expr->getKind() == ASTNodeKind::StructLiteralExpr);
    auto lit = static_cast<StructLiteralExpr*>(expr.get());
    assert(lit->getBase()->getKind() == ASTNodeKind::SpecializationExpr);
    auto spec = static_cast<const SpecializationExpr*>(lit->getBase());
    assert(spec->getTypeArgs().size() == 1);
    assert(spec->getTypeArgs()[0]->toString() == "f64");
    assert(lit->getFields().size() == 2);
    std::cout << "testParseGenericStructLiteral passed!" << std::endl;
}

int main() {
    try {
        testParseGenericFunction();
        testParseGenericStruct();
        testParseGenericCall();
        testParseGenericStructLiteral();
    } catch (const std::exception& e) {
        std::cout << "Caught expected exception: " << e.what() << std::endl;
    }
    return 0;
}
