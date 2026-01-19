#include "Sema/SymbolTable.h"
#include <iostream>

namespace chtholly {

SymbolTable::SymbolTable() {
    pushScope(); // Global scope
}

void SymbolTable::pushScope() {
    scopes.emplace_back();
}

void SymbolTable::popScope() {
    if (scopes.size() > 1) {
        scopes.pop_back();
    }
}

bool SymbolTable::insert(const std::string& name, std::shared_ptr<Type> type, bool isMutable, bool isPublic, ASTNode* decl) {
    if (lookupCurrentScope(name)) {
        return false;
    }
    scopes.back().symbols[name] = {name, type, isMutable, false, isPublic, decl};
    return true;
}

bool SymbolTable::insertGlobal(const std::string& name, std::shared_ptr<Type> type, bool isMutable, bool isPublic, ASTNode* decl) {
    if (scopes.front().symbols.count(name)) {
        return false;
    }
    scopes.front().symbols[name] = {name, type, isMutable, false, isPublic, decl};
    return true;
}

const Symbol* SymbolTable::lookup(const std::string& name) const {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        auto found = it->symbols.find(name);
        if (found != it->symbols.end()) {
            return &found->second;
        }
    }
    return nullptr;
}

const Symbol* SymbolTable::lookupIgnoreMoved(const std::string& name) const {
    return lookup(name);
}

const Symbol* SymbolTable::lookupCurrentScope(const std::string& name) const {
    auto found = scopes.back().symbols.find(name);
    if (found != scopes.back().symbols.end()) {
        return &found->second;
    }
    return nullptr;
}

void SymbolTable::markMoved(const std::string& name) {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        auto found = it->symbols.find(name);
        if (found != it->symbols.end()) {
            found->second.isMoved = true;
            return;
        }
    }
}

bool SymbolTable::isMoved(const std::string& name) const {
    auto sym = lookup(name);
    return sym && sym->isMoved;
}

void SymbolTable::markAccessed(const std::string& name) {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        auto found = it->symbols.find(name);
        if (found != it->symbols.end()) {
            found->second.isMoved = false;
            return;
        }
    }
}

bool SymbolTable::insertType(const std::string& name, std::shared_ptr<Type> type, bool isPublic, ASTNode* decl) {
    if (scopes.back().types.count(name)) {
        return false;
    }
    scopes.back().types[name] = type;
    scopes.back().typeDecls[name] = decl;
    scopes.back().typeVisibility[name] = isPublic;
    return true;
}

bool SymbolTable::insertTypeGlobal(const std::string& name, std::shared_ptr<Type> type, bool isPublic, ASTNode* decl) {
    if (scopes.front().types.count(name)) {
        return false;
    }
    scopes.front().types[name] = type;
    scopes.front().typeDecls[name] = decl;
    scopes.front().typeVisibility[name] = isPublic;
    return true;
}

std::shared_ptr<Type> SymbolTable::lookupType(const std::string& name) const {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        auto found = it->types.find(name);
        if (found != it->types.end()) {
            return found->second;
        }
    }
    return nullptr;
}

ASTNode* SymbolTable::lookupTypeDecl(const std::string& name) const {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        auto found = it->typeDecls.find(name);
        if (found != it->typeDecls.end()) {
            return found->second;
        }
    }
    return nullptr;
}

std::unordered_map<std::string, const Symbol*> SymbolTable::getPublicSymbols() const {
    std::unordered_map<std::string, const Symbol*> result;
    for (const auto& [name, sym] : scopes.front().symbols) {
        if (sym.isPublic) {
            result[name] = &sym;
        }
    }
    return result;
}

std::unordered_map<std::string, std::shared_ptr<Type>> SymbolTable::getPublicTypes() const {
    std::unordered_map<std::string, std::shared_ptr<Type>> result;
    for (const auto& [name, type] : scopes.front().types) {
        if (scopes.front().typeVisibility.count(name) && scopes.front().typeVisibility.at(name)) {
            result[name] = type;
        }
    }
    return result;
}

} // namespace chtholly
