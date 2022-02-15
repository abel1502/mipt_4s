#pragma once
#include "../general.h"
#include "visualizer.h"
#include <vector>
#include <map>


class DotTraceVisualizer : public TraceVisualizer {
public:
    inline DotTraceVisualizer(const std::fs::path &outputFile) :
        TraceVisualizer(outputFile) {}

    virtual void visualize(const Trace &trace) override;

    virtual ~DotTraceVisualizer() override;

protected:
    std::map<const void *, unsigned> ptrCells{};
    unsigned ptrCellCounter = 0;
    std::vector<bool> created;


    void beginLog();
    void endLog();

    void logEntry(const TraceEntry &entry);
    void logVarInfo(const TraceEntry::VarInfo &info);

    inline unsigned getPtrCell(const void *addr);
    inline void onVarCreated(unsigned idx);
    inline bool wasVarCreated(unsigned idx) const;

};


inline unsigned DotTraceVisualizer::getPtrCell(const void *addr) {
    if (!ptrCells.contains(addr)) {
        ptrCells[addr] = ptrCellCounter++;
    }

    return ptrCells[addr];
}

inline void DotTraceVisualizer::onVarCreated(unsigned idx) {
    if (idx >= created.size()) {
        created.resize((size_t)idx + 1);
    }

    created[idx] = true;
}

inline bool DotTraceVisualizer::wasVarCreated(unsigned idx) const {
    return idx < created.size() ? created[idx] : false;
}
