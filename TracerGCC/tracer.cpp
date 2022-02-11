#include "tracer.h"
#include "trace.h"


namespace _myimpl {

void addToTrace(TracedOp op, const TraceEntry::VarInfo &inst,
                const TraceEntry::VarInfo &other, const char *opStr) {
    Trace::getInstance().regEvent(op, inst, other, opStr);
}

}


template
class Tracer<int>;
