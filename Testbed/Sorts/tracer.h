#pragma once

#include <ACL/general.h>
#include <ACL/signal.h>


namespace _myimpl {

struct TracerExtraPlaceholder {};

}


template <typename T, typename E = _myimpl::TracerExtraPlaceholder>
class Tracer {
public:
    enum class Op {
        Ctor,
        Dtor,
        Copy,
        Move,
        Unary,
        Binary,
        Inplace,
        Cmp,
    };


    // Other may be null
    static abel::Signal<bool (Op op, const Tracer *inst, const Tracer *other)> sigOp;
    // Any extra data a particular implementation
    // might desire should reside here
    E extra{};


    const T &getVal() const { return value; }


    #pragma region Ctor,Dtor,Copy,Move
    Tracer() :
        Tracer(T{}) {}

    Tracer(const T &value_) :
        Tracer(std::move(T{value_})) {}

    Tracer(T &&value_) :
        value{std::move(value_)} {

        sigOp(Op::Ctor, this, nullptr);
    }

    Tracer(const Tracer &other) :
        value{other.value} {

        sigOp(Op::Copy, this, &other);
    }

    Tracer(Tracer &&other) :
        value{std::move(other.value)} {

        sigOp(Op::Move, this, &other);
    }

    Tracer &operator=(const Tracer &other) {
        value = other.value;

        sigOp(Op::Copy, this, &other);

        return *this;
    }

    Tracer &operator=(Tracer &&other) {
        std::swap(value, other.value);

        sigOp(Op::Move, this, &other);

        return *this;
    }

    ~Tracer() {
        sigOp(Op::Dtor, this, nullptr);
    }
    #pragma endregion Ctor,Dtor,Copy,Move

    #pragma region Inplace
    #define INPLACE_OP_(OP)                         \
        Tracer &operator OP(const Tracer &other) {  \
            value OP other.value;                   \
                                                    \
            sigOp(Op::Inplace, this, &other);       \
                                                    \
            return *this;                           \
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
    #define BINARY_OP_(OP)                                  \
        friend Tracer operator OP(Tracer self,              \
                                  const Tracer &other) {    \
            self.value OP##= other.value;                   \
                                                            \
            sigOp(Op::Binary, &self, &other);               \
                                                            \
            return self;                                    \
        }

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
        ++value;

        sigOp(Op::Unary, this, nullptr);

        return *this;
    }

    Tracer &operator--() {
        --value;

        sigOp(Op::Unary, this, nullptr);

        return *this;
    }

    Tracer operator++(int) {
        Tracer result{value++};

        sigOp(Op::Unary, this, nullptr);

        return result;
    }

    Tracer operator--(int) {
        Tracer result{value--};

        sigOp(Op::Unary, this, nullptr);

        return result;
    }
    #pragma endregion Increments & Decrements

    #pragma region Unary
    #define UNARY_OP_(OP)                           \
        friend Tracer operator OP(Tracer self) {    \
            self.value = OP self.value;             \
                                                    \
            sigOp(Op::Unary, &self, nullptr);       \
                                                    \
            return self;                            \
        }

    UNARY_OP_(+);
    UNARY_OP_(-);
    UNARY_OP_(~);

    #undef UNARY_OP_
    #pragma endregion Unary

    #pragma region Cmp
    #define CMP_OP_(OP)                                 \
        bool operator OP(const Tracer &other) const {   \
            bool result = (value OP other.value);       \
                                                        \
            sigOp(Op::Cmp, this, &other);               \
                                                        \
            return result;                              \
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
    T value{};

};


template <typename T, typename E>
abel::Signal<bool (typename Tracer<T, E>::Op op, const Tracer<T, E> *inst, const Tracer<T, E> *other)>
    Tracer<T, E>::sigOp{};

