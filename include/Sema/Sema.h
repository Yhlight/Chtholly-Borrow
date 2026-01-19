#ifndef CHTHOLLY_SEMA_H
#define CHTHOLLY_SEMA_H

#include "AST/ASTNode.h"
#include "AST/Declarations.h"
#include "AST/Expressions.h"
#include "AST/ImportDecl.h"
#include "Sema/SymbolTable.h"
#include "Sema/Substituter.h"
#include <set>

namespace chtholly {

class Sema {
public:
    Sema();
    void analyze(ASTNode* node);
    std::shared_ptr<Type> checkExpr(Expr* expr);
    
    SymbolTable& getSymbolTable() { return symbolTable; }
    std::vector<std::unique_ptr<ASTNode>>& getAnalyzedNodes() { return analyzedNodes; }
    const std::unordered_map<std::string, std::shared_ptr<SymbolTable>>& getModules() const { return modules; }

    // Monomorphization
    std::string mangleGenericName(const std::string& baseName, const std::vector<std::shared_ptr<Type>>& typeArgs);
    FunctionDecl* monomorphizeFunction(FunctionDecl* decl, const std::vector<std::shared_ptr<Type>>& typeArgs);
    FunctionDecl* monomorphizeMethod(ClassDecl* clDecl, MethodDecl* method, const std::vector<std::shared_ptr<Type>>& typeArgs);
    std::shared_ptr<StructType> monomorphizeStruct(StructDecl* decl, const std::vector<std::shared_ptr<Type>>& typeArgs);
    std::shared_ptr<StructType> monomorphizeClass(ClassDecl* decl, const std::vector<std::shared_ptr<Type>>& typeArgs);
    std::shared_ptr<EnumType> monomorphizeEnum(EnumDecl* decl, const std::vector<std::shared_ptr<Type>>& typeArgs);

private:
    std::shared_ptr<Type> resolveType(std::shared_ptr<Type> type);
    std::shared_ptr<Type> checkType(ASTNode* node);
    
    void analyzeVarDecl(VarDecl* decl);
    void analyzeImportDecl(ImportDecl* decl);
    void analyzePackageDecl(PackageDecl* decl);
    void analyzeUseDecl(UseDecl* decl);
    void analyzeRequestDecl(RequestDecl* decl);
    void analyzeStructDecl(StructDecl* decl);
    void analyzeEnumDecl(EnumDecl* decl);
    void analyzeClassDecl(ClassDecl* decl);
    void analyzeMethodDecl(MethodDecl* decl);
    void analyzeConstructorDecl(ConstructorDecl* decl);
    void analyzeBlock(Block* block);
    void analyzeFunctionDecl(FunctionDecl* decl);
    void analyzeIfStmt(IfStmt* stmt);
    void analyzeWhileStmt(WhileStmt* stmt);
    void analyzeDoWhileStmt(DoWhileStmt* stmt);
    void analyzeForStmt(ForStmt* stmt);
    void analyzeSwitchStmt(SwitchStmt* stmt);
    void analyzeReturnStmt(ReturnStmt* stmt);
    void analyzeBreakStmt(BreakStmt* stmt);
    void analyzeContinueStmt(ContinueStmt* stmt);
    void analyzeExprStmt(ExprStmt* stmt);
    void analyzePattern(Pattern* pattern, std::shared_ptr<Type> matchType);

    std::shared_ptr<Type> checkBinaryExpr(BinaryExpr* expr);
    std::shared_ptr<Type> checkUnaryExpr(UnaryExpr* expr);
    std::shared_ptr<Type> checkCallExpr(CallExpr* expr);
    std::shared_ptr<Type> checkMemberAccess(MemberAccessExpr* expr);
    std::shared_ptr<Type> checkStructLiteral(StructLiteralExpr* expr);
    std::shared_ptr<Type> checkArrayLiteral(ArrayLiteralExpr* expr);
    std::shared_ptr<Type> checkIndexing(IndexingExpr* expr);
    std::shared_ptr<Type> checkAddressOf(AddressOfExpr* expr);
    std::shared_ptr<Type> checkDereference(DereferenceExpr* expr);
    std::shared_ptr<Type> checkIntrinsic(IntrinsicExpr* expr);
    std::shared_ptr<Type> checkLiteralExpr(LiteralExpr* expr);
    std::shared_ptr<Type> checkIdentifierExpr(IdentifierExpr* expr);
    std::shared_ptr<Type> checkQuestionExpr(QuestionExpr* expr);
    std::shared_ptr<Type> checkSpecializationExpr(SpecializationExpr* expr);

    void checkConstraint(const std::string& requestName, std::shared_ptr<Type> type);

    SymbolTable symbolTable;
    std::vector<std::shared_ptr<EnumType>> registeredEnums;
    std::set<std::string> loadedModules;
    std::unordered_map<std::string, std::shared_ptr<SymbolTable>> modules;
    std::vector<std::unique_ptr<ASTNode>> analyzedNodes;

    // Monomorphization Cache (Ownership in analyzedNodes)
    std::unordered_map<std::string, FunctionDecl*> m_monomorphizedFunctions;
    std::unordered_map<std::string, StructDecl*> m_monomorphizedStructs;
    std::unordered_map<std::string, ClassDecl*> m_monomorphizedClasses;
    std::unordered_map<std::string, EnumDecl*> m_monomorphizedEnums;

    const FunctionDecl* currentFunction = nullptr;
    const MethodDecl* currentMethod = nullptr;
    std::shared_ptr<StructType> currentClass = nullptr;
    int loopDepth = 0;
    int switchDepth = 0;
};

} // namespace chtholly

#endif // CHTHOLLY_SEMA_H
