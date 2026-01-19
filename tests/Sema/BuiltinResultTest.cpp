#include "Sema/Sema.h"
#include "Parser.h"
#include <cassert>
#include <iostream>

using namespace chtholly;

void testResultImplicitlyDefined() {
    std::string source = "fn test(): Result[i32, bool] { return Result[i32, bool]::Ok(42); }";
    Parser parser(source);
    auto program = parser.parseProgram();

    if (program.empty()) {
        std::cerr << "Parser failed to produce any nodes for test source." << std::endl;
        assert(false);
    }

    Sema sema;
    try {
        for (auto const& node : program) {
            sema.analyze(node.get());
        }
        std::cout << "Result implicit definition test PASSED" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Result implicit definition test FAILED with exception: " << e.what() << std::endl;
        assert(false);
    }

    auto resultType = sema.getSymbolTable().lookupType("Result_i32_bool");
    if (resultType == nullptr) {
        std::cerr << "Result_i32_bool NOT found in symbol table." << std::endl;
        assert(false);
    }
    assert(resultType->isStruct());
    std::cout << "Result_i32_bool found in symbol table." << std::endl;
}

int main() {
    testResultImplicitlyDefined();
    return 0;
}