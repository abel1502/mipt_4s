#include "subapp_counter.h"
#include <cstdarg>
#include <string>


CounterSubapp::CounterSubapp() {
    wrapper_t::sigOp += [this](wrapper_t::Op op, const wrapper_t *inst, const wrapper_t *other, const char *opSym) {
        if (!running) {
            return false;
        }

        onWrapperOp(op, inst, other, opSym);

        return false;
    };
}

void CounterSubapp::activate() {
    running = true;

    wrapper_t a = 10;
    wrapper_t b = 20;

    b = a + b;
    if (b < a + 17) {
        a /= 3;
    }

    ++a;
    b++;

    running = false;
}

void CounterSubapp::onWrapperOp(wrapper_t::Op op, const wrapper_t *inst, const wrapper_t *other, const char *opSym) {
    using Op = wrapper_t::Op;

    #define WF_ "%p(%s)"
    #define WA_(NAME, LAST) NAME, std::to_string(NAME->get##LAST##Val()).c_str()

    switch (op) {
    case Op::Ctor:
        log("ctor " WF_, WA_(inst, ));
        break;

    case Op::Dtor:
        log("dtor " WF_, WA_(inst, ));
        break;

    case Op::Copy:
    case Op::Move:
        log(WF_ " %s " WF_, WA_(inst, Last), opSym, WA_(other, ));
        break;

    case Op::Inplace:
        log(WF_ " %s " WF_ , WA_(inst, Last), opSym, WA_(other, ));
        break;

    case Op::Binary:
        log(WF_ " = " WF_ " %s " WF_, WA_(inst, ), WA_(inst, Last), opSym, WA_(other, ));
        break;

    case Op::Unary:
        if (other) {
            log(WF_ "%s", WA_(inst, Last), opSym);
        } else {
            log("%s" WF_, opSym, WA_(inst, Last));
        }
        break;

    case Op::Cmp:
        log(WF_ " %s " WF_ " ?", WA_(inst, ), opSym, WA_(other, ));
        break;

    NODEFAULT
    }

    #undef WF_
    #undef WA_
}

void CounterSubapp::log(const char *fmt, ...) const {
    va_list args{};
    va_start(args, fmt);

    printf("[Counter] ");
    vprintf(fmt, args);
    printf("\n");

    va_end(args);
}
