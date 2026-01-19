#pragma once

#include "AST/ASTNode.h"
#include <string>

namespace chtholly {

class ImportDecl : public ASTNode {
public:
    std::string path; // File path or module path
    bool isStd;       // Is it a standard library import?
    std::string alias; // Optional alias (empty if none)

    ImportDecl(const std::string& p, bool std, const std::string& a = "")
        : path(p), isStd(std), alias(a) {}

    ASTNodeKind getKind() const override { return ASTNodeKind::ImportDecl; }

    std::unique_ptr<ASTNode> clone() const override {
        return std::make_unique<ImportDecl>(path, isStd, alias);
    }

    std::string toString() const override {
        std::string res = "import ";
        if (isStd) {
            // Standard library import is usually just the path (e.g. std::io)
            // But if path contains :: we might want to preserve it or just print path
            res += path; 
        } else {
            // File import
            res += std::string("\"") + path + std::string("\"");
        }
        if (!alias.empty()) {
            res += " as " + alias;
        }
        res += ";";
        return res;
    }
};

class PackageDecl : public ASTNode {
public:
    std::string packageName;

    PackageDecl(const std::string& name) : packageName(name) {}

    ASTNodeKind getKind() const override { return ASTNodeKind::PackageDecl; }

    std::unique_ptr<ASTNode> clone() const override {
        return std::make_unique<PackageDecl>(packageName);
    }

    std::string toString() const override {
        return "package " + packageName + ";";
    }
};

class UseDecl : public ASTNode {
public:
    std::string path; // Qualified name (e.g., std::io::println)
    std::string alias; // Optional alias

    UseDecl(const std::string& p, const std::string& a = "")
        : path(p), alias(a) {}

    ASTNodeKind getKind() const override { return ASTNodeKind::UseDecl; }

    std::unique_ptr<ASTNode> clone() const override {
        return std::make_unique<UseDecl>(path, alias);
    }

    std::string toString() const override {
        std::string res = "use " + path;
        if (!alias.empty()) {
            res += " as " + alias;
        }
        res += ";";
        return res;
    }
};

} // namespace chtholly