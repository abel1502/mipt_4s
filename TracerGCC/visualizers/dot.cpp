#include "dot.h"
#include <sstream>
#include <cstdarg>
#include <algorithm>


#pragma region DotOutput
DotOutput::ArgsWriteProxy DotOutput::writeNode(const std::string_view &name) {
    // ofile.write("\"");
    ofile.write(name);
    // ofile.write("\" ");
    ofile.write(" ");

    return ArgsWriteProxy(ofile);
}

DotOutput::ArgsWriteProxy DotOutput::writefNode(const char *fmt, ...) {
    // ofile.write("\"");
    va_list args{};
    va_start(args, fmt);
    ofile.writefv(fmt, args);
    va_end(args);
    // ofile.write("\" ");
    ofile.write(" ");

    return ArgsWriteProxy(ofile);
}

DotOutput::ArgsWriteProxy DotOutput::writeEdge(const std::string_view &from,
                                               const std::string_view &to, bool constraint) {
    // ofile.write("\"");
    ofile.write(from);
    // ofile.write("\" -> \"");
    ofile.write(" -> ");
    ofile.write(to);
    // ofile.write("\" ");
    ofile.write(" ");

    return std::move(ArgsWriteProxy(ofile).arg("constraint", constraint ? "true" : "false"));
}

[[nodiscard]] DotOutput::EdgeFmtWriteProxy DotOutput::writefEdge(const char *fmtFrom, ...) {
    // ofile.write("\"");
    va_list args{};
    va_start(args, fmtFrom);
    ofile.writefv(fmtFrom, args);
    va_end(args);
    // ofile.write("\" -> ");
    ofile.write(" -> ");

    return EdgeFmtWriteProxy(ofile);
}

void DotOutput::beginGraph(const char *type, const std::string_view &name, bool strict) {
    if (strict) {
        ofile.write("strict ");
    }
    ofile.write(type);
    ofile.write(" ");
    ofile.write(name);
    ofile.write(" {\n");
}

void DotOutput::endGraph() {
    ofile.write("}\n");
}

DotOutput::ArgsWriteProxy DotOutput::writeDefaultArgs(const char *type) {
    ofile.write(type);
    ofile.write(" ");
    return ArgsWriteProxy{ofile};
}

void DotOutput::writeArg(const char *name, const std::string_view &value) {
    ArgsWriteProxy{ofile, false}.arg(name, value);
    ofile.write("\n");
}

void DotOutput::writeArg(const char *name, const char *fmt, ...) {
    va_list args{};
    va_start(args, fmt);
    ArgsWriteProxy{ofile, false}.argfv(name, fmt, args);
    va_end(args);
    ofile.write("\n");
}

std::string DotOutput::readTpl(const std::string_view &name) {
    std::fs::path tplPath = std::fs::current_path()
        .append("../tpl/").append(name).replace_extension(".tpl");
    
    std::ifstream file{tplPath, file.binary | file.in};  // Binary to avoid size changes

    REQUIRE(!file.bad());

    file.seekg(0, file.end);
    size_t size = file.tellg();
    file.seekg(0, file.beg);

    std::string buf{};
    buf.resize(size);
    file.read(buf.data(), size);

    return buf;
}


#pragma region ArgsWriteProxy
DotOutput::ArgsWriteProxy::~ArgsWriteProxy() {
    if (!disabled) {
        writeEnd();
    }
}

DotOutput::ArgsWriteProxy::ArgsWriteProxy(OutputFile &ofile_, bool braces_) :
    ofile{ofile_}, braces{braces_} {

    if (braces) {
        ofile.write("[");
    }
}

void DotOutput::ArgsWriteProxy::writeEnd() const {
    assert(!disabled);

    if (braces) {
        ofile.write("]\n");
    }
}

DotOutput::ArgsWriteProxy::ArgsWriteProxy(ArgsWriteProxy &&other) :
    ofile{other.ofile},
    braces{other.braces} {
    
    other.disabled = true;
}

DotOutput::ArgsWriteProxy &DotOutput::ArgsWriteProxy::arg(const char *name, const std::string_view &value) {
    assert(!disabled);

    ofile.write(name);
    ofile.write("=\"");
    ofile.write(value);
    ofile.write("\"; ");

    return *this;
}

DotOutput::ArgsWriteProxy &DotOutput::ArgsWriteProxy::argf(const char *name, const char *fmt, ...) {
    assert(!disabled);

    va_list args{};
    va_start(args, fmt);
    argfv(name, fmt, args);
    va_end(args);

    return *this;
}

DotOutput::ArgsWriteProxy &DotOutput::ArgsWriteProxy::argfv(const char *name, const char *fmt, va_list args) {
    assert(!disabled);

    ofile.write(name);
    ofile.write("=\"");
    ofile.writefv(fmt, args);
    ofile.write("\"; ");

    return *this;
}
#pragma endregion ArgsWriteProxy

#pragma region EdgeFmtWriteProxy
DotOutput::EdgeFmtWriteProxy::EdgeFmtWriteProxy(EdgeFmtWriteProxy &&other) :
    ofile{other.ofile}, willConstraint{other.willConstraint} {

    other.disabled = true;
}

DotOutput::EdgeFmtWriteProxy::~EdgeFmtWriteProxy() {
    if (!disabled) {
        writeEnd();
    }
}

DotOutput::ArgsWriteProxy DotOutput::EdgeFmtWriteProxy::to(const char *fmtTo, ...) {
    assert(!disabled);

    // ofile.write("\"");
    va_list args{};
    va_start(args, fmtTo);
    ofile.writefv(fmtTo, args);
    va_end(args);
    // ofile.write("\" ");
    ofile.write(" ");

    return std::move(ArgsWriteProxy(ofile).arg("constraint", willConstraint ? "true" : "false"));
}

DotOutput::EdgeFmtWriteProxy::EdgeFmtWriteProxy(OutputFile &ofile_) :
    ofile{ofile_} {}

void DotOutput::EdgeFmtWriteProxy::writeEnd() const {}
#pragma endregion EdgeFmtWriteProxy

#pragma region HtmlLabelWriteProxy
// TODO
#pragma endregion HtmlLabelWriteProxy

#pragma endregion DotOutput


#pragma region DotTraceVisualizer
void DotTraceVisualizer::visualize(const Trace &trace) {
    stats = {};
    
    beginLog();

    const auto &entries = trace.getEntries();
    nodesCnt = entries.size();
    nodesPresent.assign(nodesCnt, true);

    for (unsigned i = 0; i < nodesCnt; ++i) {
        logEntry(trace, i);
    }

    endLog();

    // TODO: Set to false after having debugged stuff
    compileOutput(true);
}

DotTraceVisualizer::~DotTraceVisualizer() {
    // TODO: Maybe conditionally endLog();
}

void DotTraceVisualizer::beginLog() {
    std::fs::remove(tmpFile);
    ofile.open();

    dot.beginGraph("digraph", "Log");

    dot.writeDefaultArgs("graph").arg("fontname", "Times");
    dot.writeDefaultArgs("node") .arg("fontname", "Times");
    dot.writeDefaultArgs("edge") .arg("fontname", "Times");

    dot.writeArg("rankdir", "TB");
    dot.writeArg("newrank", "true");

    dumpLegend();
    // Intentionally missing, due to it needing the summary data that's only available in the end
    // dumpInfo();

    dumpPadding();

    dot.beginGraph("subgraph", "cluster_log");
}

void DotTraceVisualizer::endLog() {
    dot.endGraph();

    dumpInfo();

    dumpOrdering();

    dot.endGraph();

    ofile.forceFlush();
    ofile.close();
}

void DotTraceVisualizer::dumpInfo() {
    dot.beginGraph("subgraph", "cluster_metadata");
    dot.writeNode("info")
        .arg("fontname", "Consolas")
        .arg("shape", "plain")
        .labelt("label_info",
                stats.copies,
                stats.moves,
                stats.copies + stats.moves);
    dot.endGraph();
}

void DotTraceVisualizer::dumpLegend() {
    dot.beginGraph("subgraph", "cluster_metadata");
    dot.beginGraph("subgraph", "cluster_legend");
    dot.writeArg("label", "Legend");

    dot.beginGraph("subgraph", "");
    dot.writeArg("rank", "same");

    dot.writeNode("legend_names")
        .labelt("label_legend",
                "timeline&nbsp;",
                "variable lifespan&nbsp;",
                "variable usage&nbsp;")
        .arg("shape", "plaintext");
    
    dot.writeNode("legend_targets")
        .labelt("label_legend",
                " ",
                " ",
                " ")
        .arg("shape", "plaintext");

    dot.endGraph();

    dot.writeEdge("legend_names:1:e", "legend_targets:1:w", true)
        .arg("color", style::edge_timeline_color)
        .arg("style", style::edge_timeline_style);
    
    dot.writeEdge("legend_names:2:e", "legend_targets:2:w", true)
        .arg("color", style::edge_lifespan_color)
        .arg("style", style::edge_lifespan_style);
    
    dot.writeEdge("legend_names:3:e", "legend_targets:3:w", true)
        .arg("color", style::edge_usage_color)
        .arg("style", style::edge_usage_style);

    dot.endGraph();
    dot.endGraph();
}

void DotTraceVisualizer::dumpOrdering() {
    unsigned lastNode = 0;

    while (lastNode < nodesCnt && !nodesPresent[lastNode]) {
        ++lastNode;
    }

    for (unsigned node = lastNode + 1; node < nodesCnt; ++node) {
        if (!nodesPresent[node]) {
            continue;
        }

        dot.writefEdge(NODE_FMT, lastNode).constraint().to(NODE_FMT, node)
            .arg("style", style::edge_timeline_style)
            .arg("color", style::edge_timeline_color);

        lastNode = node;
    }
}

void DotTraceVisualizer::dumpPadding() {
    dot.beginGraph("", "");
    dot.writeDefaultArgs("node")
        .arg("style", "invis");
    dot.writeNode("pad_1");
    dot.writeNode("pad_2");
    dot.endGraph();
    dot.writeEdge("pad_1", "pad_2")
        .arg("style", "invis");
    dot.writeEdge("pad_2", "node_0")
        .arg("style", "invis");
}

void DotTraceVisualizer::logEntry(const Trace &trace, unsigned idx) {
    const TraceEntry &entry = trace.getEntries()[idx];
    
    switch (entry.op) {
    case TracedOp::FuncEnter: {
        // A new function was entered

        dot.beginGraph("subgraph", std::string("cluster_func_") + std::to_string(idx));

        dot.writeArg("style", "filled");
        dot.writeArg("color", getFuncColor(entry.place.recursionDepth));
        dot.writeArg("label", entry.place.func.function_name());
        dot.writeArg("labeljust", "l");

        nodesPresent[idx] = false;
    } break;
    
    case TracedOp::FuncLeave: {
        // A function was exited

        dot.endGraph();

        nodesPresent[idx] = false;
    } break;

    case TracedOp::DbgMsg: {
        writeNode(idx, true)
            .label("[dbg] <i>%s</i>", entry.opStr);
    } break;

    case TracedOp::Ctor: {
        writeNode(idx)
            .labelt("label_unary", "green", "ctor", buildVarInfo(entry.inst).c_str());
        
        vars.onCtor(idx, entry.inst);
    } break;

    #if 0
    case TracedOp::Dtor: {
        logIndent();

        openTag("span")
            .arg("class", "label label-dtor")
            .argf("id", "label-dtor-%u", entry.inst.idx);
        openTag("a").argf("href", "#label-ctor-%u", entry.inst.idx);
        ofile.write("dtor");
        closeTag();
        closeTag();
        logVarInfo(entry.inst);

        ofile.writeln();
    } break;

    case TracedOp::Copy:
    case TracedOp::Move: {
        logIndent();

        bool isCtor = !wasVarCreated(entry.inst.idx);
        bool isCopy = entry.op == TracedOp::Copy;

        {
            auto tmp = openTag("span");
            tmp.argf("class", "label label-%s", isCopy ? "copy" : "move");
            if (isCtor) {
                tmp.argf("id", "label-ctor-%u", entry.inst.idx);
            }
        }
        if (isCtor) {
            openTag("a").argf("href", "#label-dtor-%u", entry.inst.idx);
        }
        ofile.write(isCopy ? "COPY" : "MOVE");
        if (!isCtor) {
            openTag("span").arg("class", "opstr");
            ofile.write("(=)");
            closeTag();
        }
        if (isCtor) {
            closeTag("a");
        }
        closeTag();

        logVarInfo(entry.inst);
        ofile.write(" from ");
        logVarInfo(entry.other);

        ofile.writeln();

        onVarCreated(entry.inst.idx);
    } break;

    case TracedOp::Inplace:
    case TracedOp::Binary:
    case TracedOp::Cmp: {
        logIndent();
        
        openTag("span").arg("class", "label label-bin");
        ofile.write("bin");
        openTag("span").arg("class", "opstr");
        ofile.writef("(%s)", entry.opStr);
        closeTag();
        closeTag();
        logVarInfo(entry.inst);
        ofile.write(" with ");
        logVarInfo(entry.other);

        ofile.writeln();
    } break;

    case TracedOp::Unary: {
        logIndent();
        
        openTag("span").arg("class", "label label-un");
        openTag("span").arg("class", "opstr");
        ofile.write("un");
        closeTag();
        closeTag();
        const char *fmt = "(%s)";  // Prefix version
        if (entry.other.isSet()) {  // Postfix version
            fmt = "(%s.)";
        }
        ofile.writef(fmt, entry.opStr);
        logVarInfo(entry.inst);

        ofile.writeln();
    } break;
    #endif

    // TODO: Remove
    default: {
        writeNode(idx, true)
            .label("Placeholder for op #%u", idx);
    } break;

    // NODEFAULT
    }
}

std::fs::path DotTraceVisualizer::getTmpFileName(const std::fs::path &outputFile) {
    return std::fs::path{outputFile}.replace_extension("dot");
}

void DotTraceVisualizer::compileOutput(bool debug) const {
    std::fs::remove(outputFile);

    std::ostringstream cmd{};
    cmd << "dot -Tsvg " << tmpFile << " -o " << outputFile;
    if (debug) {
        cmd << " -v";
    }
    cmd << '\0';

    DBG("%s", cmd.view().data());

    system(cmd.view().data());

    if (!debug) {
        std::fs::remove(tmpFile);
    }
}

std::string_view DotTraceVisualizer::getFuncColor(unsigned level) {
    constexpr std::string_view COLORS[] = {
        "#ffffff",
        "#efefef",
        "#dfdfdf",
        "#cfcfcf",
        "#bfbfbf",
        "#afafaf",
        "#9f9f9f",
        "#8f8f8f",
    };

    constexpr unsigned NUM_COLORS = sizeof(COLORS) / sizeof(COLORS[0]);

    if (level >= NUM_COLORS) {
        level = NUM_COLORS - 1;
    }

    return COLORS[level];
}

auto DotTraceVisualizer::writeNode(unsigned entryIdx, bool box) -> decltype(dot.writeNode("")) {
    return std::move(
        dot.writefNode(NODE_FMT, entryIdx)
            .arg("shape", box ? "box" : "plain")
    );
}

std::string DotTraceVisualizer::buildVarInfo(const TraceEntry::VarInfo &info) {
    #if 0
        <tr>
            <td align="left" border="1">%s</td> <!-- name -->
            <td align="left" border="1">#%u</td> <!-- idx -->
            <td align="left" border="1">%p:%u </td> <!-- ptr, ptrCell -->
            <td align="left" border="1">=%s</td> <!-- value -->
        </tr>
        <tr>
            <td align="left" colspan="4" border="1">%s</td> <!-- history -->
        </tr>;
    #endif

    std::string fmt = dot.readTpl("varinfo");

    // Args: name, idx, ptr, ptrCell, value, history
    std::string result = helpers::sprintfxx(fmt.c_str(),
        info.name, info.idx, info.addr, ptrCells.get(info.addr),
        info.valRepr.c_str(), vars[info].expression.c_str()
    );

    return result;
}

#pragma region Vars
DotTraceVisualizer::VarState &DotTraceVisualizer::Vars::getVarState(unsigned varIdx) {
    if (varIdx >= vars.size()) {
        vars.resize(std::max((size_t)varIdx + 1, vars.capacity() * 2));
    }

    return vars.at(varIdx);
}

void DotTraceVisualizer::Vars::onCtor(unsigned opIdx, const TraceEntry::VarInfo &info) {
    auto &state = getVarState(info);

    state.ctorOpIdx = opIdx;
    state.lastChangeOpIdx = opIdx;
    state.lastUseOpIdx = opIdx;
    state.expression = info.name;
    state.lastAddr = info.addr;
}

void DotTraceVisualizer::Vars::onUsed(unsigned opIdx, const TraceEntry::VarInfo &info) {
    auto &state = getVarState(info);

    state.lastUseOpIdx = opIdx;
    state.lastAddr = info.addr;
}

void DotTraceVisualizer::Vars::onDtor(unsigned opIdx, const TraceEntry::VarInfo &info) {
    auto &state = getVarState(info);

    state.dtorOpIdx = opIdx;
    state.lastChangeOpIdx = opIdx;
    state.lastUseOpIdx = opIdx;
    state.lastAddr = info.addr;
}

void DotTraceVisualizer::Vars::onChanged(unsigned opIdx, const TraceEntry::VarInfo &info,
                                         std::function<void (std::string &curExpr)> operation) {
    auto &state = getVarState(info);

    state.lastChangeOpIdx = opIdx;
    state.lastUseOpIdx = opIdx;
    state.lastAddr = info.addr;
    operation(state.expression);
}

void DotTraceVisualizer::Vars::onChanged(unsigned opIdx, const TraceEntry::VarInfo &info,
                                         const TraceEntry::VarInfo &otherInfo,
                                         std::function<void (std::string &curExpr,
                                         const std::string_view &otherExpr)> operation) {
    auto &state      = getVarState(info);
    auto &stateOther = getVarState(otherInfo);

    state.lastChangeOpIdx = opIdx;
    state.lastUseOpIdx = opIdx;
    state.lastAddr = info.addr;

    stateOther.lastUseOpIdx = opIdx;
    stateOther.lastAddr = info.addr;

    operation(state.expression, stateOther.expression);
}

#pragma endregion Vars

#pragma endregion DotTraceVisualizer

