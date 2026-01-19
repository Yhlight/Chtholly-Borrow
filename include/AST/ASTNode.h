#ifndef CHTHOLLY_ASTNODE_H
#define CHTHOLLY_ASTNODE_H

#include <vector>
#include <memory>
#include <string>
#include "AST/Types.h"

namespace chtholly {

enum class ASTNodeKind {
    VarDecl,
    StructDecl,
    EnumDecl,
    ClassDecl,
    EnumVariant,
    FunctionDecl,
    MethodDecl,
    ConstructorDecl,
    Param,
    ImportDecl,
    PackageDecl,
    UseDecl,
    RequestDecl,
    Block,
    IfStmt,
    WhileStmt,
    DoWhileStmt,
    ForStmt,
    SwitchStmt,
    CaseStmt,
    BreakStmt,
    ContinueStmt,
    ReturnStmt,
    ExprStmt,
    BinaryExpr,
    UnaryExpr,
    CallExpr,
    MemberAccessExpr,
    StructLiteralExpr,
    ArrayLiteralExpr,
    IndexingExpr,
    AddressOfExpr,
    DereferenceExpr,
    IntrinsicExpr,
    QuestionExpr,
    ConstraintExpr,
    LiteralExpr,
    IdentifierExpr,
    LiteralPattern,
    IdentifierPattern,
    VariantPattern,
    WildcardPattern,
    SpecializationExpr,
};

class ASTNode {
public:
    virtual ~ASTNode() = default;
    virtual std::string toString() const = 0;
    virtual std::unique_ptr<ASTNode> clone() const = 0;
    virtual ASTNodeKind getKind() const = 0;

    virtual const std::string& getName() const { static std::string empty = ""; return empty; }
    virtual std::shared_ptr<Type> getType() const { return nullptr; }
};

} // namespace chtholly

#endif // CHTHOLLY_ASTNODE_H
