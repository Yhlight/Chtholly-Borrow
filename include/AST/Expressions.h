#ifndef CHTHOLLY_EXPRESSIONS_H
#define CHTHOLLY_EXPRESSIONS_H

#include "AST/ASTNode.h"
#include "Lexer/Token.h"
#include <variant>

namespace chtholly {

class Expr : public ASTNode {
public:
    virtual ~Expr() = default;

    void setType(std::shared_ptr<Type> type) { m_type = type; }
    std::shared_ptr<Type> getType() const { return m_type; }

    virtual std::unique_ptr<Expr> cloneExpr() const = 0;
    std::unique_ptr<ASTNode> clone() const override { return cloneExpr(); }

protected:
    std::shared_ptr<Type> m_type = nullptr;
};

class LiteralExpr : public Expr {
public:
    using Value = std::variant<int64_t, double, bool, std::string, std::nullptr_t>;

    LiteralExpr(Value value, std::shared_ptr<Type> explicitType = nullptr) 
        : value(value), explicitType(explicitType) {}

    ASTNodeKind getKind() const override { return ASTNodeKind::LiteralExpr; }

    const Value& getValue() const { return value; }
    std::shared_ptr<Type> getExplicitType() const { return explicitType; }

    std::unique_ptr<Expr> cloneExpr() const override {
        return std::make_unique<LiteralExpr>(value, explicitType);
    }

    std::string toString() const override {
        return std::visit([](auto&& arg) -> std::string {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, bool>) {
                return arg ? "true" : "false";
            } else if constexpr (std::is_same_v<T, std::string>) {
                return arg;
            } else if constexpr (std::is_same_v<T, double>) {
                return std::to_string(arg);
            } else if constexpr (std::is_same_v<T, std::nullptr_t>) {
                return "nullptr";
            } else {
                return std::to_string(arg);
            }
        }, value);
    }

private:
    Value value;
    std::shared_ptr<Type> explicitType;
};

class IdentifierExpr : public Expr {
public:
    IdentifierExpr(std::string name) : name(name) {}

    ASTNodeKind getKind() const override { return ASTNodeKind::IdentifierExpr; }

    std::unique_ptr<Expr> cloneExpr() const override {
        return std::make_unique<IdentifierExpr>(name);
    }

    std::string toString() const override {
        return name;
    }

private:
    std::string name;
};

class BinaryExpr : public Expr {
public:
    BinaryExpr(std::unique_ptr<Expr> left, TokenType op, std::unique_ptr<Expr> right)
        : left(std::move(left)), op(op), right(std::move(right)) {}

    ASTNodeKind getKind() const override { return ASTNodeKind::BinaryExpr; }

    std::unique_ptr<Expr> cloneExpr() const override {
        return std::make_unique<BinaryExpr>(left->cloneExpr(), op, right->cloneExpr());
    }

    const Expr* getLeft() const { return left.get(); }
    const Expr* getRight() const { return right.get(); }
    TokenType getOp() const { return op; }

    std::string toString() const override {
        std::string opStr;
        switch (op) {
            case TokenType::Plus: opStr = "+"; break;
            case TokenType::Minus: opStr = "-"; break;
            case TokenType::Star: opStr = "*"; break;
            case TokenType::Slash: opStr = "/"; break;
            default: opStr = "???"; break;
        }
        return "(" + left->toString() + " " + opStr + " " + right->toString() + ")";
    }

private:
    std::unique_ptr<Expr> left;
    TokenType op;
    std::unique_ptr<Expr> right;
};

class CallExpr : public Expr {
public:
    CallExpr(std::unique_ptr<Expr> callee, std::vector<std::unique_ptr<Expr>> args)
        : callee(std::move(callee)), args(std::move(args)) {}

    ASTNodeKind getKind() const override { return ASTNodeKind::CallExpr; }

    // Helper for legacy string-based construction (if needed, or just remove)
    CallExpr(std::string name, std::vector<std::unique_ptr<Expr>> args)
        : callee(std::make_unique<IdentifierExpr>(name)), args(std::move(args)) {}

    std::unique_ptr<Expr> cloneExpr() const override {
        std::vector<std::unique_ptr<Expr>> newArgs;
        for (const auto& arg : args) newArgs.push_back(arg->cloneExpr());
        return std::make_unique<CallExpr>(callee->cloneExpr(), std::move(newArgs));
    }

    const Expr* getCallee() const { return callee.get(); }
    const std::vector<std::unique_ptr<Expr>>& getArgs() const { return args; }

    std::string toString() const override {
        std::string result = callee->toString() + "(";
        for (size_t i = 0; i < args.size(); ++i) {
            result += args[i]->toString();
            if (i < args.size() - 1) {
                result += ", ";
            }
        }
        result += ")";
        return result;
    }

private:
    std::unique_ptr<Expr> callee;
    std::vector<std::unique_ptr<Expr>> args;
};

class MemberAccessExpr : public Expr {
public:
    MemberAccessExpr(std::unique_ptr<Expr> base, std::string memberName, bool isStatic = false)
        : base(std::move(base)), memberName(std::move(memberName)), m_isStatic(isStatic) {}

    ASTNodeKind getKind() const override { return ASTNodeKind::MemberAccessExpr; }

    std::unique_ptr<Expr> cloneExpr() const override {
        return std::make_unique<MemberAccessExpr>(base->cloneExpr(), memberName, m_isStatic);
    }

    const Expr* getBase() const { return base.get(); }
    const std::string& getMemberName() const { return memberName; }
    bool isStatic() const { return m_isStatic; }

    std::string toString() const override {
        return base->toString() + (m_isStatic ? "::" : ".") + memberName;
    }

private:
    std::unique_ptr<Expr> base;
    std::string memberName;
    bool m_isStatic;
};

class StructLiteralExpr : public Expr {
public:
    struct FieldInit {
        std::string name;
        std::unique_ptr<Expr> value;
    };

    StructLiteralExpr(std::unique_ptr<Expr> base, std::vector<FieldInit> fields)
        : base(std::move(base)), fields(std::move(fields)) {}

    ASTNodeKind getKind() const override { return ASTNodeKind::StructLiteralExpr; }

    std::unique_ptr<Expr> cloneExpr() const override {
        std::vector<FieldInit> newFields;
        for (const auto& f : fields) newFields.push_back({f.name, f.value->cloneExpr()});
        return std::make_unique<StructLiteralExpr>(base->cloneExpr(), std::move(newFields));
    }

    const Expr* getBase() const { return base.get(); }
    const std::vector<FieldInit>& getFields() const { return fields; }

    std::string toString() const override {
        std::string result = base->toString() + " { ";
        for (size_t i = 0; i < fields.size(); ++i) {
            result += fields[i].name + ": " + fields[i].value->toString();
            if (i < fields.size() - 1) result += ", ";
        }
        result += " }";
        return result;
    }

private:
    std::unique_ptr<Expr> base;
    std::vector<FieldInit> fields;
};

class ArrayLiteralExpr : public Expr {
public:
    ArrayLiteralExpr(std::vector<std::unique_ptr<Expr>> elements)
        : elements(std::move(elements)) {}

    ASTNodeKind getKind() const override { return ASTNodeKind::ArrayLiteralExpr; }

    std::unique_ptr<Expr> cloneExpr() const override {
        std::vector<std::unique_ptr<Expr>> newElements;
        for (const auto& e : elements) newElements.push_back(e->cloneExpr());
        return std::make_unique<ArrayLiteralExpr>(std::move(newElements));
    }

    const std::vector<std::unique_ptr<Expr>>& getElements() const { return elements; }

    std::string toString() const override {
        std::string res = "[";
        for (size_t i = 0; i < elements.size(); ++i) {
            res += elements[i]->toString();
            if (i < elements.size() - 1) res += ", ";
        }
        res += "]";
        return res;
    }

private:
    std::vector<std::unique_ptr<Expr>> elements;
};

class IndexingExpr : public Expr {
public:
    IndexingExpr(std::unique_ptr<Expr> base, std::unique_ptr<Expr> index)
        : base(std::move(base)), index(std::move(index)) {}

    ASTNodeKind getKind() const override { return ASTNodeKind::IndexingExpr; }

    std::unique_ptr<Expr> cloneExpr() const override {
        return std::make_unique<IndexingExpr>(base->cloneExpr(), index->cloneExpr());
    }

    const Expr* getBase() const { return base.get(); }
    const Expr* getIndex() const { return index.get(); }

    std::string toString() const override {
        return base->toString() + "[" + index->toString() + "]";
    }

private:
    std::unique_ptr<Expr> base;
    std::unique_ptr<Expr> index;
};

class UnaryExpr : public Expr {
public:
    UnaryExpr(TokenType op, std::unique_ptr<Expr> operand)
        : op(op), operand(std::move(operand)) {}

    ASTNodeKind getKind() const override { return ASTNodeKind::UnaryExpr; }

    std::unique_ptr<Expr> cloneExpr() const override {
        return std::make_unique<UnaryExpr>(op, operand->cloneExpr());
    }

    TokenType getOp() const { return op; }
    const Expr* getOperand() const { return operand.get(); }

    std::string toString() const override {
        std::string opStr;
        switch (op) {
            case TokenType::Plus: opStr = "+"; break;
            case TokenType::Minus: opStr = "-"; break;
            case TokenType::Not: opStr = "!"; break;
            case TokenType::Tilde: opStr = "~"; break;
            default: opStr = "???"; break;
        }
        return opStr + operand->toString();
    }

private:
    TokenType op;
    std::unique_ptr<Expr> operand;
};

class AddressOfExpr : public Expr {
public:
    AddressOfExpr(std::unique_ptr<Expr> operand) : operand(std::move(operand)) {}
    ASTNodeKind getKind() const override { return ASTNodeKind::AddressOfExpr; }
    std::unique_ptr<Expr> cloneExpr() const override {
        return std::make_unique<AddressOfExpr>(operand->cloneExpr());
    }
    const Expr* getOperand() const { return operand.get(); }
    std::string toString() const override { return "&" + operand->toString(); }
private:
    std::unique_ptr<Expr> operand;
};

class DereferenceExpr : public Expr {
public:
    DereferenceExpr(std::unique_ptr<Expr> operand) : operand(std::move(operand)) {}
    ASTNodeKind getKind() const override { return ASTNodeKind::DereferenceExpr; }
    std::unique_ptr<Expr> cloneExpr() const override {
        return std::make_unique<DereferenceExpr>(operand->cloneExpr());
    }
    const Expr* getOperand() const { return operand.get(); }
    std::string toString() const override { return "*" + operand->toString(); }
private:
    std::unique_ptr<Expr> operand;
};

class QuestionExpr : public Expr {
public:
    QuestionExpr(std::unique_ptr<Expr> operand) : operand(std::move(operand)) {}
    ASTNodeKind getKind() const override { return ASTNodeKind::QuestionExpr; }
    std::unique_ptr<Expr> cloneExpr() const override {
        return std::make_unique<QuestionExpr>(operand->cloneExpr());
    }
    const Expr* getOperand() const { return operand.get(); }
    std::string toString() const override { return operand->toString() + "?"; }
private:
    std::unique_ptr<Expr> operand;
};

class SpecializationExpr : public Expr {
public:
    SpecializationExpr(std::unique_ptr<Expr> base, std::vector<std::shared_ptr<Type>> typeArgs)
        : base(std::move(base)), typeArgs(std::move(typeArgs)) {}

    ASTNodeKind getKind() const override { return ASTNodeKind::SpecializationExpr; }

    std::unique_ptr<Expr> cloneExpr() const override {
        return std::make_unique<SpecializationExpr>(base->cloneExpr(), typeArgs);
    }

    const Expr* getBase() const { return base.get(); }
    const std::vector<std::shared_ptr<Type>>& getTypeArgs() const { return typeArgs; }
    void setTypeArgs(std::vector<std::shared_ptr<Type>> args) { typeArgs = std::move(args); }

    void setMangledName(std::string name) { mMangledName = std::move(name); }
    const std::string& getMangledName() const { return mMangledName; }

    std::string toString() const override {
        std::string res = base->toString() + "[";
        for (size_t i = 0; i < typeArgs.size(); ++i) {
            res += typeArgs[i]->toString();
            if (i < typeArgs.size() - 1) res += ", ";
        }
        res += "]";
        return res;
    }

private:
    std::unique_ptr<Expr> base;
    std::vector<std::shared_ptr<Type>> typeArgs;
    std::string mMangledName;
};

class ConstraintExpr : public Expr {
public:
    enum class Logic { NONE, AND, OR };
    struct ConstraintItem {
        std::string traitName;
        Logic logic = Logic::NONE;
    };

    ConstraintExpr(std::vector<ConstraintItem> items) : items(std::move(items)) {}

    ASTNodeKind getKind() const override { return ASTNodeKind::ConstraintExpr; }

    std::unique_ptr<Expr> cloneExpr() const override {
        return std::make_unique<ConstraintExpr>(items);
    }

    const std::vector<ConstraintItem>& getItems() const { return items; }

    std::string toString() const override {
        std::string res;
        for (size_t i = 0; i < items.size(); ++i) {
            res += items[i].traitName;
            if (i < items.size() - 1) {
                switch (items[i+1].logic) {
                    case Logic::AND: res += " && "; break;
                    case Logic::OR: res += " || "; break;
                    default: break;
                }
            }
        }
        return res;
    }

private:
    std::vector<ConstraintItem> items;
};

class IntrinsicExpr : public Expr {
public:
    enum class IntrinsicKind {
        Sizeof,
        Malloc,
        Alloca,
        Free,
        Alignof,
        Offsetof
    };

    IntrinsicExpr(IntrinsicKind kind, std::shared_ptr<Type> typeArg, std::vector<std::unique_ptr<Expr>> args)
        : m_intrinsicKind(kind), typeArg(typeArg), args(std::move(args)) {}

    ASTNodeKind getKind() const override { return ASTNodeKind::IntrinsicExpr; }

    std::unique_ptr<Expr> cloneExpr() const override {
        std::vector<std::unique_ptr<Expr>> newArgs;
        for (const auto& arg : args) newArgs.push_back(arg->cloneExpr());
        return std::make_unique<IntrinsicExpr>(m_intrinsicKind, typeArg, std::move(newArgs));
    }

    IntrinsicKind getIntrinsicKind() const { return m_intrinsicKind; }
    std::shared_ptr<Type> getTypeArg() const { return typeArg; }
    void setTypeArg(std::shared_ptr<Type> t) { typeArg = t; }
    const std::vector<std::unique_ptr<Expr>>& getArgs() const { return args; }

    std::string toString() const override {
        std::string res;
        switch (m_intrinsicKind) {
            case IntrinsicKind::Sizeof: res = "sizeof"; break;
            case IntrinsicKind::Malloc: res = "malloc"; break;
            case IntrinsicKind::Alloca: res = "alloca"; break;
            case IntrinsicKind::Free: res = "free"; break;
            case IntrinsicKind::Alignof: res = "alignof"; break;
            case IntrinsicKind::Offsetof: res = "offsetof"; break;
        }
        if (typeArg) res += "[" + typeArg->toString() + "]";
        res += "(";
        for (size_t i = 0; i < args.size(); ++i) {
            res += args[i]->toString();
            if (i < args.size() - 1) res += ", ";
        }
        res += ")";
        return res;
    }

private:
    IntrinsicKind m_intrinsicKind;
    std::shared_ptr<Type> typeArg;
    std::vector<std::unique_ptr<Expr>> args;
};

} // namespace chtholly

#endif // CHTHOLLY_EXPRESSIONS_H
