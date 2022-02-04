#pragma once
#include "sort_provider.h"
#include "tracer.h"
#include <ACL/vector.h>


class CounterSubapp {
private:
    struct _MarkerExtra {};

public:
    using elem_t = int;
    using wrapper_t = Tracer<elem_t, _MarkerExtra>;


    CounterSubapp();

    CounterSubapp(const CounterSubapp &other) = delete;
    CounterSubapp &operator=(const CounterSubapp &other) = delete;
    CounterSubapp(CounterSubapp &&other) = delete;
    CounterSubapp &operator=(CounterSubapp &&other) = delete;


    void activate();

protected:
    bool running = false;


    void onWrapperOp(wrapper_t::Op op, const wrapper_t *inst, const wrapper_t *other, const char *opSym);

    void log(const char *opFmt, ...) const;

};
