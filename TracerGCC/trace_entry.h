#pragma once
#include "general.h"
#include <source_location>


enum class TracedOp {
    Ctor,
    Dtor,
    Copy,
    Move,  // Unused if !defined(TRACER_RVALUE_REFS)
    Unary,
    Binary,
    Inplace,
    Cmp,
};


struct TraceEntry {
    struct VarInfo {
        const void *addr = nullptr;
        unsigned idx = -1u;
        const char *name{};
        std::string valRepr{};


        constexpr bool isSet() const {
            return (bool)addr;
        }

        static inline VarInfo empty() {
            return VarInfo{};
        }
    };

    struct FuncInfo {
        std::source_location func{};
        unsigned recursionDepth = -1u;


        constexpr bool isSet() const {
            return recursionDepth != -1u;
        }

        static constexpr FuncInfo empty() {
            return FuncInfo{};
        }
    };


    TracedOp op{};
    VarInfo inst{};
    VarInfo other{};
    const char *opStr{};
    FuncInfo place{};

};
