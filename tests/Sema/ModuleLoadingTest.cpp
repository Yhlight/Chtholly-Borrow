#include "Sema/Sema.h"
#include "Parser.h"
#include <cassert>
#include <iostream>
#include <fstream>

using namespace chtholly;

void testModuleLoading() {
    // Create lib.cns
    {
        std::ofstream out("lib.cns");
        out << "pub fn add(a: i32, b: i32): i32 { return a + b; }";
    }

    std::string source = R"(
        import "lib.cns";
        fn main(): i32 {
            return lib::add(1, 2);
        }
    )";

    Parser parser(source);
    auto nodes = parser.parseProgram();

    Sema sema;
    try {
        for (auto& node : nodes) {
            sema.analyze(node.get());
        }
        std::cout << "testModuleLoading passed!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "testModuleLoading failed with exception: " << e.what() << std::endl;
        assert(false);
    }
}

void testModuleVisibility() {
    // Create lib_vis.cns
    {
        std::ofstream out("lib_vis.cns");
        out << "pub fn public_add(a: i32, b: i32): i32 { return a + b; }\n";
        out << "fn private_add(a: i32, b: i32): i32 { return a + b; }\n";
    }

    std::string source = R"(
        import "lib_vis.cns";
        fn main(): i32 {
            let x = lib_vis::public_add(1, 2);
            return lib_vis::private_add(3, 4); // Should fail
        }
    )";

    Parser parser(source);
    auto nodes = parser.parseProgram();

    Sema sema;
    try {
        for (auto& node : nodes) {
            sema.analyze(node.get());
        }
        assert(false && "Should have failed to find private function 'private_add'");
    } catch (const std::exception& e) {
        std::cout << "Caught expected error for private access: " << e.what() << std::endl;
    }
}

void testStructVisibility() {
    // Create lib_struct.cns
    {
        std::ofstream out("lib_struct.cns");
        out << "pub struct PublicStruct { x: i32 }\n";
        out << "struct PrivateStruct { x: i32 }\n";
    }

    std::string source = R"(
        import "lib_struct.cns";
        fn main(): i32 {
            let s1 = lib_struct::PublicStruct { x: 10 };
            let s2 = lib_struct::PrivateStruct { x: 20 }; // Should fail
            return 0;
        }
    )";

    Parser parser(source);
    auto nodes = parser.parseProgram();

    Sema sema;
    try {
        for (auto& node : nodes) {
            sema.analyze(node.get());
        }
        assert(false && "Should have failed to find private struct 'PrivateStruct'");
    } catch (const std::exception& e) {
        std::cout << "Caught expected error for private struct access: " << e.what() << std::endl;
    }
}

void testUseAliasing() {
    // Create lib_use.cns
    {
        std::ofstream out("lib_use.cns");
        out << "pub fn add(a: i32, b: i32): i32 { return a + b; }\n";
    }

    std::string source = R"(
        import "lib_use.cns";
        use lib_use::add as my_add;
        fn main(): i32 {
            return my_add(1, 2);
        }
    )";

    // Note: R"()" handles backslashes fine, but I added escaping by mistake in my head.
    // Fixed string:
    source = R"(
        import "lib_use.cns";
        use lib_use::add as my_add;
        fn main(): i32 {
            return my_add(1, 2);
        }
    )";

    Parser parser(source);
    auto nodes = parser.parseProgram();

    Sema sema;
    try {
        for (auto& node : nodes) {
            sema.analyze(node.get());
        }
        std::cout << "testUseAliasing passed!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "testUseAliasing failed with exception: " << e.what() << std::endl;
        assert(false);
    }
}

int main() {
    testModuleLoading();
    testModuleVisibility();
    testStructVisibility();
    testUseAliasing();
    return 0;
}
