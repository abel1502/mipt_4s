#pragma once
#include "general.h"
#include "trace_entry.h"
#include <string_view>
#include <string>
#include <cstdarg>


// Uncomment to include move-semantics-related operations
// #define TRACER_RVALUE_REFS

// Uncomment to copy the results inside arguments for binary and unary operators
// #define TRACER_COPY_IN_ARGS


#define DECL_TVAR(T, NAME, ...) \
    Tracer<T> NAME{__VA_ARGS__ __VA_OPT__(,) #NAME}


#if 0
class TracerBase {
public:
    unsigned getIdx() const { return idx; }
    const char *getName() const { return name; }
    virtual std::string getValRepr() const = 0;
    virtual std::string getLastValRepr() const = 0;

protected:
    static unsigned nextIdx;

    unsigned idx{nextIdx++};
    const std::string_view name{};


    virtual void onOperation(TracedOp op, const Tracer *inst,
                             const Tracer *other, const char *opStr) = 0;

};
#endif


namespace _myimpl {

void addToTrace(TracedOp op, const TraceEntry::VarInfo &inst,
                const TraceEntry::VarInfo &other, const char *opStr);

}


template <typename T>
class Tracer {
public:
    // TODO: Move to VarInfo?
    static constexpr const char DEFAULT_VAR_NAME[] = "<i>&lt;?&gt;</i>";


    using underlying_t = T;


    #pragma region Getters
    const T &getVal() const { return value; }
    const T &getLastVal() const { return lastValue; }
    unsigned getIdx() const { return idx; }
    const char *getName() const { return name; }

    explicit operator T() const { return getVal(); }

    std::string getValRepr() const {
        return std::to_string(getVal());
    }

    std::string getLastValRepr() const {
        return std::to_string(getLastVal());
    }

    TraceEntry::VarInfo getVarInfo(bool useLastVal = false) const {
        return TraceEntry::VarInfo{
            this,
            idx,
            name,
            useLastVal ? getLastValRepr() : getValRepr()
        };
    }
    #pragma endregion Getters

    #pragma region Ctor,Dtor
    Tracer(const T &value_ = T{}, const char *name_ = DEFAULT_VAR_NAME) :
        value{value_}, name{name_} {

        onOperation(TracedOp::Ctor, this, nullptr, "");
    }

    ~Tracer() {
        onOperation(TracedOp::Dtor, this, nullptr, "");
    }
    #pragma endregion Ctor,Dtor

    #pragma region Copy,Move
    Tracer(const Tracer &other) :
        value{other.value},
        lastValue{other.lastValue},
        name{DEFAULT_VAR_NAME} {

        onOperation(TracedOp::Copy, this, &other, "cp");
    }

    Tracer &operator=(const Tracer &other) {
        lastValue = value;
        value = other.value;

        onOperation(TracedOp::Copy, this, &other, "cp=");

        return *this;
    }

    #ifdef TRACER_RVALUE_REFS
    Tracer(Tracer &&other) :
        value{std::move(other.value)},
        lastValue{std::move(other.lastValue)},
        name{DEFAULT_VAR_NAME} {

        onOperation(TracedOp::Move, this, &other, "mv");
    }

    Tracer &operator=(Tracer &&other) {
        lastValue = value;
        other.lastValue = other.value;
        std::swap(value, other.value);

        onOperation(TracedOp::Move, this, &other, "mv=");

        return *this;
    }
    #endif // TRACER_RVALUE_REFS
    #pragma endregion Copy,Move

    #pragma region Inplace
    #define INPLACE_OP_(OP)                                     \
        Tracer &operator OP(const Tracer &other) {              \
            lastValue = value;                                  \
            value OP other.value;                               \
                                                                \
            onOperation(TracedOp::Inplace, this, &other, #OP);  \
                                                                \
            return *this;                                       \
        }

    INPLACE_OP_(+=);
    INPLACE_OP_(-=);
    INPLACE_OP_(*=);
    INPLACE_OP_(/=);
    INPLACE_OP_(%=);
    INPLACE_OP_(>>=);
    INPLACE_OP_(<<=);
    INPLACE_OP_(&=);
    INPLACE_OP_(^=);
    INPLACE_OP_(|=);

    #undef INPLACE_OP_
    #pragma endregion Inplace

    #pragma region Binary
    #ifdef TRACER_COPY_IN_ARGS
    #define BINARY_OP_(OP)                                      \
        friend Tracer operator OP(Tracer self,                  \
                                  const Tracer &other) {        \
            self.lastValue = self.value;                        \
            self.value OP##= other.value;                       \
                                                                \
            onOperation(TracedOp::Binary, &self, &other, #OP);  \
                                                                \
            return self;                                        \
        }
    #else
    #define BINARY_OP_(OP)                                      \
        Tracer operator OP(const Tracer &other) const {         \
            Tracer self{*this};                                 \
            self.lastValue = self.value;                        \
            self.value OP##= other.value;                       \
                                                                \
            onOperation(TracedOp::Binary, &self, &other, #OP);  \
                                                                \
            return self;                                        \
        }
    #endif

    BINARY_OP_(+);
    BINARY_OP_(-);
    BINARY_OP_(*);
    BINARY_OP_(/);
    BINARY_OP_(%);
    BINARY_OP_(>>);
    BINARY_OP_(<<);
    BINARY_OP_(&);
    BINARY_OP_(^);
    BINARY_OP_(|);

    #undef BINARY_OP_
    #pragma endregion Binary

    #pragma region Increments & Decrements
    Tracer &operator++() {
        lastValue = value;
        ++value;

        onOperation(TracedOp::Unary, this, nullptr, "++");

        return *this;
    }

    Tracer &operator--() {
        lastValue = value;
        --value;

        onOperation(TracedOp::Unary, this, nullptr, "--");

        return *this;
    }

    Tracer operator++(int) {
        Tracer result{value++};

        onOperation(TracedOp::Unary, this, &result, "++");

        return result;
    }

    Tracer operator--(int) {
        Tracer result{value--};

        onOperation(TracedOp::Unary, this, &result, "--");

        return result;
    }
    #pragma endregion Increments & Decrements

    #pragma region Unary
    #ifdef TRACER_COPY_IN_ARGS
    #define UNARY_OP_(OP)                                       \
        friend Tracer operator OP(Tracer self) {                \
            /*self.lastValue = self.value;*/                    \
            self.value = OP self.value;                         \
                                                                \
            onOperation(TracedOp::Unary, &self, nullptr, #OP);  \
                                                                \
            return self;                                        \
        }
    #else
    #define UNARY_OP_(OP)                                       \
        Tracer operator OP() const {                            \
            Tracer self{*this};                                 \
            /*self.lastValue = self.value;*/                    \
            self.value = OP self.value;                         \
                                                                \
            onOperation(TracedOp::Unary, &self, nullptr, #OP);  \
                                                                \
            return self;                                        \
        }
    #endif

    UNARY_OP_(+);
    UNARY_OP_(-);
    UNARY_OP_(~);

    #undef UNARY_OP_
    #pragma endregion Unary

    #pragma region Cmp
    #define CMP_OP_(OP)                                     \
        bool operator OP(const Tracer &other) const {       \
            bool result = (value OP other.value);           \
                                                            \
            onOperation(TracedOp::Cmp, this, &other, #OP);  \
                                                            \
            return result;                                  \
        }

    CMP_OP_(==);
    CMP_OP_(!=);
    CMP_OP_(<);
    CMP_OP_(<=);
    CMP_OP_(>);
    CMP_OP_(>=);

    #undef BINARY_OP_
    #pragma endregion Cmp

protected:
    static unsigned nextIdx;

    T value{};
    T lastValue{};
    unsigned idx{nextIdx++};
    const char *name{};


    static /*virtual*/ void onOperation(TracedOp op, const Tracer *inst, const Tracer *other, const char *opStr) {
        // basicLogOp(op, inst, other, opStr);

        addToTrace(op, inst, other, opStr);
    }

    static void addToTrace(TracedOp op, const Tracer *inst, const Tracer *other, const char *opStr) {
        switch (op) {
        case TracedOp::Ctor:
        case TracedOp::Dtor:
            _myimpl::addToTrace(op, inst->getVarInfo(), TraceEntry::VarInfo::empty(), opStr);
            break;

        case TracedOp::Copy:
        case TracedOp::Move:
        case TracedOp::Inplace:
        case TracedOp::Binary:
            _myimpl::addToTrace(op, inst->getVarInfo(true), other->getVarInfo(), opStr);
            break;

        case TracedOp::Unary:
            _myimpl::addToTrace(op, inst->getVarInfo(true), TraceEntry::VarInfo::empty(), opStr);
            break;

        case TracedOp::Cmp:
            _myimpl::addToTrace(op, inst->getVarInfo(), other->getVarInfo(), opStr);
            break;

        NODEFAULT
        }
    }

    static void basicLogOp(TracedOp op, const Tracer *inst, const Tracer *other, const char *opStr) {
        #define WF_ "%p(%s)"
        #define WA_(NAME, LAST) NAME, NAME->get##LAST##ValRepr().c_str()

        switch (op) {
        case TracedOp::Ctor:
            _log("ctor " WF_, WA_(inst, ));
            break;

        case TracedOp::Dtor:
            _log("dtor " WF_, WA_(inst, ));
            break;

        case TracedOp::Copy:
        case TracedOp::Move:
            _log(WF_ " %s " WF_, WA_(inst, Last), opStr, WA_(other, ));
            break;

        case TracedOp::Inplace:
            _log(WF_ " %s " WF_ , WA_(inst, Last), opStr, WA_(other, ));
            break;

        case TracedOp::Binary:
            _log(WF_ " = " WF_ " %s " WF_, WA_(inst, ), WA_(inst, Last), opStr, WA_(other, ));
            break;

        case TracedOp::Unary:
            if (other) {
                _log(WF_ "%s", WA_(inst, Last), opStr);
            } else {
                _log("%s" WF_, opStr, WA_(inst, Last));
            }
            break;

        case TracedOp::Cmp:
            _log(WF_ " %s " WF_ " ?", WA_(inst, ), opStr, WA_(other, ));
            break;

        default:
            break;
        }

        #undef WF_
        #undef WA_
    }

    static void _log(const char *fmt, ...) {
        va_list args{};
        va_start(args, fmt);

        printf("[Trace] ");
        vprintf(fmt, args);
        printf("\n");

        va_end(args);
    }

};


template <typename T>
unsigned Tracer<T>::nextIdx = 0;


extern template
class Tracer<int>;

