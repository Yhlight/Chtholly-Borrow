#ifndef CHTHOLLY_SYMBOLTABLE_H
#define CHTHOLLY_SYMBOLTABLE_H

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include "AST/Types.h"

namespace chtholly {

class ASTNode;

struct Symbol {
    std::string name;
    std::shared_ptr<Type> type;
    bool isMutable;
    bool isMoved = false;
    bool isPublic = false;
    ASTNode* decl = nullptr;
};

class SymbolTable {
public:
    SymbolTable();

    void pushScope();
    void popScope();

    bool insert(const std::string& name, std::shared_ptr<Type> type, bool isMutable, bool isPublic = false, ASTNode* decl = nullptr);
    bool insertGlobal(const std::string& name, std::shared_ptr<Type> type, bool isMutable, bool isPublic = false, ASTNode* decl = nullptr);
    const Symbol* lookup(const std::string& name) const;
    const Symbol* lookupCurrentScope(const std::string& name) const;
    const Symbol* lookupIgnoreMoved(const std::string& name) const;

    void markMoved(const std::string& name);
    bool isMoved(const std::string& name) const;
    void markAccessed(const std::string& name); // Mark as not moved (e.g. after re-assignment)

    bool insertType(const std::string& name, std::shared_ptr<Type> type, bool isPublic = false, ASTNode* decl = nullptr);
    bool insertTypeGlobal(const std::string& name, std::shared_ptr<Type> type, bool isPublic = false, ASTNode* decl = nullptr);
    std::shared_ptr<Type> lookupType(const std::string& name) const;
    ASTNode* lookupTypeDecl(const std::string& name) const;

    std::unordered_map<std::string, const Symbol*> getPublicSymbols() const;
    std::unordered_map<std::string, std::shared_ptr<Type>> getPublicTypes() const;

private:
    struct Scope {
        std::unordered_map<std::string, Symbol> symbols;
        std::unordered_map<std::string, std::shared_ptr<Type>> types;
        std::unordered_map<std::string, ASTNode*> typeDecls;
        std::unordered_map<std::string, bool> typeVisibility; // Track if type is public
    };
    std::vector<Scope> scopes;
};

} // namespace chtholly

#endif // CHTHOLLY_SYMBOLTABLE_H
