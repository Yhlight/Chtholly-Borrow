#ifndef CHTHOLLY_STATEMENTS_H
#define CHTHOLLY_STATEMENTS_H

#include "AST/ASTNode.h"
#include "AST/Expressions.h"
#include <vector>
#include <memory>

namespace chtholly {

class Stmt : public ASTNode {
public:
    virtual ~Stmt() = default;
    virtual std::unique_ptr<Stmt> cloneStmt() const = 0;
    std::unique_ptr<ASTNode> clone() const override { return cloneStmt(); }
};

class Block : public Stmt {
public:
    Block(std::vector<std::unique_ptr<Stmt>> statements)
        : statements(std::move(statements)) {}

    ASTNodeKind getKind() const override { return ASTNodeKind::Block; }

    std::unique_ptr<Stmt> cloneStmt() const override {
        std::vector<std::unique_ptr<Stmt>> newStmts;
        for (const auto& s : statements) newStmts.push_back(s->cloneStmt());
        return std::make_unique<Block>(std::move(newStmts));
    }

    const std::vector<std::unique_ptr<Stmt>>& getStatements() const { return statements; }

    std::string toString() const override {
        std::string result = "{\n";
        for (const auto& stmt : statements) {
            result += "  " + stmt->toString() + "\n";
        }
        result += "}";
        return result;
    }

private:
    std::vector<std::unique_ptr<Stmt>> statements;
};

class IfStmt : public Stmt {
public:
    IfStmt(std::unique_ptr<Expr> condition, std::unique_ptr<Block> thenBlock, std::unique_ptr<Block> elseBlock = nullptr)
        : condition(std::move(condition)), thenBlock(std::move(thenBlock)), elseBlock(std::move(elseBlock)) {}

    ASTNodeKind getKind() const override { return ASTNodeKind::IfStmt; }

    std::unique_ptr<Stmt> cloneStmt() const override {
        auto newThen = std::unique_ptr<Block>(static_cast<Block*>(thenBlock->cloneStmt().release()));
        std::unique_ptr<Block> newElse = nullptr;
        if (elseBlock) newElse = std::unique_ptr<Block>(static_cast<Block*>(elseBlock->cloneStmt().release()));
        return std::make_unique<IfStmt>(condition->cloneExpr(), std::move(newThen), std::move(newElse));
    }

    const Expr* getCondition() const { return condition.get(); }
    const Block* getThenBlock() const { return thenBlock.get(); }
    const Block* getElseBlock() const { return elseBlock.get(); }

    std::string toString() const override {
        std::string result = "if " + condition->toString() + " " + thenBlock->toString();
        if (elseBlock) {
            result += " else " + elseBlock->toString();
        }
        return result;
    }

private:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Block> thenBlock;
    std::unique_ptr<Block> elseBlock;
};

class WhileStmt : public Stmt {
public:
    WhileStmt(std::unique_ptr<Expr> condition, std::unique_ptr<Block> body)
        : condition(std::move(condition)), body(std::move(body)) {}

    ASTNodeKind getKind() const override { return ASTNodeKind::WhileStmt; }

    std::unique_ptr<Stmt> cloneStmt() const override {
        auto newBody = std::unique_ptr<Block>(static_cast<Block*>(body->cloneStmt().release()));
        return std::make_unique<WhileStmt>(condition->cloneExpr(), std::move(newBody));
    }

    const Expr* getCondition() const { return condition.get(); }
    const Block* getBody() const { return body.get(); }

    std::string toString() const override {
        return "while " + condition->toString() + " " + body->toString();
    }

private:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Block> body;
};

class ReturnStmt : public Stmt {
public:
    ReturnStmt(std::unique_ptr<Expr> expression)
        : expression(std::move(expression)) {}

    ASTNodeKind getKind() const override { return ASTNodeKind::ReturnStmt; }

    std::unique_ptr<Stmt> cloneStmt() const override {
        return std::make_unique<ReturnStmt>(expression ? expression->cloneExpr() : nullptr);
    }

    const Expr* getExpression() const { return expression.get(); }

    std::string toString() const override {
        std::string result = "return";
        if (expression) {
            result += " " + expression->toString();
        }
        result += ";";
        return result;
    }

private:
    std::unique_ptr<Expr> expression;
};

class ExprStmt : public Stmt {
public:
    ExprStmt(std::unique_ptr<Expr> expression)
        : expression(std::move(expression)) {}

    ASTNodeKind getKind() const override { return ASTNodeKind::ExprStmt; }

    std::unique_ptr<Stmt> cloneStmt() const override {
        return std::make_unique<ExprStmt>(expression->cloneExpr());
    }

    const Expr* getExpression() const { return expression.get(); }

    std::string toString() const override {
        return expression->toString() + ";";
    }

private:
    std::unique_ptr<Expr> expression;
};

class BreakStmt : public Stmt {
public:
    ASTNodeKind getKind() const override { return ASTNodeKind::BreakStmt; }
    std::unique_ptr<Stmt> cloneStmt() const override { return std::make_unique<BreakStmt>(); }
    std::string toString() const override { return "break;"; }
};

class ContinueStmt : public Stmt {
public:
    ASTNodeKind getKind() const override { return ASTNodeKind::ContinueStmt; }
    std::unique_ptr<Stmt> cloneStmt() const override { return std::make_unique<ContinueStmt>(); }
    std::string toString() const override { return "continue;"; }
};

class DoWhileStmt : public Stmt {
public:
    DoWhileStmt(std::unique_ptr<Block> body, std::unique_ptr<Expr> condition)
        : body(std::move(body)), condition(std::move(condition)) {}

    ASTNodeKind getKind() const override { return ASTNodeKind::DoWhileStmt; }

    std::unique_ptr<Stmt> cloneStmt() const override {
        auto newBody = std::unique_ptr<Block>(static_cast<Block*>(body->cloneStmt().release()));
        return std::make_unique<DoWhileStmt>(std::move(newBody), condition->cloneExpr());
    }

    const Block* getBody() const { return body.get(); }
    const Expr* getCondition() const { return condition.get(); }

    std::string toString() const override {
        return "do " + body->toString() + " while (" + condition->toString() + ");";
    }

private:
    std::unique_ptr<Block> body;
    std::unique_ptr<Expr> condition;
};

class ForStmt : public Stmt {
public:
    ForStmt(std::unique_ptr<Stmt> init, std::unique_ptr<Expr> condition, std::unique_ptr<Expr> step, std::unique_ptr<Block> body)
        : init(std::move(init)), condition(std::move(condition)), step(std::move(step)), body(std::move(body)) {}

    ASTNodeKind getKind() const override { return ASTNodeKind::ForStmt; }

    std::unique_ptr<Stmt> cloneStmt() const override {
        auto newBody = std::unique_ptr<Block>(static_cast<Block*>(body->cloneStmt().release()));
        return std::make_unique<ForStmt>(init ? init->cloneStmt() : nullptr, 
                                         condition ? condition->cloneExpr() : nullptr, 
                                         step ? step->cloneExpr() : nullptr, 
                                         std::move(newBody));
    }

    const Stmt* getInit() const { return init.get(); }
    const Expr* getCondition() const { return condition.get(); }
    const Expr* getStep() const { return step.get(); }
    const Block* getBody() const { return body.get(); }

    std::string toString() const override {
        std::string res = "for (";
        if (init) res += init->toString();
        else res += ";";
        res += " ";
        if (condition) res += condition->toString();
        res += "; ";
        if (step) res += step->toString();
        res += ") " + body->toString();
        return res;
    }

private:
    std::unique_ptr<Stmt> init;
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Expr> step;
    std::unique_ptr<Block> body;
};

} // namespace chtholly

#include "AST/Patterns.h"

namespace chtholly {

class CaseStmt : public Stmt {
public:
    CaseStmt(std::unique_ptr<Pattern> pattern, std::unique_ptr<Block> body, bool isDefault = false)
        : pattern(std::move(pattern)), body(std::move(body)), isDefault(isDefault) {}

    ASTNodeKind getKind() const override { return ASTNodeKind::CaseStmt; }

    std::unique_ptr<Stmt> cloneStmt() const override {
        auto newBody = std::unique_ptr<Block>(static_cast<Block*>(body->cloneStmt().release()));
        return std::make_unique<CaseStmt>(pattern ? pattern->clonePattern() : nullptr, std::move(newBody), isDefault);
    }

    const Pattern* getPattern() const { return pattern.get(); }
    const Block* getBody() const { return body.get(); }
    bool isDefaultCase() const { return isDefault; }

    std::string toString() const override {
        if (isDefault) return "default: " + body->toString();
        return "case " + (pattern ? pattern->toString() : "") + ": " + body->toString();
    }

private:
    std::unique_ptr<Pattern> pattern;
    std::unique_ptr<Block> body;
    bool isDefault;
};

class SwitchStmt : public Stmt {
public:
    SwitchStmt(std::unique_ptr<Expr> condition, std::vector<std::unique_ptr<CaseStmt>> cases)
        : condition(std::move(condition)), cases(std::move(cases)) {}

    ASTNodeKind getKind() const override { return ASTNodeKind::SwitchStmt; }

    std::unique_ptr<Stmt> cloneStmt() const override {
        std::vector<std::unique_ptr<CaseStmt>> newCases;
        for (const auto& c : cases) newCases.push_back(std::unique_ptr<CaseStmt>(static_cast<CaseStmt*>(c->cloneStmt().release())));
        return std::make_unique<SwitchStmt>(condition->cloneExpr(), std::move(newCases));
    }

    const Expr* getCondition() const { return condition.get(); }
    const std::vector<std::unique_ptr<CaseStmt>>& getCases() const { return cases; }

    std::string toString() const override {
        std::string res = "switch (" + condition->toString() + ") {\n";
        for (const auto& c : cases) {
            res += "  " + c->toString() + "\n";
        }
        res += "}";
        return res;
    }

private:
    std::unique_ptr<Expr> condition;
    std::vector<std::unique_ptr<CaseStmt>> cases;
};

} // namespace chtholly

#endif // CHTHOLLY_STATEMENTS_H
