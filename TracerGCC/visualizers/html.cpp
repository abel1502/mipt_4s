#include "html.h"


#pragma region HtmlTag
HtmlTag::WriteProxy HtmlTag::writeOpen(OutputFile &ofile) const {
    ofile.write("<");
    ofile.write(tag);
    
    return WriteProxy{ofile, false};
}

void HtmlTag::writeClose(OutputFile &ofile) const {
    ofile.write("</");
    ofile.write(tag);
    ofile.write(">");
}

HtmlTag::WriteProxy HtmlTag::writeInline(OutputFile &ofile) const {
    ofile.write("<");
    ofile.write(tag);
    
    return WriteProxy{ofile, true};
}


HtmlTag::WriteProxy::WriteProxy(OutputFile &ofile_, bool isInline_) :
    ofile{ofile_}, isInline{isInline_} {}

HtmlTag::WriteProxy::~WriteProxy() {
    writeEnd();
}

HtmlTag::WriteProxy &HtmlTag::WriteProxy::arg(const char *name, const std::string_view &value) {
    ofile.write(" ");
    ofile.write(name);
    ofile.write("=\"");
    ofile.write(value);
    ofile.write("\"");

    return *this;
}

void HtmlTag::WriteProxy::writeEnd() const {
    ofile.write(isInline ? "/>" : ">");
}
#pragma endregion HtmlTag


#pragma region HtmlTraceVisualizer
void HtmlTraceVisualizer::visualize(const Trace &trace) {
    beginLog();

    for (const auto &entry : trace.getEntries()) {
        logEntry(entry);
    }

    endLog();
}

HtmlTraceVisualizer::~HtmlTraceVisualizer() {
    if (!tagStack.empty()) {
        // Just to make sure everything is at least formally correct
        endLog();
    }
}

void HtmlTraceVisualizer::beginLog() {
    ofile.open();

    openTag("body").arg("class", "main_body");

    openTag("style");
    dumpStyle();
    closeTag("style");

    openTag("pre");
}

void HtmlTraceVisualizer::endLog() {
    if (tagStack.size() != 2) {
        DBG("Warning: abnormal termination");
    }
    
    while (!tagStack.empty()) {
        closeTag();
    }

    ofile.forceFlush();
    ofile.close();
}

void HtmlTraceVisualizer::dumpStyle() {
    ofile.write(
        ".main_body {\n"
        "    background-color: black;\n"
        "    color: white;\n"
        "    font-size: 18px;\n"
        "}\n"
        "\n"
        ".var_info {\n"
        "    font-size: 0.6em;\n"
        "}\n"
    );
}

const HtmlTag &HtmlTraceVisualizer::pushTag(const char *tag) {
    return tagStack.emplace_back(tag);
}

HtmlTag HtmlTraceVisualizer::popTag() {
    HtmlTag result = tagStack.back();

    tagStack.pop_back();

    return result;
}

void HtmlTraceVisualizer::logEntry(const TraceEntry &entry) {
    // constexpr unsigned LABEL_WIDTH = 10;  // Just for reference

    switch (entry.op) {
    case TracedOp::FuncEnter: {
        // A new function was entered

        logIndent();
        openTag("i");
        ofile.write(entry.place.func.function_name());
        ofile.write(" {\n");
        closeTag();

        recursionDepth = entry.place.recursionDepth;
    } break;
    
    case TracedOp::FuncLeave: {
        // A function was exited

        recursionDepth = entry.place.recursionDepth;

        logIndent();
        openTag("i");
        ofile.write("}\n");
        closeTag();
    } break;

    case TracedOp::DbgMsg: {
        logIndent();

        openTag("font").arg("color", "#808080");
        ofile.write("[dbg] " /*"    "*/);
        openTag("i");
        ofile.write(entry.opStr);
        closeTag();
        closeTag();

        ofile.writeln();
    } break;

    case TracedOp::Ctor: {
        logIndent();

        openTag("font").arg("color", "#40C040");
        ofile.write("ctor      ");
        closeTag();
        logVarInfo(entry.inst);

        ofile.writeln();
    } break;

    case TracedOp::Dtor: {
        logIndent();

        openTag("font").arg("color", "#C04040");
        ofile.write("dtor      ");
        closeTag();
        logVarInfo(entry.inst);

        ofile.writeln();
    } break;

    case TracedOp::Copy: {
        logIndent();

        openTag("font").arg("color", "#F00000");
        openTag("b");
        ofile.write("COPY      ");
        closeTag();
        closeTag();

        logVarInfo(entry.inst);
        ofile.write(" from ");
        logVarInfo(entry.other);

        ofile.writeln();
    } break;

    case TracedOp::Move: {
        logIndent();

        openTag("font").arg("color", "#00F000");
        openTag("b");
        ofile.write("MOVE      ");
        closeTag();
        closeTag();

        logVarInfo(entry.inst);
        ofile.write(" from ");
        logVarInfo(entry.other);

        ofile.writeln();
    } break;

    case TracedOp::Inplace:
    case TracedOp::Binary:
    case TracedOp::Cmp: {
        logIndent();
        
        openTag("font").arg("color", "#6060C0");
        ofile.write("bin");
        closeTag();
        ofile.writef("(%-3s)  ", entry.opStr);
        logVarInfo(entry.inst);
        ofile.write(" with ");
        logVarInfo(entry.other);

        ofile.writeln();
    } break;

    case TracedOp::Unary: {
        logIndent();
        
        openTag("font").arg("color", "#6060C0");
        ofile.write("un");
        closeTag();
        const char *fmt = "(%-2s)     ";  // Prefix version
        if (entry.other.isSet()) {  // Postfix version
            fmt = "(%-2s.)   ";
        }
        ofile.writef(fmt, entry.opStr);
        logVarInfo(entry.inst);

        ofile.writeln();
    } break;

    NODEFAULT
    }
}

void HtmlTraceVisualizer::logVarInfo(const TraceEntry::VarInfo &info) {
    // TODO: Handle default name specially
    ofile.write(info.name);
    openTag("span").arg("class", "var_info");
    ofile.writef("(#%u)[%p](val=%s)", info.idx, info.addr, info.valRepr.c_str());
    closeTag();
}

void HtmlTraceVisualizer::logIndent() {
    ofile.indent(getIndent());
}


#pragma endregion HtmlTraceVisualizer

