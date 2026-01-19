#include "Sema/Sema.h"
#include "Parser.h"
#include <cassert>
#include <iostream>

using namespace chtholly;

void testMonomorphizeFunction() {
    std::string source = "fn add[T](a: T, b: T): T { return a + b; } \n" 
                         "fn main() { let x = add[i32](1, 2); }";
    Parser parser(source);
    auto program = parser.parseProgram();
    
    Sema sema;
    for (const auto& node : program) {
        sema.analyze(node.get());
    }
    
    // Check if add_i32 was created
    auto sym = sema.getSymbolTable().lookup("add_i32");
    assert(sym != nullptr);
    assert(sym->type->isFunction());
    auto funcTy = std::static_pointer_cast<FunctionType>(sym->type);
    assert(funcTy->getParamTypes()[0]->isI32());
    assert(funcTy->getReturnType()->isI32());
    
    std::cout << "testMonomorphizeFunction passed!" << std::endl;
}

void testMonomorphizeStruct() {
    std::string source = "struct Point[T] { let x: T, let y: T } \n" 
                         "fn main() { let p = Point[f64] { x: 1.0, y: 2.0 }; }";
    Parser parser(source);
    auto program = parser.parseProgram();
    
    Sema sema;
    for (const auto& node : program) {
        sema.analyze(node.get());
    }
    
    // Check if Point_f64 was created
    auto type = sema.getSymbolTable().lookupType("Point_f64");
    assert(type != nullptr);
    assert(type->isStruct());
    auto stTy = std::static_pointer_cast<StructType>(type);
    assert(stTy->getFields().size() == 2);
    assert(stTy->getFields()[0].type->isF64());
    
    std::cout << "testMonomorphizeStruct passed!" << std::endl;
}

void testNestedGenerics() {
    std::string source = "struct Wrapper[T] { let value: T } \n"
                         "struct Point[T] { let x: T, let y: T } \n"
                         "fn main() { let w = Wrapper[Point[i32]] { value: Point[i32] { x: 1, y: 2 } }; }";
    Parser parser(source);
    auto program = parser.parseProgram();
    
    Sema sema;
    for (const auto& node : program) {
        sema.analyze(node.get());
    }
    
    // Check if Wrapper_Point_i32_ was created
    auto type = sema.getSymbolTable().lookupType("Wrapper_Point_i32_");
    assert(type != nullptr);
    assert(type->isStruct());
    auto stTy = std::static_pointer_cast<StructType>(type);
    assert(stTy->getFields()[0].type->isStruct());
    assert(stTy->getFields()[0].type->getName() == "Point_i32");
    
    std::cout << "testNestedGenerics passed!" << std::endl;
}

void testGenericClass() {
    std::string source = "class Container[T] { pub let value: T; pub fn get(&self): T { return self.value; } } \n"
                         "fn main() { let c = Container[i32] { value: 42 }; let x = c.get(); }";
    Parser parser(source);
    auto program = parser.parseProgram();
    
    Sema sema;
    for (const auto& node : program) {
        sema.analyze(node.get());
    }
    
    // Check if Container_i32 was created
    auto type = sema.getSymbolTable().lookupType("Container_i32");
    assert(type != nullptr);
    assert(type->isClass());
    auto stTy = std::static_pointer_cast<StructType>(type);
    assert(stTy->getFields().size() == 1);
    assert(stTy->getFields()[0].type->isI32());
    assert(stTy->getMethods().size() == 1);
    
    std::cout << "testGenericClass passed!" << std::endl;
}

void testGenericMethod() {
    std::string source = "class Helper { pub fn identity[T](&self, val: T): T { return val; } } \n"
                         "fn main() { let h = Helper {}; let x = h.identity[i32](42); }";
    Parser parser(source);
    auto program = parser.parseProgram();
    
    Sema sema;
    for (const auto& node : program) {
        sema.analyze(node.get());
    }
    
    // Check if Helper_identity_i32 was created
    auto sym = sema.getSymbolTable().lookup("Helper_identity_i32");
    assert(sym != nullptr);
    assert(sym->type->isFunction());
    auto funcTy = std::static_pointer_cast<FunctionType>(sym->type);
    assert(funcTy->getParamTypes()[0]->isPointer()); // &self is a pointer/ref
    
    std::cout << "testGenericMethod passed!" << std::endl;
}

void testGenericMethodInGenericClass() {
    std::string source = "class Container[T] { pub fn combine[U](&self, a: T, b: U): i32 { return 0; } } \n"
                         "fn main() { let c = Container[i32] { }; let x = c.combine[f64](1, 2.0); }";
    Parser parser(source);
    auto program = parser.parseProgram();
    
    Sema sema;
    for (const auto& node : program) {
        sema.analyze(node.get());
    }
    
    // Check if Container_i32_combine_f64 was created
    auto sym = sema.getSymbolTable().lookup("Container_i32_combine_f64");
    assert(sym != nullptr);
    assert(sym->type->isFunction());
    auto funcTy = std::static_pointer_cast<FunctionType>(sym->type);
    assert(funcTy->getParamTypes()[1]->isI32());
    assert(funcTy->getParamTypes()[2]->isF64());
    
    std::cout << "testGenericMethodInGenericClass passed!" << std::endl;
}

int main() {
    try {
        testMonomorphizeFunction();
        testMonomorphizeStruct();
        testNestedGenerics();
        testGenericClass();
        testGenericMethod();
        testGenericMethodInGenericClass();
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
