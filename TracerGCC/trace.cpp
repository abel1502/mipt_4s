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

    addEntry(TraceEntry::funcSwitch(getCurFuncInfo(), true));

    return prev;
}

void Trace::popFunc(TraceFuncGuard *cur, TraceFuncGuard *prev) {
    assert(curFunc == cur);
    assert(recursionDepth > 0);

    curFunc = prev;
    --recursionDepth;

    addEntry(TraceEntry::funcSwitch(getCurFuncInfo(), false));
}

void Trace::regEvent(TracedOp op, const TraceEntry::VarInfo &inst,
                     const TraceEntry::VarInfo &other, const char *opStr) {
    addEntry(TraceEntry{op, inst, other, opStr, getCurFuncInfo()});
}

void Trace::addDbgMsg(const char *msg) {
    addEntry(TraceEntry{TracedOp::DbgMsg,
                        TraceEntry::VarInfo::empty(),
                        TraceEntry::VarInfo::empty(),
                        msg, getCurFuncInfo()});
}

TraceEntry::FuncInfo Trace::getCurFuncInfo() const {
    return curFunc ?
           TraceEntry::FuncInfo{curFunc->getPlace(), recursionDepth} :
           TraceEntry::FuncInfo::globalScope();
}

void Trace::addEntry(const TraceEntry &entry) {
    entries.push_back(entry);
}
