#ifndef CHTHOLLY_TYPES_H
#define CHTHOLLY_TYPES_H

#include <string>
#include <memory>
#include <vector>
#include <map>

namespace chtholly {

enum class TypeKind {
    I8,
    I16,
    I32,
    I64,
    U8,
    U16,
    U32,
    U64,
    F32,
    F64,
    Bool,
    Void,
    Function,
    Pointer,
    Array,
    Struct,
    Enum,
    TypeParameter
};

class Type : public std::enable_shared_from_this<Type> {
public:
    virtual ~Type() = default;

    virtual TypeKind getKind() const = 0;
    virtual std::string toString() const = 0;
    virtual std::shared_ptr<Type> substitute(const std::map<std::string, std::shared_ptr<Type>>& mapping) const = 0;

    virtual bool equals(const Type& other) const {
        if (getKind() != other.getKind()) return false;
        return toString() == other.toString(); // Fallback for complex types
    }

    bool operator==(const Type& other) const { return equals(other); }
    bool operator!=(const Type& other) const { return !equals(other); }

    bool isI8() const { return getKind() == TypeKind::I8; }
    bool isI16() const { return getKind() == TypeKind::I16; }
    bool isI32() const { return getKind() == TypeKind::I32; }
    bool isI64() const { return getKind() == TypeKind::I64; }
    bool isU8() const { return getKind() == TypeKind::U8; }
    bool isU16() const { return getKind() == TypeKind::U16; }
    bool isU32() const { return getKind() == TypeKind::U32; }
    bool isU64() const { return getKind() == TypeKind::U64; }
    bool isUnsigned() const { 
        return getKind() == TypeKind::U8 || getKind() == TypeKind::U16 || 
               getKind() == TypeKind::U32 || getKind() == TypeKind::U64; 
    }
    bool isInteger() const { 
        return isI8() || isI16() || isI32() || isI64() || 
               isU8() || isU16() || isU32() || isU64(); 
    }
    bool isFloatingPoint() const { return getKind() == TypeKind::F32 || getKind() == TypeKind::F64; }
    bool isFloat() const { return isFloatingPoint(); }
    bool isBoolean() const { return getKind() == TypeKind::Bool; }
    bool isBool() const { return isBoolean(); }
    bool isVoid() const { return getKind() == TypeKind::Void; }
    bool isFunction() const { return getKind() == TypeKind::Function; }
    bool isPointer() const { return getKind() == TypeKind::Pointer; }
    bool isArray() const { return getKind() == TypeKind::Array; }
    bool isStruct() const { return getKind() == TypeKind::Struct; }
    bool isClass() const { return getKind() == TypeKind::Struct && m_isClassInternal; }
    bool isEnum() const { return getKind() == TypeKind::Enum; }
    bool isTypeParameter() const { return getKind() == TypeKind::TypeParameter; }

    void setInternalIsClass(bool val) { m_isClassInternal = val; }
protected:
    bool m_isClassInternal = false;
public:
    bool isCopyType() const {
        switch (getKind()) {
            case TypeKind::I8:
            case TypeKind::I16:
            case TypeKind::I32:
            case TypeKind::I64:
            case TypeKind::U8:
            case TypeKind::U16:
            case TypeKind::U32:
            case TypeKind::U64:
            case TypeKind::F32:
            case TypeKind::F64:
            case TypeKind::Bool:
            case TypeKind::Pointer:
                return true;
            default:
                return false;
        }
    }

    static std::shared_ptr<Type> getI8();
    static std::shared_ptr<Type> getI16();
    static std::shared_ptr<Type> getI32();
    static std::shared_ptr<Type> getI64();
    static std::shared_ptr<Type> getU8();
    static std::shared_ptr<Type> getU16();
    static std::shared_ptr<Type> getU32();
    static std::shared_ptr<Type> getU64();
    static std::shared_ptr<Type> getF32();
    static std::shared_ptr<Type> getF64();
    static std::shared_ptr<Type> getBool();
    static std::shared_ptr<Type> getVoid();
    static std::shared_ptr<Type> getI8Ptr();
};

class PrimitiveType : public Type {
public:
    PrimitiveType(TypeKind kind) : kind(kind) {}

    TypeKind getKind() const override { return kind; }
    std::string toString() const override {
        switch (kind) {
            case TypeKind::I8: return "i8";
            case TypeKind::I16: return "i16";
            case TypeKind::I32: return "i32";
            case TypeKind::I64: return "i64";
            case TypeKind::U8: return "u8";
            case TypeKind::U16: return "u16";
            case TypeKind::U32: return "u32";
            case TypeKind::U64: return "u64";
            case TypeKind::F32: return "f32";
            case TypeKind::F64: return "f64";
            case TypeKind::Bool: return "bool";
            case TypeKind::Void: return "void";
            default: return "unknown";
        }
    }
    std::shared_ptr<Type> substitute(const std::map<std::string, std::shared_ptr<Type>>&) const override {
        return std::const_pointer_cast<Type>(shared_from_this());
    }

private:
    TypeKind kind;
};

class PointerType : public Type {
public:
    PointerType(std::shared_ptr<Type> baseType) : baseType(baseType) {}

    TypeKind getKind() const override { return TypeKind::Pointer; }
    std::string toString() const override {
        return baseType->toString() + "*";
    }
    std::shared_ptr<Type> substitute(const std::map<std::string, std::shared_ptr<Type>>& mapping) const override {
        return std::make_shared<PointerType>(baseType->substitute(mapping));
    }

    bool equals(const Type& other) const override {
        if (other.getKind() != TypeKind::Pointer) return false;
        return baseType->equals(*static_cast<const PointerType&>(other).baseType);
    }

    std::shared_ptr<Type> getBaseType() const { return baseType; }

private:
    std::shared_ptr<Type> baseType;
};

class ArrayType : public Type {
public:
    ArrayType(std::shared_ptr<Type> baseType, int size)
        : baseType(baseType), size(size) {}

    TypeKind getKind() const override { return TypeKind::Array; }
    std::string toString() const override {
        return baseType->toString() + "[" + std::to_string(size) + "]";
    }
    std::shared_ptr<Type> substitute(const std::map<std::string, std::shared_ptr<Type>>& mapping) const override {
        return std::make_shared<ArrayType>(baseType->substitute(mapping), size);
    }

    bool equals(const Type& other) const override {
        if (other.getKind() != TypeKind::Array) return false;
        auto& o = static_cast<const ArrayType&>(other);
        return size == o.size && baseType->equals(*o.baseType);
    }

    std::shared_ptr<Type> getBaseType() const { return baseType; }
    int getSize() const { return size; }

private:
    std::shared_ptr<Type> baseType;
    int size;
};

class FunctionType : public Type {
public:
    FunctionType(std::vector<std::shared_ptr<Type>> params, std::shared_ptr<Type> returnType, bool isVariadic = false)
        : params(std::move(params)), returnType(returnType), isVariadic(isVariadic) {}

    TypeKind getKind() const override { return TypeKind::Function; }
    std::string toString() const override {
        std::string result = "(";
        for (size_t i = 0; i < params.size(); ++i) {
            result += params[i]->toString();
            if (i < params.size() - 1) result += ", ";
        }
        if (isVariadic) {
            if (!params.empty()) result += ", ";
            result += "...";
        }
        result += "): " + returnType->toString();
        return result;
    }
    std::shared_ptr<Type> substitute(const std::map<std::string, std::shared_ptr<Type>>& mapping) const override {
        std::vector<std::shared_ptr<Type>> newParams;
        for (const auto& p : params) newParams.push_back(p->substitute(mapping));
        return std::make_shared<FunctionType>(std::move(newParams), returnType->substitute(mapping), isVariadic);
    }

    bool equals(const Type& other) const override {
        if (other.getKind() != TypeKind::Function) return false;
        auto& o = static_cast<const FunctionType&>(other);
        if (isVariadic != o.isVariadic || params.size() != o.params.size()) return false;
        if (!returnType->equals(*o.returnType)) return false;
        for (size_t i = 0; i < params.size(); ++i) {
            if (!params[i]->equals(*o.params[i])) return false;
        }
        return true;
    }

    const std::vector<std::shared_ptr<Type>>& getParamTypes() const { return params; }
    std::shared_ptr<Type> getReturnType() const { return returnType; }
    bool isVarArg() const { return isVariadic; }

private:
    std::vector<std::shared_ptr<Type>> params;
    std::shared_ptr<Type> returnType;
    bool isVariadic;
};

class StructType;
using ClassType = StructType;

class StructType : public Type {
public:
    struct Field {
        std::string name;
        std::shared_ptr<Type> type;
        bool isPublic;
    };

    struct Method {
        std::string name;
        std::shared_ptr<Type> type;
        bool isPublic;
    };

    StructType(std::string name, std::vector<Field> fields, std::vector<Method> methods = {})
        : name(std::move(name)), fields(std::move(fields)), methods(std::move(methods)) {}

    TypeKind getKind() const override { return TypeKind::Struct; }
    std::string toString() const override { return name; }
    std::shared_ptr<Type> substitute(const std::map<std::string, std::shared_ptr<Type>>& mapping) const override {
        if (mapping.count(name)) return mapping.at(name);
        // Struct monomorphization is complex because it creates a new StructType
        // For now, if no mapping affects our fields, return this.
        // Actually, ASTSubstituter will handle cloning the StructDecl.
        // In Types, we just return self if it's already a concrete struct.
        return std::const_pointer_cast<Type>(shared_from_this());
    }

    const std::string& getName() const { return name; }
    const std::vector<Field>& getFields() const { return fields; }
    const std::vector<Method>& getMethods() const { return methods; }

    void setMethods(std::vector<Method> m) { methods = std::move(m); }

    int findFieldIndex(const std::string& fieldName) const {
        for (int i = 0; i < (int)fields.size(); ++i) {
            if (fields[i].name == fieldName) return i;
        }
        return -1;
    }

    std::shared_ptr<Type> findMethod(const std::string& methodName) const {
        for (const auto& m : methods) {
            if (m.name == methodName) return m.type;
        }
        return nullptr;
    }

private:
    std::string name;
    std::vector<Field> fields;
    std::vector<Method> methods;
};

class EnumType : public Type {
public:
    struct Variant {
        std::string name;
        enum class Kind { Unit, Tuple, Struct } kind;
        std::vector<std::shared_ptr<Type>> tupleTypes;
        std::vector<StructType::Field> structFields;
    };

    EnumType(std::string name, std::vector<Variant> variants)
        : name(std::move(name)), variants(std::move(variants)) {}

    TypeKind getKind() const override { return TypeKind::Enum; }
    std::string toString() const override { return name; }
    std::shared_ptr<Type> substitute(const std::map<std::string, std::shared_ptr<Type>>&) const override {
        return std::const_pointer_cast<Type>(shared_from_this());
    }

    const std::string& getName() const { return name; }
    const std::vector<Variant>& getVariants() const { return variants; }

    const Variant* findVariant(const std::string& variantName) const {
        for (const auto& v : variants) {
            if (v.name == variantName) return &v;
        }
        return nullptr;
    }

    int findVariantIndex(const std::string& variantName) const {
        for (int i = 0; i < (int)variants.size(); ++i) {
            if (variants[i].name == variantName) return i;
        }
        return -1;
    }

private:
    std::string name;
    std::vector<Variant> variants;
};

class TypeParameterType : public Type {
public:
    TypeParameterType(std::string name, std::string constraintName = "") 
        : name(std::move(name)), constraintName(std::move(constraintName)) {}

    TypeKind getKind() const override { return TypeKind::TypeParameter; }
    std::string toString() const override { return name; }
    std::shared_ptr<Type> substitute(const std::map<std::string, std::shared_ptr<Type>>& mapping) const override {
        if (mapping.count(name)) return mapping.at(name);
        return std::const_pointer_cast<Type>(shared_from_this());
    }

    const std::string& getName() const { return name; }
    const std::string& getConstraintName() const { return constraintName; }

private:
    std::string name;
    std::string constraintName;
};

} // namespace chtholly

#endif // CHTHOLLY_TYPES_H
