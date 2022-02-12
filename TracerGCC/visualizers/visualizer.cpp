#include "visualizer.h"


TraceVisualizer::TraceVisualizer(const std::fs::path &path, bool cumulative) :
    ofile{path, cumulative} {}

