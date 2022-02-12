#pragma once
#include "general.h"
#include <vector>
#include "stdsl.h"
#include "trace_entry.h"


#define FUNC_GUARD \
    TraceFuncGuard _guard{}


// TODO: Implement everything


class TraceFuncGuard {
public:
    TraceFuncGuard(const std::source_location &place_ = std::source_location::current());

    ~TraceFuncGuard();

    inline const std::source_location &getPlace() const {
        return place;
    }

protected:
    TraceFuncGuard *prevGuard{};
    std::source_location place;

};


class Trace {
public:
    Trace() = default;

    static Trace &getInstance() {
        return instance;
    }

    void reset();

    TraceFuncGuard *pushFunc(TraceFuncGuard *next);
    void popFunc(TraceFuncGuard *cur, TraceFuncGuard *prev);

    void regEvent(TracedOp op, const TraceEntry::VarInfo &inst,
                  const TraceEntry::VarInfo &other, const char *opStr);

    inline const std::vector<TraceEntry> &getEntries() const { return entries; }
    inline       std::vector<TraceEntry> &getEntries()       { return entries; }

protected:
    static Trace instance;


    std::vector<TraceEntry> entries{};
    TraceFuncGuard *curFunc{nullptr};
    unsigned recursionDepth = 0;

};

