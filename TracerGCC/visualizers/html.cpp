#include "html.h"
#include <cstdarg>


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

HtmlTag::WriteProxy &HtmlTag::WriteProxy::arg(const char *name) {
    ofile.write(" ");
    ofile.write(name);

    return *this;
}

HtmlTag::WriteProxy &HtmlTag::WriteProxy::argf(const char *name, const char *fmt, ...) {
    ofile.write(" ");
    ofile.write(name);
    ofile.write("=\"");
    va_list args{};
    va_start(args, fmt);
    ofile.writefv(fmt, args);
    va_end(args);
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

    openTag("body").arg("class", "main-body");

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
        ".main-body {\n"
        "    background-color: black;\n"
        "    color: white;\n"
        "    font-size: 18px;\n"
        "}\n"
        "\n"
        ".var-info {\n"
        "    font-size: 0.6em;\n"
        "}\n"
        "\n"
        ".func-info {\n"
        "    font-style: italic;\n"
        "}\n"
        "\n"
        ".dbg-msg {\n"
        "    color: #808080;\n"
        "}\n"
        "\n"
        ".label {\n"
        "    display: inline-block;\n"
        "    width: 10ch;\n"
        "}\n"
        "\n"
        ".label-ctor {\n"
        "    color: #40C040;\n"
        "}\n"
        ".label-dtor {\n"
        "    color: #C04040;\n"
        "}\n"
        ".label-copy {\n"
        "    color: #F00000;\n"
        "    font-weight: bold;\n"
        "}\n"
        ".label-move {\n"
        "    color: #00F000;\n"
        "    font-weight: bold;\n"
        "}\n"
        ".label-bin {\n"
        "    color: #6060C0;\n"
        "}\n"
        ".label-un {\n"
        "    color: #6060C0;\n"
        "}\n"
        "\n"
        ".var-info-idx {\n"
        "    color: orange;\n"
        "}\n"
        "\n"
        ".var-info-cell {\n"
        "    color: #60e0f0;\n"
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
    switch (entry.op) {
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

        openTag("span").arg("class", "label label-ctor");
        ofile.writef("ctor");
        closeTag();
        logVarInfo(entry.inst);

        ofile.writeln();
    } break;

    case TracedOp::Dtor: {
        logIndent();

        openTag("span").arg("class", "label label-dtor");
        ofile.writef("dtor");
        closeTag();
        logVarInfo(entry.inst);

        ofile.writeln();
    } break;

    case TracedOp::Copy: {
        logIndent();

        openTag("span").arg("class", "label label-copy");
        ofile.writef("COPY");
        closeTag();

        logVarInfo(entry.inst);
        ofile.write(" from ");
        logVarInfo(entry.other);

        ofile.writeln();
    } break;

    case TracedOp::Move: {
        logIndent();

        openTag("span").arg("class", "label label-move");
        ofile.writef("MOVE");
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
        
        openTag("span").arg("class", "label label-bin");
        ofile.write("bin");
        closeTag();
        ofile.writef("(%s)", entry.opStr);
        logVarInfo(entry.inst);
        ofile.write(" with ");
        logVarInfo(entry.other);

        ofile.writeln();
    } break;

    case TracedOp::Unary: {
        logIndent();
        
        openTag("span").arg("class", "label label-un");
        ofile.write("un");
        closeTag();
        const char *fmt = "(%s)";  // Prefix version
        if (entry.other.isSet()) {  // Postfix version
            fmt = "(%s.)";
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
    openTag("span").arg("class", "var-info");

    // ofile.writef("(#%u)[%p:%u](val=%s)", info.idx, info.addr, getPtrCell(info.addr), info.valRepr.c_str());

    ofile.write("(");
    openTag("span").arg("class", "var-info-idx");
    ofile.writef("#%u", info.idx);
    closeTag();
    ofile.writef(")[%p:", info.addr);
    openTag("span").arg("class", "var-info-cell");
    ofile.writef("%u", getPtrCell(info.addr));
    closeTag();
    ofile.writef("](val=%s)", info.valRepr.c_str());

    closeTag();
}

void HtmlTraceVisualizer::logIndent() {
    ofile.indent(getIndent());
}


#pragma endregion HtmlTraceVisualizer

