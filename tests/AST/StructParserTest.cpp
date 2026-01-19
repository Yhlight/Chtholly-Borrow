#include "Parser.h"
#include "AST/Declarations.h"
#include <cassert>
#include <iostream>

void testParseStructDecl() {
    using namespace chtholly;
    
    std::string source = "struct Point { let x: i32; let y: i32; }";
    Parser parser(source);
    
    // We need to update Parser::parseStatement to handle Struct
    auto stmt = parser.parseStatement();
    assert(stmt->getKind() == ASTNodeKind::StructDecl);
    auto structDecl = static_cast<StructDecl*>(stmt.get());
    
    assert(structDecl->getName() == "Point");
    assert(structDecl->getMembers().size() == 2);
    assert(structDecl->getMembers()[0]->getName() == "x");
    assert(structDecl->getMembers()[1]->getName() == "y");
    
    std::cout << "testParseStructDecl passed!" << std::endl;
}

int main() {
    testParseStructDecl();
    return 0;
}
