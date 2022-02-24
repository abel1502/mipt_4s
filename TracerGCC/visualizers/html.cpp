#include "html.h"
#include <cstdarg>
#include <sstream>


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
    if (!disabled) {
        writeEnd();
    }
}

HtmlTag::WriteProxy::WriteProxy(WriteProxy &&other) :
    ofile{other.ofile}, isInline{other.isInline} {
    
    other.disabled = true;
}

HtmlTag::WriteProxy &HtmlTag::WriteProxy::arg(const char *name, const std::string_view &value) {
    assert(!disabled);

    ofile.write(" ");
    ofile.write(name);
    ofile.write("=\"");
    ofile.write(value);
    ofile.write("\"");

    return *this;
}

HtmlTag::WriteProxy &HtmlTag::WriteProxy::arg(const char *name) {
    assert(!disabled);

    ofile.write(" ");
    ofile.write(name);

    return *this;
}

HtmlTag::WriteProxy &HtmlTag::WriteProxy::argf(const char *name, const char *fmt, ...) {
    assert(!disabled);

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
    assert(!disabled);

    ofile.write(isInline ? "/>" : ">");
}
#pragma endregion HtmlTag


#pragma region HtmlTraceVisualizer
void HtmlTraceVisualizer::visualize(const Trace &trace) {
    stats = {};

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

    openTag("script");
    dumpScript();
    closeTag("script");

    openTag("pre");

    logDbgMsg("Varibale info format: name (#index)[address:ptr cell](val=value)");
    logDbgMsg("For example: counter (#5)[0xffff0000:2](val=123)");
}

void HtmlTraceVisualizer::endLog() {
    if (tagStack.size() == 2) {
        std::ostringstream buf{};
        buf << "Total copies: " << stats.copies;
        logDbgMsg(buf.view());

        buf.str(""); buf.clear();
        buf << "Total moves:  " << stats.moves;
        logDbgMsg(buf.view());

        stats = {};
    } else {
        logDbgMsg("Warning: abnormal termination");
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
        // "    color: #00F000;\n"
        "    color: #E0E000;\n"
        "    font-weight: bold;\n"
        "}\n"
        ".label-bin {\n"
        "    color: #6060C0;\n"
        "}\n"
        ".label-un {\n"
        "    color: #6060C0;\n"
        "}\n"
        "\n"
        ".opstr {\n"
        "    color: white;\n"
        "    font-weight: normal;\n"
        "}\n"
        "\n"
        ".var-info-idx {\n"
        "    color: orange;\n"
        "}\n"
        "\n"
        ".var-info-cell {\n"
        "    color: #60e0f0;\n"
        "}\n"
        "\n"
        "@keyframes blink {\n"
        "    from { "/*background: transparent;*/" }\n"
        "    50%  { background: #f0c000; }\n"
        "    to   { "/*background: transparent;*/" }\n"
        "}\n"
        "\n"
        ".main-body .label a {\n"
        "    color: inherit;\n"
        "    text-decoration: none;\n"
        "}\n"
        "\n"
    );
}

void HtmlTraceVisualizer::dumpScript() {
    ofile.write(
        "window.addEventListener('load', (event) => {\n"
        "    document.querySelectorAll('a[href^=\"#\"]').forEach(anchor => {\n"
        "        anchor.addEventListener('click', function (e) {\n"
        "            e.preventDefault();\n"
        "            \n"
        "            let target = document.querySelector(this.getAttribute('href'));\n"
        "            target.scrollIntoView({ behavior: 'smooth', block: 'nearest' });\n"
        "            target.style.animation = 'none';\n"
        "            target.offsetHeight;  // Should trigger a reflow\n"
        "            target.style.animation = 'blink 1.5s ease 0.1s 1';\n"
        "        });\n"
        "    });\n"
        "});\n"
        "\n"
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
        logDbgMsg(entry.opStr);
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

        bool isCtor = !std::string_view(entry.opStr).ends_with('=');
        bool isCopy = entry.op == TracedOp::Copy;

        if (isCopy) {
            ++stats.copies;
        } else {
            ++stats.moves;
        }

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

    NODEFAULT
    }
}

void HtmlTraceVisualizer::logVarInfo(const TraceEntry::VarInfo &info) {
    // TODO: Handle default name specially
    if (info.isUnnamed()) {
        ofile.writef(info.name, info.idx);
    } else {
        ofile.write(info.name);
    }

    openTag("span").arg("class", "var-info");

    // ofile.writef("(#%u)[%p:%u](val=%s)", info.idx, info.addr, ptrCells.get(info.addr), info.valRepr.c_str());

    ofile.write("(");
    openTag("a")
        .arg("class", "var-info-idx")
        .argf("href", "#label-ctor-%u", info.idx);
    ofile.writef("#%u", info.idx);
    closeTag();
    ofile.writef(")[%p:", info.addr);
    openTag("span").arg("class", "var-info-cell");
    ofile.writef("%u", ptrCells.get(info.addr));
    closeTag();
    ofile.writef("](val=%s)", info.valRepr.c_str());

    closeTag();
}

void HtmlTraceVisualizer::logIndent() {
    ofile.indent(getIndent());
}

void HtmlTraceVisualizer::logDbgMsg(const std::string_view &msg) {
    logIndent();

    openTag("span").arg("class", "dbg-msg");
    ofile.write("[dbg] ");
    openTag("i");
    ofile.write(msg);
    closeTag();
    closeTag();

    ofile.writeln();
}


#pragma endregion HtmlTraceVisualizer

