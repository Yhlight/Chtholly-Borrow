#include "Parser.h"
#include "Sema/Sema.h"
#include "MIR/MIRBuilder.h"
#include "Backend/CodeGenerator.h"
#include <iostream>
#include <filesystem>

using namespace chtholly;

void testGenericsEndToEnd() {
    std::string source = R"(
extern fn printf(fmt: i8*, ...): i32;
fn add[T](a: T, b: T): T { return a + b; }
struct Point[T] { let x: T; let y: T }
fn main(): i32 {
  let sum = add[i32](10, 32);
  let p = Point[f64] { x: 1.1, y: 2.2 };
  printf("Sum: %d, Point: (%f, %f)\n", sum, p.x, p.y);
  return 0;
}
)";
    
    Parser parser(source);
    auto program = parser.parseProgram();
    
    Sema sema;
    for (const auto& node : program) {
        sema.analyze(node.get());
    }
    
    MIRModule module;
    MIRBuilder mirBuilder(module);
    
    // Lower monomorphized nodes first
    for (const auto& node : sema.getAnalyzedNodes()) {
        mirBuilder.lower(node.get());
    }
    for (const auto& node : program) {
        mirBuilder.lower(node.get());
    }
    
    CodeGenerator codegen(module);
    codegen.generate();
    codegen.emitObjectFile("generics_test.obj");
    
    if (!std::filesystem::exists("generics_test.obj")) {
        throw std::runtime_error("Failed to generate generics_test.obj");
    }
    std::cout << "testGenericsEndToEnd passed!" << std::endl;
}

void testGenericMethodsEndToEnd() {
    std::string source = R"(
extern fn printf(fmt: i8*, ...): i32;
class Container[T] {
    pub let mut value: T;
    pub fn set(&mut self, v: T) { self.value = v; }
    pub fn get(&self): T { return self.value; }
}
fn main(): i32 {
    let mut c = Container[i32] { value: 0 };
    c.set(42);
    printf("Value: %d\n", c.get());
    return 0;
}
)";
    
    Parser parser(source);
    auto program = parser.parseProgram();
    
    Sema sema;
    for (const auto& node : program) {
        std::cout << "Sema analyzing: " << node->getName() << std::endl;
        sema.analyze(node.get());
    }
    std::cout << "Sema done." << std::endl;
    
    MIRModule module;
    MIRBuilder mirBuilder(module);
    
    std::cout << "MIR Lowering monomorphized..." << std::endl;
    for (const auto& node : sema.getAnalyzedNodes()) {
        mirBuilder.lower(node.get());
    }
    std::cout << "MIR Lowering program..." << std::endl;
    for (const auto& node : program) {
        mirBuilder.lower(node.get());
    }
    std::cout << "MIR done." << std::endl;
    
    try {
        CodeGenerator codegen(module);
        std::cout << "Codegen generate..." << std::endl;
        codegen.generate();
        std::cout << "Codegen emit..." << std::endl;
        codegen.emitObjectFile("methods_test.obj");
    } catch (const std::exception& e) {
        std::cerr << "testGenericMethodsEndToEnd Codegen Exception: " << e.what() << std::endl;
        throw;
    }
    
    if (!std::filesystem::exists("methods_test.obj")) {
        throw std::runtime_error("Failed to generate methods_test.obj");
    }
    std::cout << "testGenericMethodsEndToEnd passed!" << std::endl;
}

int main() {
    try {
        testGenericsEndToEnd();
        testGenericMethodsEndToEnd();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
}