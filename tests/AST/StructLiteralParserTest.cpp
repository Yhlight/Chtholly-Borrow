#include "Parser.h"
#include "AST/Expressions.h"
#include <cassert>
#include <iostream>

void testParseStructLiteral() {
    using namespace chtholly;
    
    std::string source = "Point { x: 1, y: 2 }";
    Parser parser(source);
    
    auto expr = parser.parseExpression();
    assert(expr->getKind() == ASTNodeKind::StructLiteralExpr);
    auto structLiteral = static_cast<StructLiteralExpr*>(expr.get());
    
    assert(structLiteral->getBase()->toString() == "Point");
    assert(structLiteral->getFields().size() == 2);
    assert(structLiteral->getFields()[0].name == "x");
    assert(structLiteral->getFields()[1].name == "y");
    
    std::cout << "testParseStructLiteral passed!" << std::endl;
}

int main() {
    testParseStructLiteral();
    return 0;
}
