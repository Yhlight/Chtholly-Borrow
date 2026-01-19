#ifndef CHTHOLLY_MIRBUILDER_H
#define CHTHOLLY_MIRBUILDER_H

#include "AST/ASTNode.h"
#include "AST/Declarations.h"
#include "AST/Expressions.h"
#include "AST/Statements.h"
#include "MIR/MIR.h"
#include <map>
#include <set>

namespace chtholly {

class MIRBuilder {
public:
    MIRBuilder(MIRModule& module);

    void lower(ASTNode* node);
    
    // Expression lowering returns the name of the temporary variable holding the result
    std::string lowerExpr(Expr* expr);

    void addModuleName(const std::string& name) { moduleNames.insert(name); }

private:
    void lowerVarDecl(VarDecl* decl);
    void lowerStructDecl(StructDecl* decl);
    void lowerEnumDecl(EnumDecl* decl);
    void lowerClassDecl(ClassDecl* decl);
    void lowerBlock(Block* block, bool shouldPushScope = true);
    void lowerFunctionDecl(FunctionDecl* decl);
    void lowerMethodDecl(MethodDecl* decl, const std::string& className);
    void lowerConstructorDecl(ConstructorDecl* decl, const std::string& className);
    void lowerReturnStmt(ReturnStmt* stmt);
    void lowerIfStmt(IfStmt* stmt);
    void lowerWhileStmt(WhileStmt* stmt);
    void lowerDoWhileStmt(DoWhileStmt* stmt);
    void lowerForStmt(ForStmt* stmt);
    void lowerSwitchStmt(SwitchStmt* stmt);
    void lowerBreakStmt(BreakStmt* stmt);
    void lowerContinueStmt(ContinueStmt* stmt);

    std::string lowerAddr(Expr* expr);
    std::string lowerLiteralExpr(LiteralExpr* expr);
    std::string lowerBinaryExpr(BinaryExpr* expr);
    std::string lowerUnaryExpr(UnaryExpr* expr);
    std::string lowerIdentifierExpr(IdentifierExpr* id);
    std::string lowerCallExpr(CallExpr* call);
    std::string lowerMemberAccessExpr(MemberAccessExpr* expr);
    std::string lowerStructLiteralExpr(StructLiteralExpr* expr);
    std::string lowerArrayLiteralExpr(ArrayLiteralExpr* expr);
    std::string lowerIndexingExpr(IndexingExpr* expr);
    std::string lowerAddressOfExpr(AddressOfExpr* expr);
    std::string lowerDereferenceExpr(DereferenceExpr* expr);
    std::string lowerQuestionExpr(QuestionExpr* expr);
    std::string lowerIntrinsicExpr(IntrinsicExpr* expr);

    std::string newTemp();
    BasicBlock* newBlock(std::string name);

    void pushScope();
    void popScope();
    void emitAllDestructors();

    MIRModule& module;
    MIRFunction* currentFunction = nullptr;
    BasicBlock* currentBlock = nullptr;
    int tempCount = 0;
    int blockCount = 0;
    
    struct LocalVar {
        std::string name;
        std::shared_ptr<Type> type;
    };
    struct ShadowedVar {
        std::string name;
        std::string oldMirName;
        std::string oldPtrType;
    };
    struct Scope {
        std::vector<LocalVar> variables;
        std::vector<ShadowedVar> shadowed;
    };
    std::vector<Scope> scopeStack;

    struct LoopContext {
        std::string breakLabel;
        std::string continueLabel;
    };
    std::vector<LoopContext> loopStack;

    // Map variable names to their alloca names in MIR
    std::map<std::string, std::string> varMap;
    // Map pointer names to their struct type names
    std::map<std::string, std::string> ptrTypeMap;

    struct StructInfo {
        std::string name;
        std::shared_ptr<Type> type;
        std::vector<std::string> fieldNames;
        bool hasDestructor = false;
    };
    std::map<std::string, StructInfo> structTypes;
    std::map<std::string, std::shared_ptr<EnumType>> enumTypes;
    std::set<std::string> moduleNames;
};

} // namespace chtholly

#endif // CHTHOLLY_MIRBUILDER_H
