#include "dot.h"
#include <sstream>
#include <cstdarg>


#pragma region DotOutput
DotOutput::ArgsWriteProxy DotOutput::writeNode(const std::string_view &name) {
    ofile.write("\"");
    ofile.write(name);
    ofile.write("\" ");

    return ArgsWriteProxy(ofile);
}

DotOutput::ArgsWriteProxy DotOutput::writefNode(const char *fmt, ...) {
    ofile.write("\"");
    va_list args{};
    va_start(args, fmt);
    ofile.writefv(fmt, args);
    va_end(args);
    ofile.write("\" ");

    return ArgsWriteProxy(ofile);
}

DotOutput::ArgsWriteProxy DotOutput::writeEdge(const std::string_view &from, const std::string_view &to) {
    ofile.write("\"");
    ofile.write(from);
    ofile.write("\" -> \"");
    ofile.write(to);
    ofile.write("\" ");

    return ArgsWriteProxy(ofile);
}

[[nodiscard]] DotOutput::EdgeFmtWriteProxy DotOutput::writefEdge(const char *fmtFrom, ...) {
    ofile.write("\"");
    va_list args{};
    va_start(args, fmtFrom);
    ofile.writefv(fmtFrom, args);
    va_end(args);
    ofile.write("\" -> ");

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

void DotOutput::writeArg(const char *name, const std::string_view &value) {
    ArgsWriteProxy{ofile, false}.arg(name, value);
    ofile.write(";\n");
}

void DotOutput::writeArg(const char *name, const char *fmt, ...) {
    va_list args{};
    va_start(args, fmt);
    ArgsWriteProxy{ofile, false}.argfv(name, fmt, args);
    va_end(args);
    ofile.write(";\n");
}


#pragma region ArgsWriteProxy
DotOutput::ArgsWriteProxy::~ArgsWriteProxy() {
    writeEnd();
}

DotOutput::ArgsWriteProxy::ArgsWriteProxy(OutputFile &ofile_, bool braces_) :
    ofile{ofile_}, braces{braces_} {

    if (braces) {
        ofile.write("[");
    }
}

void DotOutput::ArgsWriteProxy::writeEnd() const {
    if (braces) {
        ofile.write("]\n");
    }
}

DotOutput::ArgsWriteProxy &DotOutput::ArgsWriteProxy::arg(const char *name, const std::string_view &value) {
    ofile.write(name);
    ofile.write("=\"");
    ofile.write(value);
    ofile.write("\"; ");

    return *this;
}

DotOutput::ArgsWriteProxy &DotOutput::ArgsWriteProxy::argf(const char *name, const char *fmt, ...) {
    va_list args{};
    va_start(args, fmt);
    argfv(name, fmt, args);
    va_end(args);

    return *this;
}

DotOutput::ArgsWriteProxy &DotOutput::ArgsWriteProxy::argfv(const char *name, const char *fmt, va_list args) {
    ofile.write(name);
    ofile.write("=\"");
    ofile.writefv(fmt, args);
    ofile.write("\" ");

    return *this;
}
#pragma endregion ArgsWriteProxy

#pragma region EdgeFmtWriteProxy
DotOutput::EdgeFmtWriteProxy::~EdgeFmtWriteProxy() {
    writeEnd();
}

DotOutput::ArgsWriteProxy DotOutput::EdgeFmtWriteProxy::to(const char *fmtTo, ...) {
    ofile.write("\"");
    va_list args{};
    va_start(args, fmtTo);
    ofile.writefv(fmtTo, args);
    va_end(args);
    ofile.write("\" ");

    return ArgsWriteProxy(ofile);
}

DotOutput::EdgeFmtWriteProxy::EdgeFmtWriteProxy(OutputFile &ofile_) :
    ofile{ofile_} {}

void DotOutput::EdgeFmtWriteProxy::writeEnd() const {}
#pragma endregion EdgeFmtWriteProxy

#pragma endregion DotOutput


#pragma region DotTraceVisualizer
void DotTraceVisualizer::visualize(const Trace &trace) {
    beginLog();

    const auto &entries = trace.getEntries();

    for (unsigned i = 0; i < entries.size(); ++i) {
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

    // TODO: A header
    dot.writeNode("info")
        .labelt(std::fs::current_path().append("../tpl/label_info.tpl"), true);
}

void DotTraceVisualizer::endLog() {
    dot.endGraph();

    ofile.forceFlush();
    ofile.close();
}

void DotTraceVisualizer::logEntry(const Trace &trace, unsigned idx) {
    const TraceEntry &entry = trace.getEntries()[idx];
    
    switch (entry.op) {
    #if 0
    case TracedOp::FuncEnter: {
        // A new function was entered

        logIndent();
        openTag("span").arg("class", "func-info");
        ofile.write(entry.place.func.function_name());
        ofile.write(" {\n");
        closeTag();

        recursionDepth = entry.place.recursionDepth;
    } break;
    
    case TracedOp::FuncLeave: {
        // A function was exited

        recursionDepth = entry.place.recursionDepth;

        logIndent();
        openTag("span").arg("class", "func-info");
        ofile.write("}\n");
        closeTag();
    } break;

    case TracedOp::DbgMsg: {
        logIndent();

        openTag("span").arg("class", "dbg-msg");
        ofile.write("[dbg] ");
        openTag("i");
        ofile.write(entry.opStr);
        closeTag();
        closeTag();

        ofile.writeln();
    } break;

    case TracedOp::Ctor: {
        logIndent();

        openTag("span")
            .arg("class", "label label-ctor")
            .argf("id", "label-ctor-%u", entry.inst.idx);
        openTag("a").argf("href", "#label-dtor-%u", entry.inst.idx);
        ofile.write("ctor");
        closeTag();
        closeTag();
        logVarInfo(entry.inst);

        ofile.writeln();

        onVarCreated(entry.inst.idx);
    } break;

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
    default:
        break;

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


#pragma endregion DotTraceVisualizer

