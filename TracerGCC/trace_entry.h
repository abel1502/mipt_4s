#pragma once
#include "general.h"
#include "stdsl.h"


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


        inline VarInfo() = default;

        inline VarInfo(const void *addr_, unsigned idx_, const char *name_,
                       const std::string_view &valRepr_) :
            addr{addr_}, idx{idx_}, name{name_}, valRepr{valRepr_} {}

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


        inline FuncInfo() = default;

        inline FuncInfo(const std::source_location &func_, unsigned recursionDepth_) :
            func{func_}, recursionDepth{recursionDepth_} {}

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


    inline TraceEntry() = default;

    inline TraceEntry(TracedOp op_, const VarInfo &inst_, const VarInfo &other_,
                      const char *opStr_, const FuncInfo &place_) :
        op{op_}, inst{inst_},
        other{other_}, opStr{opStr_},
        place{place_} {}
};
