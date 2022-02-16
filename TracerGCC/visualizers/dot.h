#pragma once
#include "../general.h"
#include "visualizer.h"
#include "../helpers.h"
#include <vector>
#include <map>


class DotOutput {
protected:
    class ArgsWriteProxy;
    class EdgeFmtWriteProxy;

public:
    inline DotOutput(OutputFile &ofile_) :
        ofile{ofile_} {}

    ArgsWriteProxy writeNode(const std::string_view &name);
    ArgsWriteProxy writefNode(const char *fmt, ...);

    ArgsWriteProxy writeEdge(const std::string_view &from, const std::string_view &to);
    [[nodiscard]] EdgeFmtWriteProxy writefEdge(const char *fmtFrom, ...);

    void beginGraph(const char *type, const std::string_view &name, bool strict = false);
    void endGraph();

    ArgsWriteProxy writeDefaultArgs(const char *type);

    void writeArg(const char *name, const std::string_view &value);

    void writeArg(const char *name, const char *fmt, ...);

protected:
    OutputFile &ofile;


    class ArgsWriteProxy {
    public:
        ArgsWriteProxy &arg(const char *name, const std::string_view &value);

        ArgsWriteProxy &argf(const char *name, const char *fmt, ...);

        ArgsWriteProxy &argfv(const char *name, const char *fmt, va_list args);

        template <typename ... As>
        ArgsWriteProxy &labelt(const std::fs::path &tplPath, bool html, As &&... args)  {
            std::ifstream file{tplPath, file.binary | file.in};  // Binary to avoid size changes

            REQUIRE(!file.bad());

            file.seekg(0, file.end);
            size_t size = file.tellg();
            file.seekg(0, file.beg);

            std::string buf{};
            buf.resize(size);
            file.read(buf.data(), size);

            ofile.write("label");
            ofile.write(html ? "=<" : "=\"");
            ofile.writef(buf.c_str(), std::forward<As>(args)...);
            ofile.write(html ? ">; " : "\"; ");

            return *this;
        }

        ~ArgsWriteProxy();

    protected:
        OutputFile &ofile;
        bool braces = true;


        ArgsWriteProxy(OutputFile &ofile_, bool braces_ = true);

        void writeEnd() const;


        friend DotOutput;
        friend EdgeFmtWriteProxy;
    };

    class EdgeFmtWriteProxy {
    public:
        ~EdgeFmtWriteProxy();

        ArgsWriteProxy to(const char *fmtTo, ...);

    protected:
        OutputFile &ofile;


        EdgeFmtWriteProxy(OutputFile &ofile_);

        void writeEnd() const;


        friend DotOutput;
    };

};


class DotTraceVisualizer : public TraceVisualizer {
public:
    inline DotTraceVisualizer(const std::fs::path &outputFile) :
        TraceVisualizer(getTmpFileName(outputFile)),
        tmpFile{getTmpFileName(outputFile)},
        outputFile{outputFile},
        dot{ofile} {}

    virtual void visualize(const Trace &trace) override;

    virtual ~DotTraceVisualizer() override;

protected:
    std::fs::path tmpFile;
    std::fs::path outputFile;
    DotOutput dot;
    helpers::CounterMap<const void *, unsigned> ptrCells{};


    void beginLog();
    void endLog();

    void logVarInfo(const TraceEntry::VarInfo &info);
    void logEntry(const Trace &trace, unsigned idx);

    static std::fs::path getTmpFileName(const std::fs::path &outputFile);

    void compileOutput(bool debug = false) const;

};
