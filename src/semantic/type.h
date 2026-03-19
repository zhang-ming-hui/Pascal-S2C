#pragma once

#include <string>
#include <utility>
#include <vector>

#include "ast/ast.h"

namespace pascal_s2c {

struct TypeInfo {
    BasicTypeKind basic = BasicTypeKind::Void;
    bool isArray = false;
    std::vector<ArrayBound> dims;
};

inline TypeInfo makeScalarType(BasicTypeKind kind) {
    return TypeInfo{kind, false, {}};
}

inline TypeInfo makeArrayType(BasicTypeKind elementKind, std::vector<ArrayBound> dims) {
    TypeInfo type;
    type.basic = elementKind;
    type.isArray = true;
    type.dims = std::move(dims);
    return type;
}

inline bool isScalarType(const TypeInfo& type) {
    return !type.isArray;
}

inline bool isNumericType(const TypeInfo& type) {
    return !type.isArray && (type.basic == BasicTypeKind::Integer || type.basic == BasicTypeKind::Real);
}

inline bool isBooleanType(const TypeInfo& type) {
    return !type.isArray && type.basic == BasicTypeKind::Boolean;
}

inline bool isCharType(const TypeInfo& type) {
    return !type.isArray && type.basic == BasicTypeKind::Char;
}

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

inline bool sameType(const TypeInfo& lhs, const TypeInfo& rhs) {
    return lhs.basic == rhs.basic && lhs.isArray == rhs.isArray && sameBounds(lhs.dims, rhs.dims);
}

inline TypeInfo arrayElementType(const TypeInfo& type) {
    return makeScalarType(type.basic);
}

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
    case BasicTypeKind::Void:
    default:
        return "void";
    }
}

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
