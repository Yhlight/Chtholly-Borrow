#include "Parser.h"
#include "AST/ImportDecl.h"
#include <cassert>
#include <iostream>

using namespace chtholly;

void testParsePackageImportUse() {
    std::string source = R"(
        package std;
        import "io.cns";
        import std::vec;
        use std::io::println;
        use std::io::print as pl;
    )";

    Parser parser(source);
    auto nodes = parser.parseProgram();

    // We expect 5 nodes: PackageDecl, ImportDecl, ImportDecl, UseDecl, UseDecl
    assert(nodes.size() == 5);

    // 1. PackageDecl
    assert(nodes[0]->getKind() == ASTNodeKind::PackageDecl);
    auto pkg = static_cast<PackageDecl*>(nodes[0].get());
    assert(pkg->packageName == "std");

    // 2. ImportDecl (File)
    assert(nodes[1]->getKind() == ASTNodeKind::ImportDecl);
    auto imp1 = static_cast<ImportDecl*>(nodes[1].get());
    assert(imp1->path == "io.cns");
    assert(imp1->isStd == false);

    // 3. ImportDecl (Std)
    assert(nodes[2]->getKind() == ASTNodeKind::ImportDecl);
    auto imp2 = static_cast<ImportDecl*>(nodes[2].get());
    assert(imp2->path == "std::vec"); // Or however we decide to store it
    assert(imp2->isStd == true);

    // 4. UseDecl
    assert(nodes[3]->getKind() == ASTNodeKind::UseDecl);
    auto use1 = static_cast<UseDecl*>(nodes[3].get());
    assert(use1->path == "std::io::println");
    assert(use1->alias == "");

    // 5. UseDecl with alias
    assert(nodes[4]->getKind() == ASTNodeKind::UseDecl);
    auto use2 = static_cast<UseDecl*>(nodes[4].get());
    assert(use2->path == "std::io::print");
    assert(use2->alias == "pl");

    std::cout << "testParsePackageImportUse passed!" << std::endl;
}

int main() {
    try {
        testParsePackageImportUse();
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
