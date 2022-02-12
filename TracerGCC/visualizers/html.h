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
    // You should avoid saving the result into 
    WriteProxy writeOpen(OutputFile &ofile);
    void writeClose(OutputFile &ofile);
    WriteProxy writeInline(OutputFile &ofile);

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

    void beginLog();
    void endLog();

};
