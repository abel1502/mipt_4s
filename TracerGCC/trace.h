#pragma once
#include "general.h"
#include <vector>
#include "stdsl.h"
#include "trace_entry.h"


#define FUNC_GUARD \
    TraceFuncGuard _guard{}


#pragma region TRACE_CODE
#define TRACE_CODE(OFILE_BASE, ...) {                                       \
    Trace::getInstance().reset();                                           \
                                                                            \
    {                                                                       \
        FUNC_GUARD;                                                         \
                                                                            \
        __VA_ARGS__;                                                        \
    }                                                                       \
                                                                            \
    /*Trace::getInstance().setSourceText(#__VA_ARGS__);*/                   \
                                                                            \
    std::fs::path logPath = "./output/";                                    \
    if (std::fs::current_path().filename() == "build") {                    \
        logPath = "../output/";                                             \
    }                                                                       \
                                                                            \
    logPath.append(OFILE_BASE);                                             \
                                                                            \
    HtmlTraceVisualizer{std::fs::path{logPath}.replace_extension("html")}   \
        .visualize(Trace::getInstance());                                   \
                                                                            \
    DotTraceVisualizer{std::fs::path{logPath}.replace_extension("svg")}     \
        .visualize(Trace::getInstance());                                   \
}
#pragma endregion TRACE_CODE




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
    
    /// Warning: Msg has to be persistent - that means, either
    /// compile-time, or, is you really want to, somehow globally
    /// stored until the end of the program
    void addDbgMsg(const char *msg);

    inline void setSourceText(const char *sourceText_) {
        sourceText = sourceText_;
    }

    const char *getSourceText() const {
        return sourceText;
    }

    inline const std::vector<TraceEntry> &getEntries() const { return entries; }
    inline       std::vector<TraceEntry> &getEntries()       { return entries; }

    TraceEntry::FuncInfo getCurFuncInfo() const;

protected:
    static Trace instance;


    std::vector<TraceEntry> entries{};
    TraceFuncGuard *curFunc{nullptr};
    unsigned recursionDepth = 0;
    const char *sourceText = nullptr;

    void addEntry(const TraceEntry &entry);

};

