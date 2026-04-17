#pragma once

#include <string>
#include <utility>
#include <vector>

#include "ast/ast.h"

namespace pascal_s2c {

// 语义阶段统一类型描述。
// 使用位置：
// 1) Symbol.type 记录符号类型。
// 2) SemanticContext.expressionTypes 记录表达式类型。
// 3) CodeGenerator 根据 TypeInfo 输出 C 类型与格式串。
// 存储信息：
// - basic: 元素/标量基础类型
// - isArray: 是否数组
// - dims: 数组各维边界
struct TypeInfo {
    BasicTypeKind basic = BasicTypeKind::Void;
    bool isArray = false;
    std::vector<ArrayBound> dims;
};

// 构造标量类型。
inline TypeInfo makeScalarType(BasicTypeKind kind) {
    return TypeInfo{kind, false, {}};
}

// 构造数组类型。
inline TypeInfo makeArrayType(BasicTypeKind elementKind, std::vector<ArrayBound> dims) {
    TypeInfo type;
    type.basic = elementKind;
    type.isArray = true;
    type.dims = std::move(dims);
    return type;
}

// 判定是否标量类型。
inline bool isScalarType(const TypeInfo& type) {
    return !type.isArray;
}

// 判定是否数值类型（integer/real 且非数组）。
inline bool isNumericType(const TypeInfo& type) {
    return !type.isArray && (type.basic == BasicTypeKind::Integer || type.basic == BasicTypeKind::Real);
}

// 判定是否布尔类型（且非数组）。
inline bool isBooleanType(const TypeInfo& type) {
    return !type.isArray && type.basic == BasicTypeKind::Boolean;
}

// 判定是否字符类型（且非数组）。
inline bool isCharType(const TypeInfo& type) {
    return !type.isArray && type.basic == BasicTypeKind::Char;
}

// 判定是否字符串类型（且非数组）。
inline bool isStringType(const TypeInfo& type) {
    return !type.isArray && type.basic == BasicTypeKind::String;
}

// 比较数组边界是否完全一致。
inline bool sameBounds(const std::vector<ArrayBound>& lhs, const std::vector<ArrayBound>& rhs) {
    if (lhs.size() != rhs.size()) {
        return false;
    }
    for (std::size_t i = 0; i < lhs.size(); ++i) {
        if (lhs[i].lower != rhs[i].lower || lhs[i].upper != rhs[i].upper) {
            return false;
        }
    }
    return true;
}

// 比较两个 TypeInfo 是否完全相同。
inline bool sameType(const TypeInfo& lhs, const TypeInfo& rhs) {
    return lhs.basic == rhs.basic && lhs.isArray == rhs.isArray && sameBounds(lhs.dims, rhs.dims);
}

// 取数组元素类型（当前实现返回同 basic 的标量）。
inline TypeInfo arrayElementType(const TypeInfo& type) {
    return makeScalarType(type.basic);
}

// 基础类型转字符串。
inline std::string toString(BasicTypeKind kind) {
    switch (kind) {
    case BasicTypeKind::Integer:
        return "integer";
    case BasicTypeKind::Real:
        return "real";
    case BasicTypeKind::Boolean:
        return "boolean";
    case BasicTypeKind::Char:
        return "char";
    case BasicTypeKind::String:
        return "string";
    case BasicTypeKind::Void:
    default:
        return "void";
    }
}

// 完整 TypeInfo 转字符串（用于诊断与日志）。
inline std::string toString(const TypeInfo& type) {
    if (!type.isArray) {
        return toString(type.basic);
    }

    std::string result = "array[";
    for (std::size_t i = 0; i < type.dims.size(); ++i) {
        if (i != 0) {
            result += ", ";
        }
        result += std::to_string(type.dims[i].lower);
        result += "..";
        result += std::to_string(type.dims[i].upper);
    }
    result += "] of ";
    result += toString(type.basic);
    return result;
}

}  // namespace pascal_s2c
