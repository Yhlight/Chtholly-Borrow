#include "AST/ImportDecl.h"
#include <cassert>
#include <iostream>

using namespace chtholly;

void testCreateImportDecl() {
    ImportDecl importNode("std/io.cns", false, "io");
    assert(importNode.path == "std/io.cns");
    assert(importNode.isStd == false);
    assert(importNode.alias == "io");
    assert(importNode.toString() == "import \"std/io.cns\" as io;");
    std::cout << "testCreateImportDecl passed!" << std::endl;
}

void testCreatePackageDecl() {
    PackageDecl packageNode("std");
    assert(packageNode.packageName == "std");
    assert(packageNode.toString() == "package std;");
    std::cout << "testCreatePackageDecl passed!" << std::endl;
}

void testCreateUseDecl() {
    UseDecl useNode("std::io::println", "print");
    assert(useNode.path == "std::io::println");
    assert(useNode.alias == "print");
    assert(useNode.toString() == "use std::io::println as print;");
    std::cout << "testCreateUseDecl passed!" << std::endl;
}

int main() {
    testCreateImportDecl();
    testCreatePackageDecl();
    testCreateUseDecl();
    std::cout << "All ImportASTTests passed!" << std::endl;
    return 0;
}