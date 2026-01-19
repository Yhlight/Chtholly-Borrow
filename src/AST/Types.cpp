#include "AST/Types.h"

namespace chtholly {

std::shared_ptr<Type> Type::getI8() {
    static auto instance = std::make_shared<PrimitiveType>(TypeKind::I8);
    return instance;
}

std::shared_ptr<Type> Type::getI16() {
    static auto instance = std::make_shared<PrimitiveType>(TypeKind::I16);
    return instance;
}

std::shared_ptr<Type> Type::getI32() {
    static auto instance = std::make_shared<PrimitiveType>(TypeKind::I32);
    return instance;
}

std::shared_ptr<Type> Type::getI64() {
    static auto instance = std::make_shared<PrimitiveType>(TypeKind::I64);
    return instance;
}

std::shared_ptr<Type> Type::getU8() {
    static auto instance = std::make_shared<PrimitiveType>(TypeKind::U8);
    return instance;
}

std::shared_ptr<Type> Type::getU16() {
    static auto instance = std::make_shared<PrimitiveType>(TypeKind::U16);
    return instance;
}

std::shared_ptr<Type> Type::getU32() {
    static auto instance = std::make_shared<PrimitiveType>(TypeKind::U32);
    return instance;
}

std::shared_ptr<Type> Type::getU64() {
    static auto instance = std::make_shared<PrimitiveType>(TypeKind::U64);
    return instance;
}

std::shared_ptr<Type> Type::getF32() {
    static auto instance = std::make_shared<PrimitiveType>(TypeKind::F32);
    return instance;
}

std::shared_ptr<Type> Type::getF64() {
    static auto instance = std::make_shared<PrimitiveType>(TypeKind::F64);
    return instance;
}

std::shared_ptr<Type> Type::getBool() {
    static auto instance = std::make_shared<PrimitiveType>(TypeKind::Bool);
    return instance;
}

std::shared_ptr<Type> Type::getVoid() {
    static auto instance = std::make_shared<PrimitiveType>(TypeKind::Void);
    return instance;
}

std::shared_ptr<Type> Type::getI8Ptr() {
    static auto instance = std::make_shared<PointerType>(getI8());
    return instance;
}

} // namespace chtholly
