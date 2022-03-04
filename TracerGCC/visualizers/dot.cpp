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
    writeArgs().arg(name, value);
    ofile.write("\n");
}

void DotOutput::writeArg(const char *name, const char *fmt, ...) {
    va_list args{};
    va_start(args, fmt);
    writeArgs().argfv(name, fmt, args);
    va_end(args);
    ofile.write("\n");
}

DotOutput::ArgsWriteProxy DotOutput::writeArgs() {
    return ArgsWriteProxy{ofile, false};
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

DotOutput::HtmlLabelWriteProxy DotOutput::ArgsWriteProxy::labelh() {
    ofile.write("label=");

    return HtmlLabelWriteProxy{std::move(*this), ofile};
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
DotOutput::HtmlLabelWriteProxy::HtmlLabelWriteProxy(HtmlLabelWriteProxy &&other) :
    parent{std::move(other.parent)}, ofile{other.ofile},
    tagStack{std::move(other.tagStack)} {
    
    other.disabled = true;
}

DotOutput::HtmlLabelWriteProxy::~HtmlLabelWriteProxy() {
    if (!disabled) {
        writeEnd();
    }
}

DotOutput::ArgsWriteProxy DotOutput::HtmlLabelWriteProxy::finish() {
    assert(!disabled);

    writeEnd();
    disabled = true;

    return std::move(parent);
}

DotOutput::HtmlLabelWriteProxy::HtmlLabelWriteProxy(ArgsWriteProxy &&parent_,
                                                    OutputFile &ofile_) :
    parent{std::move(parent_)}, ofile{ofile_} {

    assert(!disabled);
    ofile.write("<");
}

void DotOutput::HtmlLabelWriteProxy::writeEnd() {
    assert(!disabled);

    while (!tagStack.empty()) {
        closeTag();
    }

    ofile.write(">; ");
}
#pragma endregion HtmlLabelWriteProxy

#pragma endregion DotOutput


#pragma region DotTraceVisualizer
void DotTraceVisualizer::visualize(const Trace &trace) {
    stats = {};
    sourceText = trace.getSourceText();
    
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
    dot.writeArg("splines", "ortho");
    // dot.writeArg("margin", "10");
    // dot.writeArg("concentrate", "true");
    // dot.writeArg("esep", "10");
    // dot.writeArg("overlap", "scalexy");
    // dot.writeArg("K", "3");

    dumpLegend();
    // Intentionally missing, due to it needing the summary data that's only available in the end
    // dumpInfo();
    dumpSourceText();

    dumpPadding();

    dot.beginGraph("subgraph", "cluster_log");
}

void DotTraceVisualizer::endLog() {
    dot.endGraph();

    dumpInfo();

    dumpOrderingEdges();
    dumpUsageEdges();

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

void DotTraceVisualizer::dumpSourceText() {
    if (!sourceText) {
        return;
    }

    std::string formattedSourceText{};

    for (const auto chr : std::string_view{sourceText}) {
        switch (chr) {
            case '&' : formattedSourceText.append("&amp;");                 break;
            case '\"': formattedSourceText.append("&quot;");                break;
            case '\'': formattedSourceText.append("&apos;");                break;
            case '<' : formattedSourceText.append("&lt;");                  break;
            case '>' : formattedSourceText.append("&gt;");                  break;
            case ';' : formattedSourceText.append("; <br align=\"left\"/>");  break;
            default  : formattedSourceText.push_back(chr);                  break;
        }
    }

    dot.beginGraph("subgraph", "cluster_metadata");
    dot.beginGraph("subgraph", "cluster_source");
    dot.writeArg("label", "Source code");

    dot.writeNode("source_text")
        .label("%s", formattedSourceText.c_str())
        .arg("shape", "note")
        .arg("fontname", "Consolas");
    
    dot.endGraph();
    dot.endGraph();
}

void DotTraceVisualizer::dumpOrderingEdges() {
    unsigned lastNode = 0;

    while (lastNode < nodesCnt && !nodesPresent[lastNode]) {
        ++lastNode;
    }

    unsigned actionIdx = 1;
    for (unsigned node = lastNode + 1; node < nodesCnt; ++node) {
        if (!nodesPresent[node]) {
            continue;
        }

        dot.writefEdge(NODE_FMT, lastNode).constraint().to(NODE_FMT, node)
            .arg("style", style::edge_timeline_style)
            .arg("color", style::edge_timeline_color)
            .arg("weight", "16")
            .argf("xlabel", "%u", actionIdx++);

        lastNode = node;
    }
}

void DotTraceVisualizer::dumpPadding() {
    // If I remove this completely, I somehow get the
    // "newtrap: Trapezoid-table overflow 441" error,
    // which is, apperently, a known issue since 2020

    dot.beginGraph("", "");
    //dot.writeArg("rank", "same");
    dot.writeDefaultArgs("node")
        .arg("style", "invis");
    dot.writeNode("tmp_1");  // This, apparently, combats the problem somehow
    //dot.writeNode("pad_2");
    dot.endGraph();
    //dot.writeEdge("pad_1", "pad_2", true)
    //    .arg("style", "invis");
    //dot.writeEdge("pad_2", "node_0", true)
    //    .arg("style", "invis");
}

void DotTraceVisualizer::dumpUsageEdges() {
    for (const auto &edge : usageEdges) {
        writeEdge(edge.idxFrom, edge.idxTo, nullptr, /*edge.toInst ? "inst:w" : "other:w"*/ nullptr)
            .arg("color", style::edge_usage_color)
            .arg("style", style::edge_usage_style);
    }
}

void DotTraceVisualizer::logEntry(const Trace &trace, unsigned idx) {
    const TraceEntry &entry = trace.getEntries()[idx];
    
    switch (entry.op) {
    case TracedOp::FuncEnter: {
        // A new function was entered

        dot.beginGraph("subgraph", std::string("cluster_func_") + std::to_string(idx));

        // dot.writeArg("style", "filled");
        dot.writeArg("bgcolor", getFuncColor(entry.place.recursionDepth));
        dot.writeArg("labeljust", "l");
        // dot.writeArg("border", "1");

        auto html = dot.writeArgs().labelh();
        html.openTag("i");
        html.openTag("b");
        ofile.writef("%s()", entry.place.func.function_name());
        html.closeTag();
        html.closeTag();
        html.finish();

        nodesPresent[idx] = false;
    } break;
    
    case TracedOp::FuncLeave: {
        // A function was exited

        dot.endGraph();

        nodesPresent[idx] = false;
    } break;

    case TracedOp::DbgMsg: {
        writeNode(idx, true)
            .label("[dbg] <i>%s</i>", entry.opStr)
            .arg("style", "filled")
            .arg("fillcolor", "#9850b0");
    } break;

    case TracedOp::Ctor: {
        writeNodeUnary(idx, "<b>ctor</b>", "#40C040", entry.inst);
        
        vars.onCtor(idx, entry.inst);
    } break;

    case TracedOp::Dtor: {
        writeNodeUnary(idx, "<b>dtor</b>", "#C04040", entry.inst);

        writeEdge(vars[entry.inst].ctorOpIdx, idx)
            .arg("style", style::edge_lifespan_style)
            .arg("color", style::edge_lifespan_color);
        
        vars.onDtor(idx, entry.inst);
    } break;

    case TracedOp::Copy:
    case TracedOp::Move: {
        bool isCtor = !vars[entry.inst].isCreated();
        bool isCopy = entry.op == TracedOp::Copy;

        writeNodeBinary(idx, helpers::sprintfxx("<b>%s</b>%s", isCopy ? "COPY" : "MOVE",
                                                isCtor ? "" : "(=)").c_str(),
                        isCopy ? "#F00000" : "#F0F000", entry.inst, entry.other);
        
        if (isCopy) {
            ++stats.copies;
        } else {
            ++stats.moves;
        }

        if (isCtor) {
            vars.onCtor(idx, entry.inst);
        }

        vars.onChanged(idx, entry.inst, entry.other, 
                       [/*isCtor,*/ isCopy/*, &entry*/](std::string &expr, const std::string_view &otherExpr) {
            expr = helpers::sprintfxx("%s(%s)", isCopy ? "cp" : "mv", otherExpr.data());
        });
    } break;

    case TracedOp::Inplace:
    case TracedOp::Binary: {
        writeNodeBinary(idx, helpers::sprintfxx("<b>bin</b>(%s)", entry.opStr).c_str(),
                        "#6060C0", entry.inst, entry.other);

        vars.onChanged(idx, entry.inst, entry.other, [&entry](std::string &expr, const std::string_view &otherExpr) {
            // TODO: This might react weird due to string_view. Should probably limit length. Also in the other ones as well
            expr = helpers::sprintfxx("(%s %s %s)", expr.c_str(), entry.opStr, otherExpr.data());
        });
    } break;

    case TracedOp::Cmp: {
        writeNodeBinary(idx, helpers::sprintfxx("<b>cmp</b>(%s)", helpers::htmlEncode(entry.opStr).c_str()).c_str(),
                        "#6060C0", entry.inst, entry.other);

        vars.onUsed(idx, entry.inst, true);
        vars.onUsed(idx, entry.other, false);
    } break;

    case TracedOp::Unary: {
        writeNodeUnary(idx, helpers::sprintfxx("<b>un</b>(%s)", entry.opStr).c_str(),
                       "#6060C0", entry.inst);

        vars.onChanged(idx, entry.inst, [&entry](std::string &expr) {
            if (entry.other.isSet()) {
                // Postfix
                expr = helpers::sprintfxx("(%s%s)", expr.c_str(), entry.opStr);
            } else {
                // Prefix
                expr = helpers::sprintfxx("(%s%s)", entry.opStr, expr.c_str());
            }
        });
    } break;

    NODEFAULT
    }
}

std::fs::path DotTraceVisualizer::getTmpFileName(const std::fs::path &outputFile) {
    return std::fs::path{outputFile}.replace_extension("dot");
}

void DotTraceVisualizer::compileOutput(bool debug) const {
    std::fs::remove(outputFile);

    std::ostringstream cmd{};
    cmd << "dot -x -Tsvg " << tmpFile << " -o " << outputFile;
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

DotTraceVisualizer::_ArgsWriteProxy
DotTraceVisualizer::writeNode(unsigned entryIdx, bool box) {
    return std::move(
        dot.writefNode(NODE_FMT, entryIdx)
            .arg("shape", box ? "box" : "plain")
            .arg("fillcolor", "white")
            .arg("style", "filled")
    );
}

void DotTraceVisualizer::writeVarInfo(_HtmlLabelWriteProxy &html, const TraceEntry::VarInfo &info, const char *port) {
    constexpr unsigned DETAIL_LEVEL = 2;

    html.openTag("tr");
    {
        auto td = [&html]() {
            return std::move(
                html.openTag("td")
                    .arg("align", "left")
                    .arg("border", "1")
            );
        };

        {
            auto nameCell = td();

            if (port) {
                nameCell.arg("port", port);
            }
        }
        if (info.isUnnamed()) {
            ofile.writef(info.name, info.idx);
        } else {
            ofile.write(info.name);
        }
        html.closeTag();

        td();
        ofile.writef("#%u", info.idx);
        html.closeTag();

        td();
        if constexpr (DETAIL_LEVEL >= 1) {
            ofile.writef("%p:%u ", info.addr, ptrCells.get(info.addr));
        } else {
            ofile.writef("[:%u] ", ptrCells.get(info.addr));
        }
        html.closeTag();

        td();
        ofile.writef("=%s", info.valRepr.c_str());
        html.closeTag();
    }
    html.closeTag();

    if constexpr (DETAIL_LEVEL >= 2) {
        html.openTag("tr");
        html.openTag("td")
            .arg("align", "left")
            .arg("colspan", "4")
            .arg("border", "1");
        html.openTag("i");
        ofile.write(vars[info].expression);
        ofile.write(" ");  // Otherwise an empty expression is treated as a syntax error
        html.closeTag();
        html.closeTag();
        html.closeTag();
    }
}

void DotTraceVisualizer::_writeNodeAry(unsigned entryIdx, const char *name,
                                       const char *color, bool binary,
                                       const TraceEntry::VarInfo &infoInst,
                                       const TraceEntry::VarInfo &infoOther) {
    auto html = writeNode(entryIdx).labelh();

    // border="0" cellpadding="2" cellspacing="0" cellborder="0"
    html.openTag("table")
        .arg("border", "0")
        .arg("cellspacing", "0")
        .arg("cellborder", "1");
    html.openTag("tr");
    html.openTag("td")
        .arg("align", "left")
        .arg("colspan", "4")
        .arg("border", "1")
        .arg("bgcolor", color);
    ofile.writef("%s", name);
    html.closeTag("td");
    html.closeTag("tr");
    writeVarInfo(html, infoInst, "inst");
    if (binary) {
        writeVarInfo(html, infoOther, "other");
    }
    html.closeTag("table");

    auto node = html.finish();
}

DotTraceVisualizer::_ArgsWriteProxy
DotTraceVisualizer::writeEdge(unsigned idxFrom, unsigned idxTo) {
    return dot.writefEdge(NODE_FMT, idxFrom)
                      .to(NODE_FMT, idxTo);
}

DotTraceVisualizer::_ArgsWriteProxy
DotTraceVisualizer::writeEdge(unsigned idxFrom, unsigned idxTo,
                              const char *portFrom, const char *portTo) {
    std::string from = portFrom ? 
        helpers::sprintfxx(NODE_PORT_FMT, idxFrom, portFrom) :
        helpers::sprintfxx(NODE_FMT,      idxFrom);
    
    std::string to = portTo ? 
        helpers::sprintfxx(NODE_PORT_FMT, idxTo, portTo) :
        helpers::sprintfxx(NODE_FMT,      idxTo);

    return dot.writeEdge(from, to);
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

    onUsed(opIdx, info, true);
    
    state.lastChangeOpIdx = opIdx;
    state.expression = info.buildNameStr();
}

void DotTraceVisualizer::Vars::onUsed(unsigned opIdx, const TraceEntry::VarInfo &info, bool asInst) {
    auto &state = getVarState(info);

    constexpr bool FROM_CHANGE = false;

    unsigned prevOpIdx = FROM_CHANGE ? state.lastChangeOpIdx : state.lastUseOpIdx;
    state.lastUseOpIdx = opIdx;

    if (prevOpIdx == state.BAD_IDX || prevOpIdx == opIdx) {
        return;
    }

    // TODO: Edge
    drawUseEdge(prevOpIdx, opIdx, asInst);
}

void DotTraceVisualizer::Vars::onDtor(unsigned opIdx, const TraceEntry::VarInfo &info) {
    auto &state = getVarState(info);

    // onUsed(opIdx, info, true);
    
    state.dtorOpIdx = opIdx;
    state.lastChangeOpIdx = opIdx;
}

void DotTraceVisualizer::Vars::onChanged(unsigned opIdx, const TraceEntry::VarInfo &info,
                                         std::function<void (std::string &curExpr)> operation) {
    auto &state = getVarState(info);

    onUsed(opIdx, info, true);

    state.lastChangeOpIdx = opIdx;
    operation(state.expression);
}

void DotTraceVisualizer::Vars::onChanged(unsigned opIdx, const TraceEntry::VarInfo &info,
                                         const TraceEntry::VarInfo &otherInfo,
                                         std::function<void (std::string &curExpr,
                                         const std::string_view &otherExpr)> operation) {
    auto &state      = getVarState(info);
    auto &stateOther = getVarState(otherInfo);

    onUsed(opIdx, info, true);
    state.lastChangeOpIdx = opIdx;

    onUsed(opIdx, otherInfo, false);

    operation(state.expression, stateOther.expression);
}


#pragma endregion Vars

#pragma endregion DotTraceVisualizer

