#ifndef CHTHOLLY_DECLARATIONS_H
#define CHTHOLLY_DECLARATIONS_H

#include "AST/ASTNode.h"
#include "AST/Statements.h"
#include "AST/Types.h"
#include "AST/Expressions.h"
#include <string>
#include <memory>
#include <vector>

namespace chtholly {

class Decl : public Stmt {
public:
    virtual ~Decl() = default;
    virtual std::unique_ptr<Decl> cloneDecl() const = 0;
    std::unique_ptr<Stmt> cloneStmt() const override { return cloneDecl(); }
};

class VarDecl : public Decl {
public:
    VarDecl(std::string name, std::shared_ptr<Type> type, std::unique_ptr<Expr> initializer, bool isMutable, bool isPublic = false)
        : name(name), type(type), initializer(std::move(initializer)), m_isMutable(isMutable), m_isPublic(isPublic) {}

    ASTNodeKind getKind() const override { return ASTNodeKind::VarDecl; }

    std::unique_ptr<Decl> cloneDecl() const override {
        return std::unique_ptr<Decl>(std::make_unique<VarDecl>(name, type, initializer ? initializer->cloneExpr() : nullptr, m_isMutable, m_isPublic).release());
    }

    const std::string& getName() const override { return name; }
    std::shared_ptr<Type> getType() const override { return type; }
    bool isMutable() const { return m_isMutable; }
    bool isPublic() const { return m_isPublic; }
    void setType(std::shared_ptr<Type> t) { type = t; }
    const Expr* getInitializer() const { return initializer.get(); }

    std::string toString() const override {
        std::string result = m_isPublic ? "pub " : "";
        result += "let ";
        if (m_isMutable) result += "mut ";
        result += name;
        if (type) {
            result += ": " + type->toString();
        }
        if (initializer) {
            result += " = " + initializer->toString();
        }
        result += ";";
        return result;
    }

private:
    std::string name;
    std::shared_ptr<Type> type;
    std::unique_ptr<Expr> initializer;
    bool m_isMutable;
    bool m_isPublic;
};

class Param : public ASTNode {
public:
    Param(std::string name, std::shared_ptr<Type> type)
        : name(name), type(type) {}

    ASTNodeKind getKind() const override { return ASTNodeKind::Param; }

    std::unique_ptr<ASTNode> clone() const override {
        return std::make_unique<Param>(name, type);
    }

    const std::string& getName() const override { return name; }
    std::shared_ptr<Type> getType() const override { return type; }
    void setType(std::shared_ptr<Type> t) { type = t; }

    std::string toString() const override {
        return name + ": " + type->toString();
    }

private:
    std::string name;
    std::shared_ptr<Type> type;
};

struct GenericParam {
    std::string name;
    std::unique_ptr<ConstraintExpr> constraint;

    GenericParam(std::string n, std::unique_ptr<ConstraintExpr> c = nullptr)
        : name(std::move(n)), constraint(std::move(c)) {}

    GenericParam(const GenericParam& other)
        : name(other.name), constraint(other.constraint ? std::unique_ptr<ConstraintExpr>(static_cast<ConstraintExpr*>(other.constraint->cloneExpr().release())) : nullptr) {}

    GenericParam& operator=(const GenericParam& other) {
        if (this != &other) {
            name = other.name;
            constraint = other.constraint ? std::unique_ptr<ConstraintExpr>(static_cast<ConstraintExpr*>(other.constraint->cloneExpr().release())) : nullptr;
        }
        return *this;
    }
};

class FunctionDecl : public Decl {
public:
    FunctionDecl(std::string name, std::shared_ptr<Type> returnType, std::vector<std::unique_ptr<Param>> params, std::unique_ptr<Block> body, bool isExtern = false, bool isPublic = false, std::vector<GenericParam> genericParams = {}) 
        : name(std::move(name)), returnType(returnType), params(std::move(params)), body(std::move(body)), isExtern(isExtern), isPublic(isPublic), isVarArg(false), genericParams(std::move(genericParams)) {}

    ASTNodeKind getKind() const override { return ASTNodeKind::FunctionDecl; }

    std::unique_ptr<Decl> cloneDecl() const override {
        std::vector<std::unique_ptr<Param>> newParams;
        for (const auto& p : params) newParams.push_back(std::unique_ptr<Param>(static_cast<Param*>(p->clone().release())));
        auto newBody = body ? std::unique_ptr<Block>(static_cast<Block*>(body->cloneStmt().release())) : nullptr;
        std::vector<GenericParam> newGParams;
        for (const auto& gp : genericParams) newGParams.push_back(gp);
        auto res = std::make_unique<FunctionDecl>(name, returnType, std::move(newParams), std::move(newBody), isExtern, isPublic, std::move(newGParams));
        res->setVarArg(isVarArg);
        return std::unique_ptr<Decl>(res.release());
    }

    std::string toString() const override {
        std::string res = isPublic ? "pub " : "";
        res += isExtern ? "extern fn " : "fn ";
        res += name;
        if (!genericParams.empty()) {
            res += "[";
            for (size_t i = 0; i < genericParams.size(); ++i) {
                res += genericParams[i].name;
                if (genericParams[i].constraint) res += " ? " + genericParams[i].constraint->toString();
                if (i < genericParams.size() - 1) res += ", ";
            }
            res += "]";
        }
        res += "(";
        for (size_t i = 0; i < params.size(); ++i) {
            res += params[i]->toString();
            if (i < params.size() - 1) res += ", ";
        }
        if (isVarArg) {
            if (!params.empty()) res += ", ";
            res += "...";
        }
        res += "): " + returnType->toString();
        if (body) res += " " + body->toString();
        else res += ";";
        return res;
    }

    const std::string& getName() const override { return name; }
    void setName(const std::string& n) { name = n; }
    std::shared_ptr<Type> getReturnType() const { return returnType; }
    void setReturnType(std::shared_ptr<Type> t) { returnType = t; }
    const std::vector<std::unique_ptr<Param>>& getParams() const { return params; }
    Block* getBody() const { return body.get(); }
    bool getIsExtern() const { return isExtern; }
    bool getIsPublic() const { return isPublic; }
    const std::vector<GenericParam>& getGenericParams() const { return genericParams; }
    void clearGenericParams() { genericParams.clear(); }
    
    void setVarArg(bool v) { isVarArg = v; }
    bool getVarArg() const { return isVarArg; }

    std::shared_ptr<Type> getType() const override {
        std::vector<std::shared_ptr<Type>> paramTypes;
        for (const auto& p : params) paramTypes.push_back(p->getType());
        return std::make_shared<FunctionType>(std::move(paramTypes), returnType, isVarArg);
    }

private:
    std::string name;
    std::shared_ptr<Type> returnType;
    std::vector<std::unique_ptr<Param>> params;
    std::unique_ptr<Block> body;
    bool isExtern;
    bool isPublic;
    bool isVarArg;
    std::vector<GenericParam> genericParams;
};

class MethodDecl : public Stmt {
public:
    MethodDecl(std::string name, std::shared_ptr<Type> returnType, std::vector<std::unique_ptr<Param>> params, std::unique_ptr<Block> body, bool isPublic = false, std::vector<GenericParam> genericParams = {})
        : name(std::move(name)), returnType(returnType), params(std::move(params)), body(std::move(body)), m_isPublic(isPublic), genericParams(std::move(genericParams)) {}

    ASTNodeKind getKind() const override { return ASTNodeKind::MethodDecl; }

    std::unique_ptr<Stmt> cloneStmt() const override {
        std::vector<std::unique_ptr<Param>> newParams;
        for (const auto& p : params) newParams.push_back(std::unique_ptr<Param>(static_cast<Param*>(p->clone().release())));
        auto newBody = body ? std::unique_ptr<Block>(static_cast<Block*>(body->cloneStmt().release())) : nullptr;
        std::vector<GenericParam> newGParams;
        for (const auto& gp : genericParams) newGParams.push_back(gp);
        return std::unique_ptr<Stmt>(std::make_unique<MethodDecl>(name, returnType, std::move(newParams), std::move(newBody), m_isPublic, std::move(newGParams)).release());
    }

    std::string toString() const override {
        std::string res = m_isPublic ? "pub fn " : "fn ";
        res += name;
        if (!genericParams.empty()) {
            res += "[";
            for (size_t i = 0; i < genericParams.size(); ++i) {
                res += genericParams[i].name;
                if (genericParams[i].constraint) res += " ? " + genericParams[i].constraint->toString();
                if (i < genericParams.size() - 1) res += ", ";
            }
            res += "]";
        }
        res += "(";
        for (size_t i = 0; i < params.size(); ++i) {
            res += params[i]->toString();
            if (i < params.size() - 1) res += ", ";
        }
        res += "): " + returnType->toString();
        if (body) res += " " + body->toString();
        else res += ";";
        return res;
    }

    const std::string& getName() const override { return name; }
    std::shared_ptr<Type> getReturnType() const { return returnType; }
    void setReturnType(std::shared_ptr<Type> t) { returnType = t; }
    const std::vector<std::unique_ptr<Param>>& getParams() const { return params; }
    Block* getBody() const { return body.get(); }
    bool isPublic() const { return m_isPublic; }
    const std::vector<GenericParam>& getGenericParams() const { return genericParams; }
    void clearGenericParams() { genericParams.clear(); }

private:
    std::string name;
    std::shared_ptr<Type> returnType;
    std::vector<std::unique_ptr<Param>> params;
    std::unique_ptr<Block> body;
    bool m_isPublic;
    std::vector<GenericParam> genericParams;
};

class ConstructorDecl : public Stmt {
public:
    ConstructorDecl(std::string name, std::vector<std::unique_ptr<Param>> params, std::unique_ptr<Block> body, bool isPublic = false)
        : name(std::move(name)), params(std::move(params)), body(std::move(body)), m_isPublic(isPublic) {}

    ASTNodeKind getKind() const override { return ASTNodeKind::ConstructorDecl; }

    std::unique_ptr<Stmt> cloneStmt() const override {
        std::vector<std::unique_ptr<Param>> newParams;
        for (const auto& p : params) newParams.push_back(std::unique_ptr<Param>(static_cast<Param*>(p->clone().release())));
        auto newBody = body ? std::unique_ptr<Block>(static_cast<Block*>(body->cloneStmt().release())) : nullptr;
        return std::unique_ptr<Stmt>(std::make_unique<ConstructorDecl>(name, std::move(newParams), std::move(newBody), m_isPublic).release());
    }

    std::string toString() const override {
        std::string res = m_isPublic ? "pub " : "";
        res += name + "(";
        for (size_t i = 0; i < params.size(); ++i) {
            res += params[i]->toString();
            if (i < params.size() - 1) res += ", ";
        }
        res += ") " + body->toString();
        return res;
    }

    const std::string& getName() const override { return name; }
    const std::vector<std::unique_ptr<Param>>& getParams() const { return params; }
    Block* getBody() const { return body.get(); }
    bool isPublic() const { return m_isPublic; }

private:
    std::string name;
    std::vector<std::unique_ptr<Param>> params;
    std::unique_ptr<Block> body;
    bool m_isPublic;
};

class StructDecl : public Decl {
public:
    StructDecl(std::string name, std::vector<std::unique_ptr<VarDecl>> members, bool isPublic = false, std::vector<GenericParam> genericParams = {}, std::vector<std::string> requirements = {}) 
        : name(std::move(name)), members(std::move(members)), m_isPublic(isPublic), genericParams(std::move(genericParams)), requirements(std::move(requirements)) {}

    ASTNodeKind getKind() const override { return ASTNodeKind::StructDecl; }

    std::unique_ptr<Decl> cloneDecl() const override {
        std::vector<std::unique_ptr<VarDecl>> newMembers;
        for (const auto& m : members) newMembers.push_back(std::unique_ptr<VarDecl>(static_cast<VarDecl*>(m->cloneDecl().release())));
        std::vector<GenericParam> newGParams;
        for (const auto& gp : genericParams) newGParams.push_back(gp);
        return std::unique_ptr<Decl>(std::make_unique<StructDecl>(name, std::move(newMembers), m_isPublic, std::move(newGParams), requirements).release());
    }

    std::string toString() const override {
        std::string res = m_isPublic ? "pub " : "";
        res += "struct " + name;
        if (!genericParams.empty()) {
            res += "[";
            for (size_t i = 0; i < genericParams.size(); ++i) {
                res += genericParams[i].name;
                if (genericParams[i].constraint) res += " ? " + genericParams[i].constraint->toString();
                if (i < genericParams.size() - 1) res += ", ";
            }
            res += "]";
        }
        if (!requirements.empty()) {
            res += " require ";
            for (size_t i = 0; i < requirements.size(); ++i) {
                res += requirements[i];
                if (i < requirements.size() - 1) res += ", ";
            }
        }
        res += " {\n";
        for (const auto& member : members) {
            res += "  " + member->toString() + ";\n";
        }
        res += "}";
        return res;
    }

    const std::string& getName() const override { return name; }
    void setName(const std::string& n) { name = n; }
    const std::vector<std::unique_ptr<VarDecl>>& getMembers() const { return members; }
    bool isPublic() const { return m_isPublic; }
    const std::vector<GenericParam>& getGenericParams() const { return genericParams; }
    void clearGenericParams() { genericParams.clear(); }
    const std::vector<std::string>& getRequirements() const { return requirements; }

private:
    std::string name;
    std::vector<std::unique_ptr<VarDecl>> members;
    bool m_isPublic;
    std::vector<GenericParam> genericParams;
    std::vector<std::string> requirements;
};

class ClassDecl : public Decl {
public:
    ClassDecl(std::string name, std::vector<std::unique_ptr<ASTNode>> members, bool isPublic = false, std::vector<GenericParam> genericParams = {}, std::vector<std::string> requirements = {}) 
        : name(std::move(name)), members(std::move(members)), m_isPublic(isPublic), genericParams(std::move(genericParams)), requirements(std::move(requirements)) {}

    ASTNodeKind getKind() const override { return ASTNodeKind::ClassDecl; }

    std::unique_ptr<Decl> cloneDecl() const override {
        std::vector<std::unique_ptr<ASTNode>> newMembers;
        for (const auto& m : members) newMembers.push_back(m->clone());
        std::vector<GenericParam> newGParams;
        for (const auto& gp : genericParams) newGParams.push_back(gp);
        return std::unique_ptr<Decl>(std::make_unique<ClassDecl>(name, std::move(newMembers), m_isPublic, std::move(newGParams), requirements).release());
    }

    std::string toString() const override {
        std::string res = m_isPublic ? "pub " : "";
        res += "class " + name;
        if (!genericParams.empty()) {
            res += "[";
            for (size_t i = 0; i < genericParams.size(); ++i) {
                res += genericParams[i].name;
                if (genericParams[i].constraint) res += " ? " + genericParams[i].constraint->toString();
                if (i < genericParams.size() - 1) res += ", ";
            }
            res += "]";
        }
        if (!requirements.empty()) {
            res += " require ";
            for (size_t i = 0; i < requirements.size(); ++i) {
                res += requirements[i];
                if (i < requirements.size() - 1) res += ", ";
            }
        }
        res += " {\n";
        for (const auto& member : members) {
            res += "  " + member->toString() + ";\n";
        }
        res += "}";
        return res;
    }

    const std::string& getName() const override { return name; }
    void setName(const std::string& n) { name = n; }
    const std::vector<std::unique_ptr<ASTNode>>& getMembers() const { return members; }
    bool isPublic() const { return m_isPublic; }
    const std::vector<GenericParam>& getGenericParams() const { return genericParams; }
    void clearGenericParams() { genericParams.clear(); }
    const std::vector<std::string>& getRequirements() const { return requirements; }

private:
    std::string name;
    const std::vector<std::unique_ptr<ASTNode>> members;
    bool m_isPublic;
    std::vector<GenericParam> genericParams;
    std::vector<std::string> requirements;
};

class EnumVariant : public ASTNode {
public:
    enum class VariantKind { Unit, Tuple, Struct };

    EnumVariant(std::string name, VariantKind kind, std::vector<std::shared_ptr<Type>> tupleTypes = {}, std::vector<std::unique_ptr<VarDecl>> structFields = {}, bool isDefault = false)
        : name(std::move(name)), m_variantKind(kind), tupleTypes(std::move(tupleTypes)), structFields(std::move(structFields)), m_isDefault(isDefault) {}

    ASTNodeKind getKind() const override { return ASTNodeKind::EnumVariant; }

    std::unique_ptr<ASTNode> clone() const override {
        std::vector<std::unique_ptr<VarDecl>> newFields;
        for (const auto& f : structFields) newFields.push_back(std::unique_ptr<VarDecl>(static_cast<VarDecl*>(f->cloneDecl().release())));
        return std::unique_ptr<ASTNode>(std::make_unique<EnumVariant>(name, m_variantKind, tupleTypes, std::move(newFields), m_isDefault).release());
    }

    const std::string& getName() const override { return name; }
    VariantKind getVariantKind() const { return m_variantKind; }
    const std::vector<std::shared_ptr<Type>>& getTupleTypes() const { return tupleTypes; }
    void setTupleTypes(std::vector<std::shared_ptr<Type>> t) { tupleTypes = std::move(t); }
    const std::vector<std::unique_ptr<VarDecl>>& getStructFields() const { return structFields; }
    bool isDefault() const { return m_isDefault; }

    std::string toString() const override {
        std::string res = m_isDefault ? "default " : "";
        res += name;
        if (m_variantKind == VariantKind::Tuple) {
            res += "(";
            for (size_t i = 0; i < tupleTypes.size(); ++i) {
                res += tupleTypes[i]->toString();
                if (i < tupleTypes.size() - 1) res += ", ";
            }
            res += ")";
        } else if (m_variantKind == VariantKind::Struct) {
            res += " { ";
            for (size_t i = 0; i < structFields.size(); ++i) {
                res += structFields[i]->toString();
                if (i < structFields.size() - 1) res += ", ";
            }
            res += " }";
        }
        return res;
    }

private:
    std::string name;
    VariantKind m_variantKind;
    std::vector<std::shared_ptr<Type>> tupleTypes;
    std::vector<std::unique_ptr<VarDecl>> structFields;
    bool m_isDefault;
};

class EnumDecl : public Decl {
public:
    EnumDecl(std::string name, std::vector<std::unique_ptr<EnumVariant>> variants, bool isPublic = false, std::vector<GenericParam> genericParams = {}, std::vector<std::string> requirements = {}) 
        : name(std::move(name)), variants(std::move(variants)), m_isPublic(isPublic), genericParams(std::move(genericParams)), requirements(std::move(requirements)) {}

    ASTNodeKind getKind() const override { return ASTNodeKind::EnumDecl; }

    std::unique_ptr<Decl> cloneDecl() const override {
        std::vector<std::unique_ptr<EnumVariant>> newVariants;
        for (const auto& v : variants) newVariants.push_back(std::unique_ptr<EnumVariant>(static_cast<EnumVariant*>(v->clone().release())));
        std::vector<GenericParam> newGParams;
        for (const auto& gp : genericParams) newGParams.push_back(gp);
        return std::unique_ptr<Decl>(std::make_unique<EnumDecl>(name, std::move(newVariants), m_isPublic, std::move(newGParams), requirements).release());
    }

    const std::string& getName() const override { return name; }
    void setName(const std::string& n) { name = n; }
    const std::vector<std::unique_ptr<EnumVariant>>& getVariants() const { return variants; }
    bool isPublic() const { return m_isPublic; }
    const std::vector<GenericParam>& getGenericParams() const { return genericParams; }
    void clearGenericParams() { genericParams.clear(); }
    const std::vector<std::string>& getRequirements() const { return requirements; }

    void setType(std::shared_ptr<Type> t) { m_type = t; }
    std::shared_ptr<Type> getType() const override { return m_type; }

    std::string toString() const override {
        std::string res = m_isPublic ? "pub " : "";
        res += "enum " + name;
        if (!genericParams.empty()) {
            res += "[";
            for (size_t i = 0; i < genericParams.size(); ++i) {
                res += genericParams[i].name;
                if (genericParams[i].constraint) res += " ? " + genericParams[i].constraint->toString();
                if (i < genericParams.size() - 1) res += ", ";
            }
            res += "]";
        }
        if (!requirements.empty()) {
            res += " require ";
            for (size_t i = 0; i < requirements.size(); ++i) {
                res += requirements[i];
                if (i < requirements.size() - 1) res += ", ";
            }
        }
        res += " {\n";
        for (const auto& v : variants) {
            res += "  " + v->toString() + ",\n";
        }
        res += "}";
        return res;
    }

private:
    std::string name;
    std::vector<std::unique_ptr<EnumVariant>> variants;
    std::shared_ptr<Type> m_type;
    bool m_isPublic;
    std::vector<GenericParam> genericParams;
    std::vector<std::string> requirements;
};


class RequestDecl : public Decl {
public:
    enum class RequestKind { Class, Enum };

    struct Member {
        std::unique_ptr<ASTNode> decl; // VarDecl, MethodDecl, or EnumVariant
        bool isDefault;
    };

    RequestDecl(std::string name, RequestKind kind, std::vector<Member> members, std::vector<std::string> bases = {}, std::vector<GenericParam> genericParams = {}, bool isPublic = false)
        : name(std::move(name)), kind(kind), members(std::move(members)), bases(std::move(bases)), genericParams(std::move(genericParams)), m_isPublic(isPublic) {}

    ASTNodeKind getKind() const override { return ASTNodeKind::RequestDecl; }

    std::unique_ptr<Decl> cloneDecl() const override {
        std::vector<Member> newMembers;
        for (const auto& m : members) newMembers.push_back({m.decl->clone(), m.isDefault});
        std::vector<GenericParam> newGParams;
        for (const auto& gp : genericParams) newGParams.push_back(gp);
        return std::unique_ptr<Decl>(std::make_unique<RequestDecl>(name, kind, std::move(newMembers), bases, std::move(newGParams), m_isPublic).release());
    }

    std::string toString() const override {
        std::string res = m_isPublic ? "pub request " : "request ";
        res += std::string(kind == RequestKind::Class ? "class " : "enum ") + name;
        if (!genericParams.empty()) {
            res += "[";
            for (size_t i = 0; i < genericParams.size(); ++i) {
                res += genericParams[i].name;
                if (genericParams[i].constraint) res += " ? " + genericParams[i].constraint->toString();
                if (i < genericParams.size() - 1) res += ", ";
            }
            res += "]";
        }
        if (!bases.empty()) {
            res += " : ";
            for (size_t i = 0; i < bases.size(); ++i) {
                res += bases[i];
                if (i < bases.size() - 1) res += ", ";
            }
        }
        res += " {\n";
        for (const auto& m : members) {
            if (m.isDefault) res += "  default ";
            res += m.decl->toString() + "\n";
        }
        res += "}";
        return res;
    }

    const std::string& getName() const override { return name; }
    RequestKind getRequestKind() const { return kind; }
    const std::vector<Member>& getMembers() const { return members; }
    const std::vector<std::string>& getBases() const { return bases; }
    const std::vector<GenericParam>& getGenericParams() const { return genericParams; }
    bool isPublic() const { return m_isPublic; }

private:
    std::string name;
    RequestKind kind;
    std::vector<Member> members;
    std::vector<std::string> bases;
    std::vector<GenericParam> genericParams;
    bool m_isPublic;
};

} // namespace chtholly

#endif // CHTHOLLY_DECLARATIONS_H
