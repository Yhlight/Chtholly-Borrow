#include "Sema/Substituter.h"
#include "Parser.h"
#include <cassert>
#include <iostream>

using namespace chtholly;

void testSubstituter() {
    std::string source = "fn add[T](a: T, b: T): T { return a + b; }";
    Parser parser(source);
    auto program = parser.parseProgram();
    assert(program.size() == 1);
    assert(program[0]->getKind() == ASTNodeKind::FunctionDecl);
    auto func = static_cast<FunctionDecl*>(program[0].get());
    
    std::map<std::string, std::shared_ptr<Type>> mapping;
    mapping["T"] = Type::getI32();
    
    ASTSubstituter substituter(mapping);
    auto specializedNode = substituter.substitute(func);
    assert(specializedNode->getKind() == ASTNodeKind::FunctionDecl);
    auto specFunc = static_cast<FunctionDecl*>(specializedNode.get());
    
    assert(specFunc->getName() == "add");
    assert(specFunc->getParams().size() == 2);
    assert(specFunc->getParams()[0]->getType()->isI32());
    assert(specFunc->getParams()[1]->getType()->isI32());
    assert(specFunc->getReturnType()->isI32());
    
    // Verify body expression
    auto body = specFunc->getBody();
    assert(body->getStatements()[0]->getKind() == ASTNodeKind::ReturnStmt);
    auto retStmt = static_cast<ReturnStmt*>(body->getStatements()[0].get());
    assert(retStmt->getExpression()->getKind() == ASTNodeKind::BinaryExpr);
    auto binExpr = static_cast<const BinaryExpr*>(retStmt->getExpression());
    // Note: Parser might not have set the type of BinaryExpr yet because Sema hasn't run.
    // But the children should be cloned.
    
    std::cout << "Original: " << func->toString() << std::endl;
    std::cout << "Specialized: " << specFunc->toString() << std::endl;
    
    std::cout << "testSubstituter passed!" << std::endl;
}

int main() {
    try {
        testSubstituter();
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
