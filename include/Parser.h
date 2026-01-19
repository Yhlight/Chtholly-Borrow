#ifndef CHTHOLLY_PARSER_H
#define CHTHOLLY_PARSER_H

#include "Lexer/Lexer.h"
#include "AST/Declarations.h"
#include "AST/Expressions.h"
#include "AST/Statements.h"
#include "AST/ImportDecl.h"
#include <memory>
#include <string>
#include <set>
#include <vector>

namespace chtholly {

class Parser {
public:
    Parser(std::string_view source);

    std::vector<std::unique_ptr<ASTNode>> parseProgram();
    std::unique_ptr<ImportDecl> parseImportDecl();
    std::unique_ptr<PackageDecl> parsePackageDecl();
    std::unique_ptr<UseDecl> parseUseDecl();
    std::unique_ptr<RequestDecl> parseRequestDecl(bool isPublic = false);
    std::unique_ptr<Stmt> parseStatement();
    std::unique_ptr<Block> parseBlock();
    std::unique_ptr<VarDecl> parseVarDecl(bool isPublic = false);
    std::unique_ptr<VarDecl> parseStructField();
    std::unique_ptr<FunctionDecl> parseFunctionDecl(bool isPublic = false);
    std::unique_ptr<StructDecl> parseStructDecl(bool isPublic = false);
    std::unique_ptr<EnumDecl> parseEnumDecl(bool isPublic = false);
    std::unique_ptr<ClassDecl> parseClassDecl(bool isPublic = false);
    std::unique_ptr<MethodDecl> parseMethodDecl(bool isPublic = false);
    std::unique_ptr<ConstructorDecl> parseConstructorDecl(bool isPublic = false);
    std::unique_ptr<StructLiteralExpr> parseStructLiteral(std::unique_ptr<Expr> base);
    std::unique_ptr<IfStmt> parseIfStmt();
    std::unique_ptr<WhileStmt> parseWhileStmt();
    std::unique_ptr<DoWhileStmt> parseDoWhileStmt();
    std::unique_ptr<ForStmt> parseForStmt();
    std::unique_ptr<SwitchStmt> parseSwitchStmt();
    std::unique_ptr<CaseStmt> parseCaseStmt();
    std::unique_ptr<Pattern> parsePattern();
    std::unique_ptr<ReturnStmt> parseReturnStmt();
    std::unique_ptr<BreakStmt> parseBreakStmt();
    std::unique_ptr<ContinueStmt> parseContinueStmt();
    std::unique_ptr<Expr> parseExpression(int minPrec = 0);
    std::unique_ptr<Expr> parseUnary();
    std::unique_ptr<Expr> parsePostfix();
    std::unique_ptr<Expr> parsePrimary();

private:
    Token advance();
    Token peek();
    bool match(TokenType type);
    Token consume(TokenType type, const std::string& message);

    int getPrecedence(TokenType type);

    std::shared_ptr<Type> parseType();
    std::unique_ptr<ConstraintExpr> parseConstraintExpr();
    bool isGenericContext();
    std::vector<GenericParam> parseGenericParams();
    std::unique_ptr<Param> parseParam();

    Lexer m_lexer;
    Token m_currentToken;
    std::vector<std::set<std::string>> m_activeGenericParams;
};

} // namespace chtholly

#endif // CHTHOLLY_PARSER_H
