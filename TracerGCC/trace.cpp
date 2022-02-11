#include "trace.h"


Trace Trace::instance{};


TraceFuncGuard::TraceFuncGuard(const std::source_location &place_) :
    place{place_} {

    prevGuard = Trace::getInstance().pushFunc(this);
}

TraceFuncGuard::~TraceFuncGuard() {
    Trace::getInstance().popFunc(this, prevGuard);
}


void Trace::reset() {
    entries.clear();

    // MUSTN'T be done, because that would invalidate the guard hierarchy
    // curFunc = nullptr;
}

TraceFuncGuard *Trace::pushFunc(TraceFuncGuard *next) {
    TraceFuncGuard *prev = curFunc;

    curFunc = next;
    ++recursionDepth;

    return prev;
}

void Trace::popFunc(TraceFuncGuard *cur, TraceFuncGuard *prev) {
    assert(curFunc == cur);

    curFunc = prev;
    --recursionDepth;
}

void Trace::regEvent(TracedOp op, const TraceEntry::VarInfo &inst,
                     const TraceEntry::VarInfo &other, const char *opStr) {
    TraceEntry::FuncInfo funcInfo{};
    if (curFunc) {
        funcInfo.func = curFunc->getPlace();
        funcInfo.recursionDepth = recursionDepth;
    }

    entries.emplace_back(op, inst, other, opStr, funcInfo);
}
