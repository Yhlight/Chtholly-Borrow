#include "Parser.h"
#include <stdexcept>
#include <iostream>

namespace chtholly {

Parser::Parser(std::string_view source) : m_lexer(source) {
    m_currentToken = m_lexer.nextToken();
}

std::vector<std::unique_ptr<ASTNode>> Parser::parseProgram() {
    std::vector<std::unique_ptr<ASTNode>> nodes;
    while (peek().type != TokenType::EndOfFile) {
        
        bool isPublic = false;
        if (match(TokenType::Pub)) {
            isPublic = true;
        }

        if (peek().type == TokenType::Fn || peek().type == TokenType::Extern) {
            nodes.push_back(parseFunctionDecl(isPublic));
        } else if (peek().type == TokenType::Struct) {
            nodes.push_back(parseStructDecl(isPublic));
        } else if (peek().type == TokenType::Enum) {
            nodes.push_back(parseEnumDecl(isPublic));
        } else if (peek().type == TokenType::Class) {
            nodes.push_back(parseClassDecl(isPublic));
        } else if (peek().type == TokenType::Let) {
            nodes.push_back(parseVarDecl(isPublic));
        } else if (peek().type == TokenType::Package) {
            nodes.push_back(parsePackageDecl());
        } else if (peek().type == TokenType::Import) {
            nodes.push_back(parseImportDecl());
        } else if (peek().type == TokenType::Use) {
            nodes.push_back(parseUseDecl());
        } else if (peek().type == TokenType::Identifier && std::string_view(peek().value) == "request") {
            nodes.push_back(parseRequestDecl(isPublic));
        } else {
            nodes.push_back(parseStatement());
        }
    }
    return nodes;
}

Token Parser::advance() {
    Token old = m_currentToken;
    m_currentToken = m_lexer.nextToken();
    return old;
}

Token Parser::peek() {
    return m_currentToken;
}

bool Parser::match(TokenType type) {
    if (m_currentToken.type == type) {
        advance();
        return true;
    }
    return false;
}

Token Parser::consume(TokenType type, const std::string& message) {
    if (m_currentToken.type == type) {
        return advance();
    }
    throw std::runtime_error(message + " at line " + std::to_string(m_currentToken.line));
}

std::shared_ptr<Type> Parser::parseType() {
    std::shared_ptr<Type> baseType = nullptr;
    if (match(TokenType::I8)) baseType = Type::getI8();
    else if (match(TokenType::I16)) baseType = Type::getI16();
    else if (match(TokenType::I32)) baseType = Type::getI32();
    else if (match(TokenType::I64)) baseType = Type::getI64();
    else if (match(TokenType::U8)) baseType = Type::getU8();
    else if (match(TokenType::U16)) baseType = Type::getU16();
    else if (match(TokenType::U32)) baseType = Type::getU32();
    else if (match(TokenType::U64)) baseType = Type::getU64();
    else if (match(TokenType::F32)) baseType = Type::getF32();
    else if (match(TokenType::F64)) baseType = Type::getF64();
    else if (match(TokenType::Bool)) baseType = Type::getBool();
    else if (match(TokenType::Void)) baseType = Type::getVoid();
    else if (peek().type == TokenType::Identifier) {
        std::string name(peek().value);
        advance();
        
        bool isGenericParam = false;
        for (auto it = m_activeGenericParams.rbegin(); it != m_activeGenericParams.rend(); ++it) {
            if (it->count(name)) {
                isGenericParam = true;
                break;
            }
        }
        
        if (isGenericParam) {
            baseType = std::make_shared<TypeParameterType>(name);
        } else {
            baseType = std::make_shared<StructType>(name, std::vector<StructType::Field>{});
        }
    }
    else throw std::runtime_error("Expected base type at line " + std::to_string(peek().line));

    while (true) {
        if (match(TokenType::Star)) {
            baseType = std::make_shared<PointerType>(baseType);
        } else if (peek().type == TokenType::LBracket) {
            if (isGenericContext()) {
                match(TokenType::LBracket);
                std::string specName = baseType->toString() + "[";
                while (true) {
                    auto t = parseType();
                    specName += t->toString();
                    if (match(TokenType::Comma)) specName += ", ";
                    else break;
                }
                consume(TokenType::RBracket, "Expected ']' after generic arguments");
                specName += "]";
                baseType = std::make_shared<StructType>(specName, std::vector<StructType::Field>{});
            } else {
                match(TokenType::LBracket);
                // Static array: T[N]
                Token sizeToken = consume(TokenType::Integer, "Expected array size");
                int size = std::stoi(std::string(sizeToken.value));
                consume(TokenType::RBracket, "Expected ']'");
                baseType = std::make_shared<ArrayType>(baseType, size);
            }
        } else {
            break;
        }
    }
    return baseType;
}

std::unique_ptr<Stmt> Parser::parseStatement() {
    bool isPublic = match(TokenType::Pub);

    if (peek().type == TokenType::Let) {
        return parseVarDecl(isPublic);
    }
    if (peek().type == TokenType::If) {
        return parseIfStmt();
    }
    if (peek().type == TokenType::While) {
        return parseWhileStmt();
    }
    if (peek().type == TokenType::Do) {
        return parseDoWhileStmt();
    }
    if (peek().type == TokenType::For) {
        return parseForStmt();
    }
    if (peek().type == TokenType::Switch) {
        return parseSwitchStmt();
    }
    if (peek().type == TokenType::Return) {
        return parseReturnStmt();
    }
    if (peek().type == TokenType::Break) {
        return parseBreakStmt();
    }
    if (peek().type == TokenType::Continue) {
        return parseContinueStmt();
    }
    if (peek().type == TokenType::Fn || peek().type == TokenType::Extern) {
        return parseFunctionDecl(isPublic);
    }
    if (peek().type == TokenType::Struct) {
        return parseStructDecl(isPublic);
    }
    if (peek().type == TokenType::Enum) {
        return parseEnumDecl(isPublic);
    }
    if (peek().type == TokenType::Class) {
        return parseClassDecl(isPublic);
    }
    if (peek().type == TokenType::Identifier && peek().value == "request") {
        return parseRequestDecl();
    }
    if (peek().type == TokenType::LBrace) {
        return parseBlock();
    }
    
    // Fallback: Expression Statement
    auto expr = parseExpression();
    consume(TokenType::Semicolon, "Expected ';'");
    return std::make_unique<ExprStmt>(std::move(expr));
}

std::unique_ptr<Block> Parser::parseBlock() {
    consume(TokenType::LBrace, "Expected '{'");
    std::vector<std::unique_ptr<Stmt>> statements;
    while (!match(TokenType::RBrace)) {
        if (m_lexer.peekToken().type == TokenType::EndOfFile) {
            throw std::runtime_error("Unterminated block");
        }
        statements.push_back(parseStatement());
    }
    return std::make_unique<Block>(std::move(statements));
}

std::unique_ptr<VarDecl> Parser::parseVarDecl(bool isPublic) {
    consume(TokenType::Let, "Expected 'let'");
    bool isMutable = match(TokenType::Mut);
    Token nameToken = consume(TokenType::Identifier, "Expected identifier");
    std::string name(nameToken.value);

    std::shared_ptr<Type> type = nullptr;
    if (match(TokenType::Colon)) {
        type = parseType();
    }

    std::unique_ptr<Expr> initializer = nullptr;
    if (match(TokenType::Equal)) {
        initializer = parseExpression();
    }

    consume(TokenType::Semicolon, "Expected ';'");
    return std::make_unique<VarDecl>(name, type, std::move(initializer), isMutable, isPublic);
}

std::unique_ptr<VarDecl> Parser::parseStructField() {
    bool foundPub = match(TokenType::Pub);
    if (foundPub) {
        std::cout << "Warning: struct fields are public by default, 'pub' is redundant at line " << m_currentToken.line << std::endl;
    }
    bool isPublic = true; // Struct fields are always public
    consume(TokenType::Let, "Expected 'let' in struct field");
    bool isMutable = match(TokenType::Mut);
    
    Token nameToken = consume(TokenType::Identifier, "Expected field name");
    std::string name(nameToken.value);

    consume(TokenType::Colon, "Expected ':'");
    std::shared_ptr<Type> type = parseType();
    
    match(TokenType::Semicolon); // Optional semicolon
    return std::make_unique<VarDecl>(name, type, nullptr, isMutable, isPublic);
}

std::unique_ptr<FunctionDecl> Parser::parseFunctionDecl(bool isPublic) {
    bool isExtern = match(TokenType::Extern);
    consume(TokenType::Fn, "Expected 'fn'");
    Token nameToken = consume(TokenType::Identifier, "Expected identifier");
    std::string name(nameToken.value);

    m_activeGenericParams.push_back({});
    auto genericParams = parseGenericParams();

    consume(TokenType::LParen, "Expected '('");
    std::vector<std::unique_ptr<Param>> params;
    bool isVarArg = false;
    if (!match(TokenType::RParen)) {
        do {
            if (match(TokenType::Ellipsis)) {
                isVarArg = true;
                break;
            }
            params.push_back(parseParam());
        } while (match(TokenType::Comma));
        consume(TokenType::RParen, "Expected ')'");
    }

    consume(TokenType::Colon, "Expected ':' before return type");
    auto returnType = parseType();

    std::unique_ptr<Block> body = nullptr;
    if (!isExtern) {
        body = parseBlock();
    } else {
        consume(TokenType::Semicolon, "Expected ';' after extern function declaration");
    }

    m_activeGenericParams.pop_back();
    auto decl = std::make_unique<FunctionDecl>(name, returnType, std::move(params), std::move(body), isExtern, isPublic, std::move(genericParams));
    decl->setVarArg(isVarArg);
    return decl;
}

std::unique_ptr<StructDecl> Parser::parseStructDecl(bool isPublic) {
    consume(TokenType::Struct, "Expected 'struct'");
    Token nameToken = consume(TokenType::Identifier, "Expected identifier");
    std::string name(nameToken.value);

    m_activeGenericParams.push_back({});
    auto genericParams = parseGenericParams();

    std::vector<std::string> requirements;
    if (match(TokenType::Identifier) && m_currentToken.value == "require") {
        advance(); // consume require
        do {
            requirements.push_back(std::string(consume(TokenType::Identifier, "Expected constraint name").value));
        } while (match(TokenType::Comma));
    }

    consume(TokenType::LBrace, "Expected '{'");
    std::vector<std::unique_ptr<VarDecl>> members;
    while (!match(TokenType::RBrace)) {
        members.push_back(parseStructField());
        match(TokenType::Semicolon); // Optional semicolon
    }

    m_activeGenericParams.pop_back();
    return std::make_unique<StructDecl>(name, std::move(members), isPublic, std::move(genericParams), std::move(requirements));
}

std::unique_ptr<EnumDecl> Parser::parseEnumDecl(bool isPublic) {
    consume(TokenType::Enum, "Expected 'enum'");
    Token nameToken = consume(TokenType::Identifier, "Expected identifier");
    std::string enumName(nameToken.value);

    m_activeGenericParams.push_back({});
    auto genericParams = parseGenericParams();

    std::vector<std::string> requirements;
    if (match(TokenType::Identifier) && m_currentToken.value == "require") {
        advance(); // consume require
        do {
            requirements.push_back(std::string(consume(TokenType::Identifier, "Expected constraint name").value));
        } while (match(TokenType::Comma));
    }

    consume(TokenType::LBrace, "Expected '{'");
    std::vector<std::unique_ptr<EnumVariant>> variants;
    while (!match(TokenType::RBrace)) {
        Token vNameToken = consume(TokenType::Identifier, "Expected variant name");
        std::string vName(vNameToken.value);
        EnumVariant::VariantKind kind = EnumVariant::VariantKind::Unit;
        std::vector<std::shared_ptr<Type>> tupleTypes;
        std::vector<std::unique_ptr<VarDecl>> structFields;

        if (match(TokenType::LParen)) {
            kind = EnumVariant::VariantKind::Tuple;
            if (!match(TokenType::RParen)) {
                do {
                    tupleTypes.push_back(parseType());
                } while (match(TokenType::Comma));
                consume(TokenType::RParen, "Expected ')' after tuple variant types");
            }
        } else if (match(TokenType::LBrace)) {
            kind = EnumVariant::VariantKind::Struct;
            while (!match(TokenType::RBrace)) {
                structFields.push_back(parseStructField());
                match(TokenType::Semicolon); // Optional semicolon
            }
        }
        variants.push_back(std::make_unique<EnumVariant>(vName, kind, std::move(tupleTypes), std::move(structFields)));
        match(TokenType::Comma); // Optional comma
    }

    m_activeGenericParams.pop_back();
    return std::make_unique<EnumDecl>(enumName, std::move(variants), isPublic, std::move(genericParams), std::move(requirements));
}

std::unique_ptr<ClassDecl> Parser::parseClassDecl(bool isPublic) {
    consume(TokenType::Class, "Expected 'class'");
    Token nameToken = consume(TokenType::Identifier, "Expected identifier");
    std::string name(nameToken.value);

    m_activeGenericParams.push_back({});
    auto genericParams = parseGenericParams();

    std::vector<std::string> requirements;
    if (match(TokenType::Identifier) && m_currentToken.value == "require") {
        advance(); // consume require
        do {
            requirements.push_back(std::string(consume(TokenType::Identifier, "Expected constraint name").value));
        } while (match(TokenType::Comma));
    }

    consume(TokenType::LBrace, "Expected '{'");
    std::vector<std::unique_ptr<ASTNode>> members;
    while (!match(TokenType::RBrace)) {
        bool memberPublic = match(TokenType::Pub);
        if (peek().type == TokenType::Let) {
            members.push_back(parseVarDecl(memberPublic));
        } else if (peek().type == TokenType::Fn) {
            members.push_back(parseMethodDecl(memberPublic));
        } else if (peek().type == TokenType::Tilde) {
            members.push_back(parseMethodDecl(memberPublic));
        } else if (peek().type == TokenType::Identifier) {
            if (peek().value == name) {
                members.push_back(parseConstructorDecl(memberPublic));
            } else {
                throw std::runtime_error("Unexpected identifier in class body: " + std::string(peek().value));
            }
        } else {
            throw std::runtime_error("Expected field, method, or constructor declaration in class");
        }
    }
    
    m_activeGenericParams.pop_back();
    return std::make_unique<ClassDecl>(name, std::move(members), isPublic, std::move(genericParams), std::move(requirements));
}

std::unique_ptr<ConstructorDecl> Parser::parseConstructorDecl(bool isPublic) {
    Token nameToken = consume(TokenType::Identifier, "Expected constructor name");
    std::string name(nameToken.value);

    consume(TokenType::LParen, "Expected '('");
    std::vector<std::unique_ptr<Param>> params;
    if (!match(TokenType::RParen)) {
        do {
            params.push_back(parseParam());
        } while (match(TokenType::Comma));
        consume(TokenType::RParen, "Expected ')'");
    }

    auto body = parseBlock();
    return std::make_unique<ConstructorDecl>(name, std::move(params), std::move(body), isPublic);
}

std::unique_ptr<MethodDecl> Parser::parseMethodDecl(bool isPublic) {
    match(TokenType::Fn); // Optional 'fn' prefix for destructors
    
    std::string name;
    if (match(TokenType::Tilde)) {
        name = "~";
    }
    Token nameToken = consume(TokenType::Identifier, "Expected identifier");
    name += std::string(nameToken.value);

    m_activeGenericParams.push_back({});
    auto genericParams = parseGenericParams();

    consume(TokenType::LParen, "Expected '('");
    std::vector<std::unique_ptr<Param>> params;
    
    // Handle 'self' parameter
    if (match(TokenType::Self)) {
        // self (value)
        auto selfType = std::make_shared<StructType>("Self", std::vector<StructType::Field>{});
        params.push_back(std::make_unique<Param>("self", selfType));
        if (peek().type != TokenType::RParen) {
            consume(TokenType::Comma, "Expected ',' after self");
        }
    } else if (match(TokenType::Ampersand)) {
        bool isMut = match(TokenType::Mut);
        consume(TokenType::Self, "Expected 'self' after '&' or '&mut'");
        
        // &self or &mut self
        auto selfType = std::make_shared<StructType>("Self", std::vector<StructType::Field>{});
        auto ptrType = std::make_shared<PointerType>(selfType);
        params.push_back(std::make_unique<Param>("self", ptrType));
        
        if (peek().type != TokenType::RParen) {
            consume(TokenType::Comma, "Expected ',' after self");
        }
    }

    if (!match(TokenType::RParen)) {
        do {
            params.push_back(parseParam());
        } while (match(TokenType::Comma));
        consume(TokenType::RParen, "Expected ')'");
    }

    std::shared_ptr<Type> returnType = Type::getVoid();
    if (match(TokenType::Colon)) {
        returnType = parseType();
    }

    std::unique_ptr<Block> body = nullptr;
    if (peek().type == TokenType::LBrace) {
        body = parseBlock();
    } else {
        consume(TokenType::Semicolon, "Expected ';' or block after method declaration");
    }

    m_activeGenericParams.pop_back();
    return std::make_unique<MethodDecl>(name, returnType, std::move(params), std::move(body), isPublic, std::move(genericParams));
}

std::unique_ptr<StructLiteralExpr> Parser::parseStructLiteral(std::unique_ptr<Expr> base) {
    std::vector<StructLiteralExpr::FieldInit> fields;
    if (!match(TokenType::RBrace)) {
        do {
            Token fieldNameToken = consume(TokenType::Identifier, "Expected field name");
            consume(TokenType::Colon, "Expected ':' after field name");
            auto value = parseExpression();
            fields.push_back({std::string(fieldNameToken.value), std::move(value)});
        } while (match(TokenType::Comma));
        consume(TokenType::RBrace, "Expected '}' after field initializers");
    }
    return std::make_unique<StructLiteralExpr>(std::move(base), std::move(fields));
}

std::unique_ptr<Param> Parser::parseParam() {
    Token nameToken = consume(TokenType::Identifier, "Expected identifier");
    consume(TokenType::Colon, "Expected ':'");
    auto type = parseType();
    return std::make_unique<Param>(std::string(nameToken.value), type);
}

std::unique_ptr<IfStmt> Parser::parseIfStmt() {
    consume(TokenType::If, "Expected 'if'");
    
    // Support optional parentheses around condition
    bool hasParen = match(TokenType::LParen);
    auto condition = parseExpression();
    if (hasParen) {
        consume(TokenType::RParen, "Expected ')'");
    }

    auto thenBlock = parseBlock();
    
    std::unique_ptr<Block> elseBlock = nullptr;
    if (match(TokenType::Else)) {
        if (peek().type == TokenType::If) {
            // Support else if
            std::vector<std::unique_ptr<Stmt>> elseStmts;
            elseStmts.push_back(parseIfStmt());
            elseBlock = std::make_unique<Block>(std::move(elseStmts));
        } else {
            elseBlock = parseBlock();
        }
    }

    return std::make_unique<IfStmt>(std::move(condition), std::move(thenBlock), std::move(elseBlock));
}

std::unique_ptr<WhileStmt> Parser::parseWhileStmt() {
    consume(TokenType::While, "Expected 'while'");
    
    bool hasParen = match(TokenType::LParen);
    auto condition = parseExpression();
    if (hasParen) {
        consume(TokenType::RParen, "Expected ')'");
    }

    auto body = parseBlock();
    return std::make_unique<WhileStmt>(std::move(condition), std::move(body));
}

std::unique_ptr<ReturnStmt> Parser::parseReturnStmt() {
    consume(TokenType::Return, "Expected 'return'");
    std::unique_ptr<Expr> expr = nullptr;
    if (peek().type != TokenType::Semicolon) {
        expr = parseExpression();
    }
    consume(TokenType::Semicolon, "Expected ';'");
    return std::make_unique<ReturnStmt>(std::move(expr));
}

std::unique_ptr<BreakStmt> Parser::parseBreakStmt() {
    consume(TokenType::Break, "Expected 'break'");
    consume(TokenType::Semicolon, "Expected ';'");
    return std::make_unique<BreakStmt>();
}

std::unique_ptr<ContinueStmt> Parser::parseContinueStmt() {
    consume(TokenType::Continue, "Expected 'continue'");
    consume(TokenType::Semicolon, "Expected ';'");
    return std::make_unique<ContinueStmt>();
}

std::unique_ptr<DoWhileStmt> Parser::parseDoWhileStmt() {
    consume(TokenType::Do, "Expected 'do'");
    auto body = parseBlock();
    consume(TokenType::While, "Expected 'while'");
    consume(TokenType::LParen, "Expected '('");
    auto condition = parseExpression();
    consume(TokenType::RParen, "Expected ')'");
    consume(TokenType::Semicolon, "Expected ';'");
    return std::make_unique<DoWhileStmt>(std::move(body), std::move(condition));
}

std::unique_ptr<ForStmt> Parser::parseForStmt() {
    consume(TokenType::For, "Expected 'for'");
    consume(TokenType::LParen, "Expected '('");
    
    std::unique_ptr<Stmt> init = nullptr;
    if (!match(TokenType::Semicolon)) {
        if (peek().type == TokenType::Let) {
            init = parseVarDecl(); // parseVarDecl consumes semicolon
        } else {
            auto expr = parseExpression();
            consume(TokenType::Semicolon, "Expected ';'");
            init = std::make_unique<ExprStmt>(std::move(expr));
        }
    }

    std::unique_ptr<Expr> condition = nullptr;
    if (!match(TokenType::Semicolon)) {
        condition = parseExpression();
        consume(TokenType::Semicolon, "Expected ';'");
    }

    std::unique_ptr<Expr> step = nullptr;
    if (!match(TokenType::RParen)) {
        step = parseExpression();
        consume(TokenType::RParen, "Expected ')'");
    }

    auto body = parseBlock();
    return std::make_unique<ForStmt>(std::move(init), std::move(condition), std::move(step), std::move(body));
}

std::unique_ptr<SwitchStmt> Parser::parseSwitchStmt() {
    consume(TokenType::Switch, "Expected 'switch'");
    consume(TokenType::LParen, "Expected '('");
    auto condition = parseExpression();
    consume(TokenType::RParen, "Expected ')'");
    consume(TokenType::LBrace, "Expected '{'");
    
    std::vector<std::unique_ptr<CaseStmt>> cases;
    while (!match(TokenType::RBrace)) {
        cases.push_back(parseCaseStmt());
    }
    return std::make_unique<SwitchStmt>(std::move(condition), std::move(cases));
}

std::unique_ptr<CaseStmt> Parser::parseCaseStmt() {
    std::unique_ptr<Pattern> pattern = nullptr;
    bool isDefault = false;
    
    if (match(TokenType::Case)) {
        pattern = parsePattern();
    } else if (match(TokenType::Default)) {
        isDefault = true;
    } else {
        throw std::runtime_error("Expected 'case' or 'default'");
    }
    
    consume(TokenType::Colon, "Expected ':'");
    
    std::vector<std::unique_ptr<Stmt>> stmts;
    while (peek().type != TokenType::Case && 
           peek().type != TokenType::Default && 
           peek().type != TokenType::RBrace &&
           peek().type != TokenType::EndOfFile) {
        stmts.push_back(parseStatement());
    }
    
    auto body = std::make_unique<Block>(std::move(stmts));
    return std::make_unique<CaseStmt>(std::move(pattern), std::move(body), isDefault);
}

std::unique_ptr<Pattern> Parser::parsePattern() {
    if (match(TokenType::Underscore)) {
        return std::make_unique<WildcardPattern>();
    }

    if (peek().type == TokenType::Integer || peek().type == TokenType::Float || peek().type == TokenType::String || peek().type == TokenType::True || peek().type == TokenType::False) {
        auto expr = parsePrimary();
        auto lit = std::unique_ptr<LiteralExpr>(static_cast<LiteralExpr*>(expr.release()));
        return std::make_unique<LiteralPattern>(std::move(lit));
    }

    if (peek().type == TokenType::Identifier) {
        std::string first = std::string(advance().value);
        
        // Handle generic specialization in pattern: Enum[T]::Variant
        if (peek().type == TokenType::LBracket && isGenericContext()) {
            match(TokenType::LBracket);
            first += "[";
            while (true) {
                auto t = parseType();
                first += t->toString();
                if (match(TokenType::Comma)) first += ", ";
                else break;
            }
            consume(TokenType::RBracket, "Expected ']' after generic arguments");
            first += "]";
        }

        // Enum::Variant or Binding
        if (match(TokenType::ColonColon)) {
            std::string variant = std::string(consume(TokenType::Identifier, "Expected variant name").value);
            std::vector<std::unique_ptr<Pattern>> subPatterns;
            if (match(TokenType::LParen)) {
                if (!match(TokenType::RParen)) {
                    do {
                        subPatterns.push_back(parsePattern());
                    } while (match(TokenType::Comma));
                    consume(TokenType::RParen, "Expected ')'");
                }
            } else if (match(TokenType::LBrace)) {
                if (!match(TokenType::RBrace)) {
                    do {
                        subPatterns.push_back(parsePattern());
                    } while (match(TokenType::Comma));
                    consume(TokenType::RBrace, "Expected '}'");
                }
            }
            return std::make_unique<VariantPattern>(first, variant, std::move(subPatterns));
        } else {
            if (match(TokenType::LParen)) {
                std::vector<std::unique_ptr<Pattern>> subPatterns;
                if (!match(TokenType::RParen)) {
                    do {
                        subPatterns.push_back(parsePattern());
                    } while (match(TokenType::Comma));
                    consume(TokenType::RParen, "Expected ')'");
                }
                return std::make_unique<VariantPattern>("", first, std::move(subPatterns));
            } else if (match(TokenType::LBrace)) {
                std::vector<std::unique_ptr<Pattern>> subPatterns;
                if (!match(TokenType::RBrace)) {
                    do {
                        subPatterns.push_back(parsePattern());
                    } while (match(TokenType::Comma));
                    consume(TokenType::RBrace, "Expected '}'");
                }
                return std::make_unique<VariantPattern>("", first, std::move(subPatterns));
            }
            return std::make_unique<IdentifierPattern>(first);
        }
    }

    throw std::runtime_error("Unexpected token in pattern: " + std::string(peek().value));
}

int Parser::getPrecedence(TokenType type) {
    switch (type) {
        case TokenType::Equal:
            return 5;
        case TokenType::OrOr:
            return 8;
        case TokenType::AndAnd:
            return 10;
        case TokenType::Pipe:
            return 15;
        case TokenType::Caret:
            return 20;
        case TokenType::Ampersand:
            return 25;
        case TokenType::EqualEqual:
        case TokenType::NotEqual:
            return 30;
        case TokenType::Less:
        case TokenType::LessEqual:
        case TokenType::Greater:
        case TokenType::GreaterEqual:
            return 35;
        case TokenType::ShiftLeft:
        case TokenType::ShiftRight:
            return 38;
        case TokenType::Plus:
        case TokenType::Minus:
            return 40;
        case TokenType::Star:
        case TokenType::Slash:
        case TokenType::Percent:
            return 50;
        default:
            return -1;
    }
}

std::unique_ptr<Expr> Parser::parseExpression(int minPrec) {
    auto left = parseUnary();

    while (true) {
        int prec = getPrecedence(m_currentToken.type);
        if (prec < minPrec) break;

        TokenType op = m_currentToken.type;
        advance();
        auto right = parseExpression(prec + 1);
        left = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
    }

    return left;
}

std::unique_ptr<Expr> Parser::parseUnary() {
    if (match(TokenType::Ampersand)) {
        return std::make_unique<AddressOfExpr>(parseUnary());
    }
    if (match(TokenType::Star)) {
        return std::make_unique<DereferenceExpr>(parseUnary());
    }
    if (match(TokenType::Minus)) {
        return std::make_unique<UnaryExpr>(TokenType::Minus, parseUnary());
    }
    if (match(TokenType::Not)) {
        return std::make_unique<UnaryExpr>(TokenType::Not, parseUnary());
    }
    if (match(TokenType::Tilde)) {
        return std::make_unique<UnaryExpr>(TokenType::Tilde, parseUnary());
    }
    return parsePostfix();
}

std::unique_ptr<Expr> Parser::parsePostfix() {
    auto left = parsePrimary();

    while (true) {
        if (peek().type == TokenType::Dot || peek().type == TokenType::ColonColon) {
            bool isStatic = (peek().type == TokenType::ColonColon);
            advance();
            Token memberToken = consume(TokenType::Identifier, "Expected member name after selector");
            std::string memberName(memberToken.value);
            
            auto memberAccess = std::make_unique<MemberAccessExpr>(std::move(left), memberName, isStatic);
            if (match(TokenType::LBrace)) {
                left = parseStructLiteral(std::move(memberAccess));
            } else {
                left = std::move(memberAccess);
            }
        } else if (match(TokenType::LParen)) {
            std::vector<std::unique_ptr<Expr>> args;
            if (!match(TokenType::RParen)) {
                do {
                    args.push_back(parseExpression());
                } while (match(TokenType::Comma));
                consume(TokenType::RParen, "Expected ')'");
            }
            left = std::make_unique<CallExpr>(std::move(left), std::move(args));
        } else if (peek().type == TokenType::LBracket) {
            if (isGenericContext()) {
                match(TokenType::LBracket);
                // It's a generic specialization
                std::vector<std::shared_ptr<Type>> typeArgs;
                if (!match(TokenType::RBracket)) {
                    do {
                        typeArgs.push_back(parseType());
                    } while (match(TokenType::Comma));
                    consume(TokenType::RBracket, "Expected ']' after generic arguments");
                }
                left = std::make_unique<SpecializationExpr>(std::move(left), std::move(typeArgs));
            } else {
                match(TokenType::LBracket);
                // It's indexing
                auto index = parseExpression();
                consume(TokenType::RBracket, "Expected ']' after index");
                left = std::make_unique<IndexingExpr>(std::move(left), std::move(index));
            }
        } else if (match(TokenType::Question)) {
            left = std::make_unique<QuestionExpr>(std::move(left));
        } else if (peek().type == TokenType::LBrace) {
            auto kind = left->getKind();
            if (kind == ASTNodeKind::IdentifierExpr || 
                kind == ASTNodeKind::MemberAccessExpr || 
                kind == ASTNodeKind::SpecializationExpr) {
                advance();
                left = parseStructLiteral(std::move(left));
            } else {
                break;
            }
        } else {
            break;
        }
    }
    return left;
}

static std::string unescapeString(std::string_view s) {
    std::string res;
    for (size_t i = 0; i < s.length(); ++i) {
        if (s[i] == '\\' && i + 1 < s.length()) {
            switch (s[++i]) {
                case 'n': res += '\n'; break;
                case 'r': res += '\r'; break;
                case 't': res += '\t'; break;
                case '\\': res += '\\'; break;
                case '"': res += '"'; break;
                default: res += s[i]; break;
            }
        } else {
            res += s[i];
        }
    }
    return res;
}

std::unique_ptr<Expr> Parser::parsePrimary() {
    if (match(TokenType::LParen)) {
        auto expr = parseExpression();
        consume(TokenType::RParen, "Expected ')'");
        return expr;
    }
    if (m_currentToken.type == TokenType::Integer) {
        std::string s(m_currentToken.value);
        size_t underscore = s.find('_');
        std::shared_ptr<Type> explicitType = nullptr;
        if (underscore != std::string::npos) {
            std::string suffix = s.substr(underscore + 1);
            if (suffix == "i8") explicitType = Type::getI8();
            else if (suffix == "i16") explicitType = Type::getI16();
            else if (suffix == "i32") explicitType = Type::getI32();
            else if (suffix == "i64") explicitType = Type::getI64();
            else if (suffix == "u8") explicitType = Type::getU8();
            else if (suffix == "u16") explicitType = Type::getU16();
            else if (suffix == "u32") explicitType = Type::getU32();
            else if (suffix == "u64") explicitType = Type::getU64();
        }
        int64_t val = std::stoll(s);
        advance();
        return std::make_unique<LiteralExpr>(val, explicitType);
    }
    if (m_currentToken.type == TokenType::Float) {
        std::string s(m_currentToken.value);
        size_t underscore = s.find('_');
        std::shared_ptr<Type> explicitType = nullptr;
        if (underscore != std::string::npos) {
            std::string suffix = s.substr(underscore + 1);
            if (suffix == "f32") explicitType = Type::getF32();
            else if (suffix == "f64") explicitType = Type::getF64();
        }
        double val = std::stod(s);
        advance();
        return std::make_unique<LiteralExpr>(val, explicitType);
    }
    if (m_currentToken.type == TokenType::String) {
        std::string val = std::string(m_currentToken.value);
        // Strip quotes
        if (val.length() >= 2 && val.front() == '"' && val.back() == '"') {
            val = val.substr(1, val.length() - 2);
        }
        val = unescapeString(val);
        advance();
        return std::make_unique<LiteralExpr>(val);
    }
    if (match(TokenType::True)) return std::make_unique<LiteralExpr>(true);
    if (match(TokenType::False)) return std::make_unique<LiteralExpr>(false);
    if (match(TokenType::Nullptr)) return std::make_unique<LiteralExpr>(nullptr);
    
    if (match(TokenType::LBracket)) {
        std::vector<std::unique_ptr<Expr>> elements;
        if (!match(TokenType::RBracket)) {
            do {
                elements.push_back(parseExpression());
            } while (match(TokenType::Comma));
            consume(TokenType::RBracket, "Expected ']' after array literal");
        }
        return std::make_unique<ArrayLiteralExpr>(std::move(elements));
    }

    if (peek().type == TokenType::Sizeof || 
        peek().type == TokenType::Malloc ||
        peek().type == TokenType::Alloca ||
        peek().type == TokenType::Free ||
        peek().type == TokenType::Alignof ||
        peek().type == TokenType::Offsetof) {
        
        IntrinsicExpr::IntrinsicKind kind;
        auto tokType = advance().type;
        if (tokType == TokenType::Sizeof) kind = IntrinsicExpr::IntrinsicKind::Sizeof;
        else if (tokType == TokenType::Malloc) kind = IntrinsicExpr::IntrinsicKind::Malloc;
        else if (tokType == TokenType::Alloca) kind = IntrinsicExpr::IntrinsicKind::Alloca;
        else if (tokType == TokenType::Free) kind = IntrinsicExpr::IntrinsicKind::Free;
        else if (tokType == TokenType::Alignof) kind = IntrinsicExpr::IntrinsicKind::Alignof;
        else kind = IntrinsicExpr::IntrinsicKind::Offsetof;

        std::shared_ptr<Type> typeArg = nullptr;
        if (match(TokenType::LBracket)) {
            typeArg = parseType();
            consume(TokenType::RBracket, "Expected ']' after type argument");
        }

        consume(TokenType::LParen, "Expected '('");
        std::vector<std::unique_ptr<Expr>> args;
        if (!match(TokenType::RParen)) {
            do {
                if (kind == IntrinsicExpr::IntrinsicKind::Offsetof && args.empty()) {
                    // Special case for offsetof member name
                    auto memberTok = consume(TokenType::Identifier, "Expected member name in offsetof");
                    args.push_back(std::make_unique<IdentifierExpr>(std::string(memberTok.value)));
                } else {
                    args.push_back(parseExpression());
                }
            } while (match(TokenType::Comma));
            consume(TokenType::RParen, "Expected ')'");
        }
        return std::make_unique<IntrinsicExpr>(kind, typeArg, std::move(args));
    }

    if (m_currentToken.type == TokenType::Identifier || m_currentToken.type == TokenType::Self) {
        std::string name(m_currentToken.value);
        advance();
        
        auto base = std::make_unique<IdentifierExpr>(name);
        if (match(TokenType::LBrace)) {
            return parseStructLiteral(std::move(base));
        }
        
        return base;
    }

        throw std::runtime_error("Expected expression at line " + std::to_string(m_currentToken.line));       
    }
    
    std::unique_ptr<ImportDecl> Parser::parseImportDecl() {
        consume(TokenType::Import, "Expected 'import'");
        std::string path;
        bool isStd = false;
        
                if (peek().type == TokenType::String) {
        
                    path = std::string(peek().value);
        
                    if (path.size() >= 2 && path.front() == '"' && path.back() == '"') {
        
                        path = path.substr(1, path.size() - 2);
        
                    }
        
                    advance();
        
                } else {            // Parse qualified name: std::vec
            isStd = true;
            path = std::string(consume(TokenType::Identifier, "Expected identifier").value);
            while (match(TokenType::ColonColon)) {
                path += "::" + std::string(consume(TokenType::Identifier, "Expected identifier").value);
            }
        }
        
        std::string alias;
        if (match(TokenType::As)) {
            alias = std::string(consume(TokenType::Identifier, "Expected alias").value);
        }
        
        consume(TokenType::Semicolon, "Expected ';'");
        return std::make_unique<ImportDecl>(path, isStd, alias);
    }
    
    std::unique_ptr<PackageDecl> Parser::parsePackageDecl() {
        consume(TokenType::Package, "Expected 'package'");
        std::string name = std::string(consume(TokenType::Identifier, "Expected package name").value);
        while (match(TokenType::ColonColon)) {
            name += "::" + std::string(consume(TokenType::Identifier, "Expected identifier").value);
        }
        consume(TokenType::Semicolon, "Expected ';'");
        return std::make_unique<PackageDecl>(name);
    }
    
    std::unique_ptr<UseDecl> Parser::parseUseDecl() {
        consume(TokenType::Use, "Expected 'use'");
        std::string path = std::string(consume(TokenType::Identifier, "Expected identifier").value);
        while (match(TokenType::ColonColon)) {
            path += "::" + std::string(consume(TokenType::Identifier, "Expected identifier").value);
        }
        
        std::string alias;
        if (match(TokenType::As)) {
            alias = std::string(consume(TokenType::Identifier, "Expected alias").value);
        }
        
            consume(TokenType::Semicolon, "Expected ';'");
            return std::make_unique<UseDecl>(path, alias);
        }
        
        bool Parser::isGenericContext() {
    Token next = m_lexer.peekToken();
    if (next.type == TokenType::I8 || next.type == TokenType::I16 || next.type == TokenType::I32 || next.type == TokenType::I64 ||
        next.type == TokenType::U8 || next.type == TokenType::U16 || next.type == TokenType::U32 || next.type == TokenType::U64 ||
        next.type == TokenType::F32 || next.type == TokenType::F64 || next.type == TokenType::Bool || next.type == TokenType::Void ||
        next.type == TokenType::Identifier || next.type == TokenType::Star || next.type == TokenType::Ampersand) {
        return true;
    }
    return false;
}

std::vector<GenericParam> Parser::parseGenericParams() {
    std::vector<GenericParam> params;
    if (match(TokenType::LBracket)) {
        if (!match(TokenType::RBracket)) {
            do {
                std::string name(consume(TokenType::Identifier, "Expected generic parameter name").value);
                std::unique_ptr<ConstraintExpr> constraint = nullptr;
                if (match(TokenType::Question)) {
                    constraint = parseConstraintExpr();
                }
                params.emplace_back(name, std::move(constraint));
                m_activeGenericParams.back().insert(name);
            } while (match(TokenType::Comma));
            consume(TokenType::RBracket, "Expected ']' after generic parameters");
        }
    }
    return params;
}

std::unique_ptr<ConstraintExpr> Parser::parseConstraintExpr() {
    std::vector<ConstraintExpr::ConstraintItem> items;
    do {
        ConstraintExpr::Logic logic = ConstraintExpr::Logic::NONE;
        if (!items.empty()) {
            if (match(TokenType::AndAnd)) logic = ConstraintExpr::Logic::AND;
            else if (match(TokenType::OrOr)) logic = ConstraintExpr::Logic::OR;
            else break;
        }
        std::string traitName(consume(TokenType::Identifier, "Expected trait name in constraint").value);
        items.push_back({traitName, logic});
    } while (peek().type == TokenType::AndAnd || peek().type == TokenType::OrOr);
    return std::make_unique<ConstraintExpr>(std::move(items));
}

std::unique_ptr<RequestDecl> Parser::parseRequestDecl(bool isPublic) {
    consume(TokenType::Identifier, "Expected 'request'");
    RequestDecl::RequestKind kind;
    if (match(TokenType::Class)) kind = RequestDecl::RequestKind::Class;
    else if (match(TokenType::Enum)) {
        consume(TokenType::Enum, "Expected 'class' or 'enum' after 'request'");
        kind = RequestDecl::RequestKind::Enum;
    } else {
        throw std::runtime_error("Expected 'class' or 'enum' after 'request'");
    }

    std::string name(consume(TokenType::Identifier, "Expected request name").value);
    
    m_activeGenericParams.push_back({});
    auto genericParams = parseGenericParams();

    std::vector<std::string> bases;
    if (match(TokenType::Colon)) {
        do {
            bases.push_back(std::string(consume(TokenType::Identifier, "Expected base request name").value));
        } while (match(TokenType::Comma));
    }

    consume(TokenType::LBrace, "Expected '{' in request body");
    std::vector<RequestDecl::Member> members;
    while (!match(TokenType::RBrace)) {
        bool isDefault = match(TokenType::Default);
        bool memberPublic = match(TokenType::Pub);
        
        std::unique_ptr<ASTNode> decl;
        if (peek().type == TokenType::Let) {
            decl = parseVarDecl(memberPublic);
        } else if (peek().type == TokenType::Fn) {
            decl = parseMethodDecl(memberPublic);
        } else if (kind == RequestDecl::RequestKind::Enum && peek().type == TokenType::Identifier) {
            // Enum variant in request enum
            std::string vName(consume(TokenType::Identifier, "Expected variant name").value);
            EnumVariant::VariantKind vKind = EnumVariant::VariantKind::Unit;
            std::vector<std::shared_ptr<Type>> tupleTypes;
            std::vector<std::unique_ptr<VarDecl>> structFields;

            if (match(TokenType::LParen)) {
                vKind = EnumVariant::VariantKind::Tuple;
                if (!match(TokenType::RParen)) {
                    do {
                        tupleTypes.push_back(parseType());
                    } while (match(TokenType::Comma));
                    consume(TokenType::RParen, "Expected ')' after tuple variant types");
                }
            } else if (match(TokenType::LBrace)) {
                vKind = EnumVariant::VariantKind::Struct;
                while (!match(TokenType::RBrace)) {
                    structFields.push_back(parseStructField());
                    match(TokenType::Semicolon); // Optional semicolon
                }
            }
            decl = std::make_unique<EnumVariant>(vName, vKind, std::move(tupleTypes), std::move(structFields), isDefault);
        } else {
            throw std::runtime_error("Unexpected token in request body");
        }
        members.push_back({std::move(decl), isDefault});
    }
    m_activeGenericParams.pop_back();
    return std::make_unique<RequestDecl>(name, kind, std::move(members), bases, std::move(genericParams), isPublic);
}

} // namespace chtholly