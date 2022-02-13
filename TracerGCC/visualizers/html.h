#pragma once
#include "../general.h"
#include "visualizer.h"
#include "../stdfs.h"
#include <fstream>
#include <vector>


class HtmlTag {
protected:
    class WriteProxy;

public:
    inline HtmlTag(const char *tag_) :
        tag{tag_} {}

    // You should avoid saving the results into variables
    WriteProxy writeOpen(OutputFile &ofile) const;
    void writeClose(OutputFile &ofile) const;
    WriteProxy writeInline(OutputFile &ofile) const;

    inline const char *getTag() const { return tag; }

protected:
    const char *tag = "";


    class WriteProxy {
    public:
        WriteProxy &arg(const char *name, const std::string_view &value);

        ~WriteProxy();

    protected:
        OutputFile &ofile;
        bool isInline = false;


        WriteProxy(OutputFile &ofile_, bool isInline_ = false);

        void writeEnd() const;


        friend HtmlTag;
    };

};


class HtmlTraceVisualizer : public TraceVisualizer {
public:
    inline HtmlTraceVisualizer(const std::fs::path &outputFile, bool cumulative = false) :
        TraceVisualizer(outputFile, cumulative) {}

    virtual void visualize(const Trace &trace) override;

    virtual ~HtmlTraceVisualizer() override;

protected:
    std::vector<HtmlTag> tagStack{};
    unsigned recursionDepth = 0;

    void beginLog();
    void endLog();

    void dumpStyle();

    const HtmlTag &pushTag(const char *tag);
    HtmlTag popTag();

    inline auto openTag(const char *tag);
    inline auto inlineTag(const char *tag);
    inline void closeTag(const char *tag);
    inline void closeTag();

    inline unsigned getIndent() const {
        return recursionDepth * 4;
    }

    void logEntry(const TraceEntry &entry);
    void logVarInfo(const TraceEntry::VarInfo &info);
    void logIndent();

};


inline auto HtmlTraceVisualizer::openTag(const char *tag) {
    return pushTag(tag).writeOpen(ofile);
}

inline auto HtmlTraceVisualizer::inlineTag(const char *tag) {
    return pushTag(tag).writeInline(ofile);
}

inline void HtmlTraceVisualizer::closeTag(const char *tag) {
    REQUIRE(tagStack.back().getTag() == std::string_view{tag});

    closeTag();
}

inline void HtmlTraceVisualizer::closeTag() {
    popTag().writeClose(ofile);
}
