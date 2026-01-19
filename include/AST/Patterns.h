#ifndef CHTHOLLY_PATTERNS_H
#define CHTHOLLY_PATTERNS_H

#include "AST/ASTNode.h"
#include "AST/Expressions.h"
#include <vector>
#include <memory>

namespace chtholly {

enum class PatternKind {
    Literal,
    Identifier,
    Variant,
    Wildcard
};

class Pattern : public ASTNode {
public:
    virtual ~Pattern() = default;
    virtual PatternKind getPatternKind() const = 0;
    virtual std::unique_ptr<Pattern> clonePattern() const = 0;
    std::unique_ptr<ASTNode> clone() const override { return clonePattern(); }
};

class LiteralPattern : public Pattern {
public:
    LiteralPattern(std::unique_ptr<LiteralExpr> literal) : literal(std::move(literal)) {}
    ASTNodeKind getKind() const override { return ASTNodeKind::LiteralPattern; }
    PatternKind getPatternKind() const override { return PatternKind::Literal; }
    std::unique_ptr<Pattern> clonePattern() const override {
        return std::make_unique<LiteralPattern>(std::unique_ptr<LiteralExpr>(static_cast<LiteralExpr*>(literal->cloneExpr().release())));
    }
    const LiteralExpr* getLiteral() const { return literal.get(); }
    std::string toString() const override { return literal->toString(); }

private:
    std::unique_ptr<LiteralExpr> literal;
};

class IdentifierPattern : public Pattern {
public:
    IdentifierPattern(std::string name) : name(name) {}
    ASTNodeKind getKind() const override { return ASTNodeKind::IdentifierPattern; }
    PatternKind getPatternKind() const override { return PatternKind::Identifier; }
    std::unique_ptr<Pattern> clonePattern() const override {
        return std::make_unique<IdentifierPattern>(name);
    }
    const std::string& getName() const { return name; }
    std::string toString() const override { return name; }

private:
    std::string name;
};

class VariantPattern : public Pattern {
public:
    VariantPattern(std::string enumName, std::string variantName, std::vector<std::unique_ptr<Pattern>> subPatterns)
        : enumName(enumName), variantName(variantName), subPatterns(std::move(subPatterns)) {}
    
    ASTNodeKind getKind() const override { return ASTNodeKind::VariantPattern; }
    PatternKind getPatternKind() const override { return PatternKind::Variant; }
    std::unique_ptr<Pattern> clonePattern() const override {
        std::vector<std::unique_ptr<Pattern>> newSubs;
        for (const auto& p : subPatterns) newSubs.push_back(p->clonePattern());
        return std::make_unique<VariantPattern>(enumName, variantName, std::move(newSubs));
    }
    const std::string& getEnumName() const { return enumName; }
    const std::string& getVariantName() const { return variantName; }
    const std::vector<std::unique_ptr<Pattern>>& getSubPatterns() const { return subPatterns; }
    
    std::string toString() const override {
        std::string res = enumName + "::" + variantName;
        if (!subPatterns.empty()) {
            res += "(";
            for (size_t i = 0; i < subPatterns.size(); ++i) {
                res += subPatterns[i]->toString();
                if (i < subPatterns.size() - 1) res += ", ";
            }
            res += ")";
        }
        return res;
    }

private:
    std::string enumName;
    std::string variantName;
    std::vector<std::unique_ptr<Pattern>> subPatterns;
};

class WildcardPattern : public Pattern {
public:
    ASTNodeKind getKind() const override { return ASTNodeKind::WildcardPattern; }
    PatternKind getPatternKind() const override { return PatternKind::Wildcard; }
    std::unique_ptr<Pattern> clonePattern() const override { return std::make_unique<WildcardPattern>(); }
    std::string toString() const override { return "_"; }
};

} // namespace chtholly

#endif // CHTHOLLY_PATTERNS_H
