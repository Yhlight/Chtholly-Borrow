#ifndef CHTHOLLY_SUBSTITUTER_H
#define CHTHOLLY_SUBSTITUTER_H

#include "AST/Declarations.h"
#include "AST/Expressions.h"
#include "AST/Statements.h"
#include "AST/Patterns.h"
#include <map>
#include <iostream>

namespace chtholly
{

    class ASTSubstituter
    {
    public:
        ASTSubstituter(const std::map<std::string, std::shared_ptr<Type>> &mapping)
            : m_mapping(mapping) {}

        std::unique_ptr<ASTNode> substitute(const ASTNode *node)
        {
            if (!node)
                return nullptr;
            auto cloned = node->clone();
            replaceTypes(cloned.get());
            return cloned;
        }

        std::unique_ptr<Expr> substituteExpr(const Expr *expr)
        {
            if (!expr)
                return nullptr;
            auto cloned = expr->cloneExpr();
            replaceTypes(cloned.get());
            return cloned;
        }

        std::unique_ptr<Stmt> substituteStmt(const Stmt *stmt)
        {
            if (!stmt)
                return nullptr;
            auto cloned = stmt->cloneStmt();
            replaceTypes(cloned.get());
            return cloned;
        }

        std::unique_ptr<Decl> substituteDecl(const Decl *decl)
        {
            if (!decl)
                return nullptr;
            auto cloned = decl->cloneDecl();
            replaceTypes(cloned.get());
            return cloned;
        }

    private:
        void replaceTypes(ASTNode *node)
        {
            if (!node)
                return;

            switch (node->getKind()) {
                case ASTNodeKind::LiteralExpr:
                case ASTNodeKind::IdentifierExpr:
                case ASTNodeKind::BinaryExpr:
                case ASTNodeKind::CallExpr:
                case ASTNodeKind::MemberAccessExpr:
                case ASTNodeKind::StructLiteralExpr:
                case ASTNodeKind::ArrayLiteralExpr:
                case ASTNodeKind::IndexingExpr:
                case ASTNodeKind::UnaryExpr:
                case ASTNodeKind::AddressOfExpr:
                case ASTNodeKind::DereferenceExpr:
                case ASTNodeKind::SpecializationExpr:
                case ASTNodeKind::IntrinsicExpr:
                case ASTNodeKind::QuestionExpr: {
                    auto expr = static_cast<Expr*>(node);
                    if (expr->getType()) {
                        expr->setType(expr->getType()->substitute(m_mapping));
                    }
                    if (node->getKind() == ASTNodeKind::BinaryExpr) {
                        auto binary = static_cast<BinaryExpr*>(node);
                        replaceTypes(const_cast<Expr*>(binary->getLeft()));
                        replaceTypes(const_cast<Expr*>(binary->getRight()));
                    } else if (node->getKind() == ASTNodeKind::UnaryExpr) {
                        replaceTypes(const_cast<Expr*>(static_cast<UnaryExpr*>(node)->getOperand()));
                    } else if (node->getKind() == ASTNodeKind::CallExpr) {
                        auto call = static_cast<CallExpr*>(node);
                        replaceTypes(const_cast<Expr*>(call->getCallee()));
                        for (const auto& arg : call->getArgs()) replaceTypes(arg.get());
                    } else if (node->getKind() == ASTNodeKind::MemberAccessExpr) {
                        replaceTypes(const_cast<Expr*>(static_cast<MemberAccessExpr*>(node)->getBase()));
                    } else if (node->getKind() == ASTNodeKind::StructLiteralExpr) {
                        auto structLit = static_cast<StructLiteralExpr*>(node);
                        replaceTypes(const_cast<Expr*>(structLit->getBase()));
                        for (const auto& f : structLit->getFields()) replaceTypes(f.value.get());
                    } else if (node->getKind() == ASTNodeKind::ArrayLiteralExpr) {
                        for (const auto& e : static_cast<ArrayLiteralExpr*>(node)->getElements()) replaceTypes(e.get());
                    } else if (node->getKind() == ASTNodeKind::IndexingExpr) {
                        auto index = static_cast<IndexingExpr*>(node);
                        replaceTypes(const_cast<Expr*>(index->getBase()));
                        replaceTypes(const_cast<Expr*>(index->getIndex()));
                    } else if (node->getKind() == ASTNodeKind::AddressOfExpr) {
                        replaceTypes(const_cast<Expr*>(static_cast<AddressOfExpr*>(node)->getOperand()));
                    } else if (node->getKind() == ASTNodeKind::DereferenceExpr) {
                        replaceTypes(const_cast<Expr*>(static_cast<DereferenceExpr*>(node)->getOperand()));
                    } else if (node->getKind() == ASTNodeKind::QuestionExpr) {
                        replaceTypes(const_cast<Expr*>(static_cast<QuestionExpr*>(node)->getOperand()));
                    } else if (node->getKind() == ASTNodeKind::SpecializationExpr) {
                        auto spec = static_cast<SpecializationExpr*>(node);
                        replaceTypes(const_cast<Expr*>(spec->getBase()));
                        std::vector<std::shared_ptr<Type>> newArgs;
                        for (const auto& t : spec->getTypeArgs()) newArgs.push_back(t->substitute(m_mapping));
                        spec->setTypeArgs(std::move(newArgs));
                    } else if (node->getKind() == ASTNodeKind::IntrinsicExpr) {
                        auto intrinsic = static_cast<IntrinsicExpr*>(node);
                        if (intrinsic->getTypeArg()) intrinsic->setTypeArg(intrinsic->getTypeArg()->substitute(m_mapping));
                        for (const auto& arg : intrinsic->getArgs()) replaceTypes(arg.get());
                    }
                    break;
                }
                case ASTNodeKind::Block: {
                    for (const auto& s : static_cast<Block*>(node)->getStatements()) replaceTypes(s.get());
                    break;
                }
                case ASTNodeKind::IfStmt: {
                    auto ifStmt = static_cast<IfStmt*>(node);
                    replaceTypes(const_cast<Expr*>(ifStmt->getCondition()));
                    replaceTypes(const_cast<Block*>(ifStmt->getThenBlock()));
                    if (ifStmt->getElseBlock()) replaceTypes(const_cast<Block*>(ifStmt->getElseBlock()));
                    break;
                }
                case ASTNodeKind::WhileStmt: {
                    auto whileStmt = static_cast<WhileStmt*>(node);
                    replaceTypes(const_cast<Expr*>(whileStmt->getCondition()));
                    replaceTypes(const_cast<Block*>(whileStmt->getBody()));
                    break;
                }
                case ASTNodeKind::DoWhileStmt: {
                    auto doWhile = static_cast<DoWhileStmt*>(node);
                    replaceTypes(const_cast<Block*>(doWhile->getBody()));
                    replaceTypes(const_cast<Expr*>(doWhile->getCondition()));
                    break;
                }
                case ASTNodeKind::ForStmt: {
                    auto forStmt = static_cast<ForStmt*>(node);
                    if (forStmt->getInit()) replaceTypes(const_cast<Stmt*>(forStmt->getInit()));
                    if (forStmt->getCondition()) replaceTypes(const_cast<Expr*>(forStmt->getCondition()));
                    if (forStmt->getStep()) replaceTypes(const_cast<Expr*>(forStmt->getStep()));
                    replaceTypes(const_cast<Block*>(forStmt->getBody()));
                    break;
                }
                case ASTNodeKind::ReturnStmt: {
                    auto ret = static_cast<ReturnStmt*>(node);
                    if (ret->getExpression()) replaceTypes(const_cast<Expr*>(ret->getExpression()));
                    break;
                }
                case ASTNodeKind::ExprStmt: {
                    replaceTypes(const_cast<Expr*>(static_cast<ExprStmt*>(node)->getExpression()));
                    break;
                }
                case ASTNodeKind::SwitchStmt: {
                    auto sw = static_cast<SwitchStmt*>(node);
                    replaceTypes(const_cast<Expr*>(sw->getCondition()));
                    for (const auto& c : sw->getCases()) replaceTypes(c.get());
                    break;
                }
                case ASTNodeKind::CaseStmt: {
                    auto cs = static_cast<CaseStmt*>(node);
                    if (cs->getPattern()) replaceTypes(const_cast<Pattern*>(cs->getPattern()));
                    replaceTypes(const_cast<Block*>(cs->getBody()));
                    break;
                }
                case ASTNodeKind::VarDecl: {
                    auto var = static_cast<VarDecl*>(node);
                    if (var->getType()) var->setType(var->getType()->substitute(m_mapping));
                    if (var->getInitializer()) replaceTypes(const_cast<Expr*>(var->getInitializer()));
                    break;
                }
                case ASTNodeKind::FunctionDecl: {
                    auto func = static_cast<FunctionDecl*>(node);
                    for (const auto& p : func->getParams()) p->setType(p->getType()->substitute(m_mapping));
                    func->setReturnType(func->getReturnType()->substitute(m_mapping));
                    if (func->getBody()) replaceTypes(func->getBody());
                    break;
                }
                case ASTNodeKind::MethodDecl: {
                    auto method = static_cast<MethodDecl*>(node);
                    for (const auto& p : method->getParams()) p->setType(p->getType()->substitute(m_mapping));
                    method->setReturnType(method->getReturnType()->substitute(m_mapping));
                    if (method->getBody()) replaceTypes(method->getBody());
                    break;
                }
                case ASTNodeKind::ConstructorDecl: {
                    auto ctor = static_cast<ConstructorDecl*>(node);
                    for (const auto& p : ctor->getParams()) p->setType(p->getType()->substitute(m_mapping));
                    if (ctor->getBody()) replaceTypes(ctor->getBody());
                    break;
                }
                case ASTNodeKind::StructDecl: {
                    for (const auto& m : static_cast<StructDecl*>(node)->getMembers()) replaceTypes(m.get());
                    break;
                }
                case ASTNodeKind::EnumDecl: {
                    auto en = static_cast<EnumDecl*>(node);
                    if (en->getType()) en->setType(en->getType()->substitute(m_mapping));
                    for (const auto& v : en->getVariants()) replaceTypes(v.get());
                    break;
                }
                case ASTNodeKind::ClassDecl: {
                    for (const auto& m : static_cast<ClassDecl*>(node)->getMembers()) replaceTypes(m.get());
                    break;
                }
                case ASTNodeKind::EnumVariant: {
                    auto variant = static_cast<EnumVariant*>(node);
                    std::vector<std::shared_ptr<Type>> newTupleTypes;
                    for (const auto& t : variant->getTupleTypes()) newTupleTypes.push_back(t->substitute(m_mapping));
                    variant->setTupleTypes(std::move(newTupleTypes));
                    for (const auto& m : variant->getStructFields()) replaceTypes(m.get());
                    break;
                }
                case ASTNodeKind::Param: {
                    auto p = static_cast<Param*>(node);
                    p->setType(p->getType()->substitute(m_mapping));
                    break;
                }
                case ASTNodeKind::VariantPattern: {
                    for (const auto& p : static_cast<VariantPattern*>(node)->getSubPatterns()) replaceTypes(p.get());
                    break;
                }
                case ASTNodeKind::LiteralPattern: {
                    replaceTypes(const_cast<LiteralExpr*>(static_cast<LiteralPattern*>(node)->getLiteral()));
                    break;
                }
                case ASTNodeKind::IdentifierPattern:
                case ASTNodeKind::WildcardPattern:
                    break;
                default:
                    break;
            }
        }

        const std::map<std::string, std::shared_ptr<Type>> &m_mapping;
    };

} // namespace chtholly

#endif // CHTHOLLY_SUBSTITUTER_H
