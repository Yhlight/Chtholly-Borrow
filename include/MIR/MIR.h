#ifndef CHTHOLLY_MIR_H
#define CHTHOLLY_MIR_H

#include <string>
#include <vector>
#include <memory>
#include "AST/Types.h"
#include "Lexer/Token.h"

namespace chtholly {

enum class MIRInstructionKind {
    Alloca,
    ConstInt,
    ConstBool,
    ConstString,
    ConstDouble,
    UnaryOp,
    BinOp,
    Store,
    Load,
    StructElementPtr,
    ArrayElementPtr,
    Sizeof,
    Alignof,
    Offsetof,
    VariantTag,
    VariantData,
    VariantExtract,
    Ret,
    Call,
    Br,
    CondBr
};

class MIRInstruction {
public:
    virtual ~MIRInstruction() = default;
    virtual std::string toString() const = 0;
    virtual MIRInstructionKind getKind() const = 0;
};

class AllocaInst : public MIRInstruction {
public:
    AllocaInst(std::string name, std::shared_ptr<Type> type)
        : name(std::move(name)), type(type) {}

    MIRInstructionKind getKind() const override { return MIRInstructionKind::Alloca; }
    std::string toString() const override {
        return name + " = alloca " + type->toString();
    }

    const std::string& getName() const { return name; }
    std::shared_ptr<Type> getType() const { return type; }

private:
    std::string name;
    std::shared_ptr<Type> type;
};

class ConstIntInst : public MIRInstruction {
public:
    ConstIntInst(std::string dest, int64_t value)
        : dest(std::move(dest)), value(value) {}

    MIRInstructionKind getKind() const override { return MIRInstructionKind::ConstInt; }
    std::string toString() const override {
        return dest + " = const " + std::to_string(value);
    }

    const std::string& getDest() const { return dest; }
    int64_t getValue() const { return value; }

private:
    std::string dest;
    int64_t value;
};

class ConstBoolInst : public MIRInstruction {
public:
    ConstBoolInst(std::string dest, bool value)
        : dest(std::move(dest)), value(value) {}

    MIRInstructionKind getKind() const override { return MIRInstructionKind::ConstBool; }
    std::string toString() const override {
        return dest + " = const " + (value ? "true" : "false");
    }

    const std::string& getDest() const { return dest; }
    bool getValue() const { return value; }

private:
    std::string dest;
    bool value;
};

class ConstStringInst : public MIRInstruction {
public:
    ConstStringInst(std::string dest, std::string value)
        : dest(std::move(dest)), value(std::move(value)) {}

    MIRInstructionKind getKind() const override { return MIRInstructionKind::ConstString; }
    std::string toString() const override {
        return dest + " = const \"" + value + "\"";
    }

    const std::string& getDest() const { return dest; }
    const std::string& getValue() const { return value; }

private:
    std::string dest;
    std::string value;
};

class ConstDoubleInst : public MIRInstruction {
public:
    ConstDoubleInst(std::string dest, double value)
        : dest(std::move(dest)), value(value) {}

    MIRInstructionKind getKind() const override { return MIRInstructionKind::ConstDouble; }
    std::string toString() const override {
        return dest + " = const " + std::to_string(value);
    }

    const std::string& getDest() const { return dest; }
    double getValue() const { return value; }

private:
    std::string dest;
    double value;
};

class UnaryOpInst : public MIRInstruction {
public:
    UnaryOpInst(std::string dest, std::string operand, TokenType op)
        : dest(std::move(dest)), operand(std::move(operand)), op(op) {}

    MIRInstructionKind getKind() const override { return MIRInstructionKind::UnaryOp; }
    std::string toString() const override {
        return dest + " = unaryop " + std::to_string((int)op) + " " + operand;
    }

    const std::string& getDest() const { return dest; }
    const std::string& getOperand() const { return operand; }
    TokenType getOp() const { return op; }

private:
    std::string dest;
    std::string operand;
    TokenType op;
};

class BinOpInst : public MIRInstruction {
public:
    BinOpInst(std::string dest, std::string left, std::string right, TokenType op)
        : dest(std::move(dest)), left(std::move(left)), right(std::move(right)), op(op) {}

    MIRInstructionKind getKind() const override { return MIRInstructionKind::BinOp; }
    std::string toString() const override {
        return dest + " = binop " + std::to_string((int)op) + " " + left + ", " + right;
    }

    const std::string& getDest() const { return dest; }
    const std::string& getLeft() const { return left; }
    const std::string& getRight() const { return right; }
    TokenType getOp() const { return op; }

private:
    std::string dest;
    std::string left;
    std::string right;
    TokenType op;
};

class StoreInst : public MIRInstruction {
public:
    StoreInst(std::string src, std::string dest)
        : src(std::move(src)), dest(std::move(dest)) {}

    MIRInstructionKind getKind() const override { return MIRInstructionKind::Store; }
    std::string toString() const override {
        return "store " + src + ", " + dest;
    }

    const std::string& getSrc() const { return src; }
    const std::string& getDest() const { return dest; }

private:
    std::string src;
    std::string dest;
};

class LoadInst : public MIRInstruction {
public:
    LoadInst(std::string dest, std::string src)
        : dest(std::move(dest)), src(std::move(src)) {}

    MIRInstructionKind getKind() const override { return MIRInstructionKind::Load; }
    std::string toString() const override {
        return dest + " = load " + src;
    }

    const std::string& getDest() const { return dest; }
    const std::string& getSrc() const { return src; }

private:
    std::string dest;
    std::string src;
};

class StructElementPtrInst : public MIRInstruction {
public:
    StructElementPtrInst(std::string dest, std::string ptr, std::string structName, std::string fieldName)
        : dest(std::move(dest)), ptr(std::move(ptr)), structName(std::move(structName)), fieldName(std::move(fieldName)) {}

    MIRInstructionKind getKind() const override { return MIRInstructionKind::StructElementPtr; }
    std::string toString() const override {
        return dest + " = struct_gep " + ptr + " (" + structName + "), " + fieldName;
    }

    const std::string& getDest() const { return dest; }
    const std::string& getPtr() const { return ptr; }
    const std::string& getStructName() const { return structName; }
    const std::string& getFieldName() const { return fieldName; }

private:
    std::string dest;
    std::string ptr;
    std::string structName;
    std::string fieldName;
};

class ArrayElementPtrInst : public MIRInstruction {
public:
    ArrayElementPtrInst(std::string dest, std::string ptr, std::string index, std::shared_ptr<Type> elementType)
        : dest(std::move(dest)), ptr(std::move(ptr)), index(std::move(index)), elementType(std::move(elementType)) {}

    MIRInstructionKind getKind() const override { return MIRInstructionKind::ArrayElementPtr; }
    std::string toString() const override {
        return dest + " = array_gep " + ptr + ", " + index + " (" + elementType->toString() + ")";
    }

    const std::string& getDest() const { return dest; }
    const std::string& getPtr() const { return ptr; }
    const std::string& getIndex() const { return index; }
    std::shared_ptr<Type> getElementType() const { return elementType; }

private:
    std::string dest;
    std::string ptr;
    std::string index;
    std::shared_ptr<Type> elementType;
};

class SizeofInst : public MIRInstruction {
public:
    SizeofInst(std::string dest, std::shared_ptr<Type> type)
        : dest(std::move(dest)), type(type) {}

    MIRInstructionKind getKind() const override { return MIRInstructionKind::Sizeof; }
    std::string toString() const override {
        return dest + " = sizeof " + type->toString();
    }

    const std::string& getDest() const { return dest; }
    std::shared_ptr<Type> getType() const { return type; }

private:
    std::string dest;
    std::shared_ptr<Type> type;
};

class AlignofInst : public MIRInstruction {
public:
    AlignofInst(std::string dest, std::shared_ptr<Type> type)
        : dest(std::move(dest)), type(type) {}

    MIRInstructionKind getKind() const override { return MIRInstructionKind::Alignof; }
    std::string toString() const override {
        return dest + " = alignof " + type->toString();
    }

    const std::string& getDest() const { return dest; }
    std::shared_ptr<Type> getType() const { return type; }

private:
    std::string dest;
    std::shared_ptr<Type> type;
};

class OffsetofInst : public MIRInstruction {
public:
    OffsetofInst(std::string dest, std::shared_ptr<Type> type, std::string memberName)
        : dest(std::move(dest)), type(type), memberName(std::move(memberName)) {}

    MIRInstructionKind getKind() const override { return MIRInstructionKind::Offsetof; }
    std::string toString() const override {
        return dest + " = offsetof " + type->toString() + ", " + memberName;
    }

    const std::string& getDest() const { return dest; }
    std::shared_ptr<Type> getType() const { return type; }
    const std::string& getMemberName() const { return memberName; }

private:
    std::string dest;
    std::shared_ptr<Type> type;
    std::string memberName;
};

class VariantTagInst : public MIRInstruction {
public:
    VariantTagInst(std::string dest, std::string enumPtr)
        : dest(std::move(dest)), enumPtr(std::move(enumPtr)) {}

    MIRInstructionKind getKind() const override { return MIRInstructionKind::VariantTag; }
    std::string toString() const override {
        return dest + " = variant_tag " + enumPtr;
    }

    const std::string& getDest() const { return dest; }
    const std::string& getEnumPtr() const { return enumPtr; }

private:
    std::string dest;
    std::string enumPtr;
};

class VariantDataInst : public MIRInstruction {
public:
    VariantDataInst(std::string dest, std::string enumPtr, int tag, std::vector<std::string> args)
        : dest(std::move(dest)), enumPtr(std::move(enumPtr)), tag(tag), args(std::move(args)) {}

    MIRInstructionKind getKind() const override { return MIRInstructionKind::VariantData; }
    std::string toString() const override {
        std::string res = dest + " = variant_data " + enumPtr + ", tag " + std::to_string(tag) + "(";
        for (size_t i = 0; i < args.size(); ++i) {
            res += args[i];
            if (i < args.size() - 1) res += ", ";
        }
        res += ")";
        return res;
    }

    const std::string& getDest() const { return dest; }
    const std::string& getEnumPtr() const { return enumPtr; }
    int getTag() const { return tag; }
    const std::vector<std::string>& getArgs() const { return args; }

private:
    std::string dest;
    std::string enumPtr;
    int tag;
    std::vector<std::string> args;
};

class VariantExtractInst : public MIRInstruction {
public:
    VariantExtractInst(std::string dest, std::string enumPtr, int tag, int fieldIndex, std::shared_ptr<Type> fieldType)
        : dest(std::move(dest)), enumPtr(std::move(enumPtr)), tag(tag), fieldIndex(fieldIndex), fieldType(std::move(fieldType)) {}

    MIRInstructionKind getKind() const override { return MIRInstructionKind::VariantExtract; }
    std::string toString() const override {
        return dest + " = variant_extract " + enumPtr + ", tag " + std::to_string(tag) + ", index " + std::to_string(fieldIndex);
    }

    const std::string& getDest() const { return dest; }
    const std::string& getEnumPtr() const { return enumPtr; }
    int getTag() const { return tag; }
    int getFieldIndex() const { return fieldIndex; }
    std::shared_ptr<Type> getFieldType() const { return fieldType; }

private:
    std::string dest;
    std::string enumPtr;
    int tag;
    int fieldIndex;
    std::shared_ptr<Type> fieldType;
};

class ReturnInst : public MIRInstruction {
public:
    ReturnInst(std::string val = "") : val(std::move(val)) {}

    MIRInstructionKind getKind() const override { return MIRInstructionKind::Ret; }
    std::string toString() const override {
        return "ret " + val;
    }

    const std::string& getVal() const { return val; }

private:
    std::string val;
};

class CallInst : public MIRInstruction {
public:
    CallInst(std::string dest, std::string callee, std::vector<std::string> args)
        : dest(std::move(dest)), callee(std::move(callee)), args(std::move(args)) {}

    MIRInstructionKind getKind() const override { return MIRInstructionKind::Call; }
    std::string toString() const override {
        return dest + " = call " + callee;
    }

    const std::string& getDest() const { return dest; }
    const std::string& getCallee() const { return callee; }
    const std::vector<std::string>& getArgs() const { return args; }

private:
    std::string dest;
    std::string callee;
    std::vector<std::string> args;
};

class BrInst : public MIRInstruction {
public:
    BrInst(std::string target) : target(std::move(target)) {}

    MIRInstructionKind getKind() const override { return MIRInstructionKind::Br; }
    std::string toString() const override {
        return "br label %" + target;
    }

    const std::string& getTarget() const { return target; }

private:
    std::string target;
};

class CondBrInst : public MIRInstruction {
public:
    CondBrInst(std::string cond, std::string thenLabel, std::string elseLabel)
        : cond(std::move(cond)), thenLabel(std::move(thenLabel)), elseLabel(std::move(elseLabel)) {}

    MIRInstructionKind getKind() const override { return MIRInstructionKind::CondBr; }
    std::string toString() const override {
        return "br " + cond + ", label %" + thenLabel + ", label %" + elseLabel;
    }

    const std::string& getCond() const { return cond; }
    const std::string& getThenLabel() const { return thenLabel; }
    const std::string& getElseLabel() const { return elseLabel; }

private:
    std::string cond;
    std::string thenLabel;
    std::string elseLabel;
};

class BasicBlock {
public:
    BasicBlock(std::string name) : name(std::move(name)) {}

    const std::string& getName() const { return name; }
    
    void appendInstruction(std::unique_ptr<MIRInstruction> inst) {
        instructions.push_back(std::move(inst));
    }

    bool hasTerminator() const {
        if (instructions.empty()) return false;
        auto kind = instructions.back()->getKind();
        return kind == MIRInstructionKind::Br || kind == MIRInstructionKind::CondBr || kind == MIRInstructionKind::Ret;
    }

    const std::vector<std::unique_ptr<MIRInstruction>>& getInstructions() const {
        return instructions;
    }

    std::string toString() const {
        std::string res = name + ":\n";
        for (const auto& inst : instructions) {
            res += "  " + inst->toString() + "\n";
        }
        return res;
    }

    private:
    std::string name;
    std::vector<std::unique_ptr<MIRInstruction>> instructions;
};

class MIRFunction {
public:
    MIRFunction(std::string name, std::shared_ptr<Type> returnType)
        : name(std::move(name)), returnType(returnType), isVarArg(false) {}

    const std::string& getName() const { return name; }
    std::shared_ptr<Type> getReturnType() const { return returnType; }

    void addParameter(std::string name, std::shared_ptr<Type> type) {
        params.push_back({std::move(name), type});
    }

    const std::vector<std::pair<std::string, std::shared_ptr<Type>>>& getParameters() const {
        return params;
    }

    void setVarArg(bool v) { isVarArg = v; }
    bool getVarArg() const { return isVarArg; }

    void appendBlock(std::unique_ptr<BasicBlock> block) {
        blocks.push_back(std::move(block));
    }

    const std::vector<std::unique_ptr<BasicBlock>>& getBlocks() const {
        return blocks;
    }

    std::string toString() const {
        std::string res = "fn " + name + "(";
        for (size_t i = 0; i < params.size(); ++i) {
            res += params[i].first + ": " + params[i].second->toString();
            if (i < params.size() - 1) res += ", ";
        }
        res += "): " + returnType->toString() + " {\n";
        for (const auto& block : blocks) {
            res += block->toString();
        }
        res += "}\n";
        return res;
    }

private:
    std::string name;
    std::shared_ptr<Type> returnType;
    std::vector<std::pair<std::string, std::shared_ptr<Type>>> params;
    std::vector<std::unique_ptr<BasicBlock>> blocks;
    bool isVarArg;
};

class MIRModule {
public:
    void addFunction(std::unique_ptr<MIRFunction> func) {
        functions.push_back(std::move(func));
    }

    void appendFunction(std::unique_ptr<MIRFunction> func) {
        addFunction(std::move(func));
    }

    MIRFunction* getFunction(const std::string& name) {
        for (const auto& f : functions) {
            if (f->getName() == name) return f.get();
        }
        return nullptr;
    }

    const std::vector<std::unique_ptr<MIRFunction>>& getFunctions() const { return functions; }

    std::string toString() const {
        std::string res;
        for (const auto& func : functions) {
            res += func->toString() + "\n";
        }
        return res;
    }

private:
    std::vector<std::unique_ptr<MIRFunction>> functions;
};

} // namespace chtholly
#endif // CHTHOLLY_MIR_H
