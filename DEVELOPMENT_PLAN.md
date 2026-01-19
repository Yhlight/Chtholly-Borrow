# Chtholly Development Plan

## Overview
Chtholly is a systems programming language aiming for C++ performance with Rust memory safety. This document outlines the roadmap to achieve these goals, leveraging C++20 and LLVM 18.

## Phase 1: Environment & Infrastructure (Completed/Ongoing)
- [x] **Compiler Infrastructure:** Migrate to C++20 and LLVM 18.
- [x] **Cross-Platform Support:** Ensure build and execution on both Windows and Linux.
- [ ] **MIR (Mid-Level IR) Refinement:** Ensure MIR captures all necessary semantics for ownership analysis before lowering to LLVM IR.

## Phase 2: Core Safety Features (Priority: High)
The core value proposition of Chtholly is "Safety by Default".
- [ ] **Borrow Checker (Sema):** Implement ownership and borrowing analysis in the Semantic Analysis phase.
    - Track variable lifetimes.
    - Enforce "one mutable reference OR multiple immutable references" rule.
    - Prevent use-after-move.
- [ ] **Lifecycle Elision:** Implement intelligent lifecycle deduction to minimize manual annotations.

## Phase 3: Type System & Generics
- [ ] **Generics Implementation:**
    - Support `fn add<T>(a: T, b: T)` syntax.
    - Implement monomorphization (generating specialized code for each type used) in the MIR generation phase.
- [ ] **Constraints System:**
    - Implement `require` and `request` keywords for interfaces/traits.
    - Enforce constraints at compile time.

## Phase 4: Pattern Matching & Control Flow
- [ ] **Switch-Case Pattern Matching:**
    - Implement destructuring for Enums and Structs in `switch` statements.
    - Ensure exhaustiveness checking for Enums.

## Phase 5: Standard Library (StdLib)
- [ ] **Core Modules:**
    - `std::string` (UTF-8 dynamic string).
    - `std::vector` (Dynamic array).
    - `std::io` (Input/Output).
- [ ] **FFI:** Refine Foreign Function Interface for easy C/C++ interoperability.

## Phase 6: Tooling
- [ ] **Package Manager:** Standardize module resolution and package management.
- [ ] **Language Server:** Provide IDE support (completion, diagnostics).

## Immediate Next Steps
1.  Verify Linux build with LLVM 18.
2.  Start implementation of Borrow Checker in `Sema`.
3.  Add unit tests for ownership rules.
