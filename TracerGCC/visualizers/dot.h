#pragma once
#include "../general.h"
#include "visualizer.h"
#include "../helpers.h"
#include "html.h"  // For html tag output
#include <vector>
#include <map>
#include <functional>


#pragma region DotOutput
class DotOutput {
protected:
    class ArgsWriteProxy;
    class EdgeFmtWriteProxy;
    class HtmlLabelWriteProxy;

public:
    inline DotOutput(OutputFile &ofile_) :
        ofile{ofile_} {}

    ArgsWriteProxy writeNode(const std::string_view &name);
    ArgsWriteProxy writefNode(const char *fmt, ...);

    ArgsWriteProxy writeEdge(const std::string_view &from, const std::string_view &to, bool constraint = false);
    [[nodiscard]] EdgeFmtWriteProxy writefEdge(const char *fmtFrom, ...);

    void beginGraph(const char *type, const std::string_view &name, bool strict = false);
    void endGraph();

    ArgsWriteProxy writeDefaultArgs(const char *type);

    void writeArg(const char *name, const std::string_view &value);

    void writeArg(const char *name, const char *fmt, ...);

    static std::string readTpl(const std::string_view &name);

protected:
    OutputFile &ofile;


    class ArgsWriteProxy {
    public:
        ArgsWriteProxy(const ArgsWriteProxy &other) = delete;
        ArgsWriteProxy &operator=(const ArgsWriteProxy &other) = delete;

        ArgsWriteProxy(ArgsWriteProxy &&other);
        ArgsWriteProxy &operator=(ArgsWriteProxy &&other) = delete;

        ArgsWriteProxy &arg(const char *name, const std::string_view &value);

        ArgsWriteProxy &argf(const char *name, const char *fmt, ...);

        ArgsWriteProxy &argfv(const char *name, const char *fmt, va_list args);

        /// The label() and labelt() functions treat their data as html.
        /// If you want a plaintext label, use arg("label", ...) instead
        template <typename ... As>
        ArgsWriteProxy &label(const char *fmt, As &&... args) {
            ofile.write("label");
            ofile.write("=<");
            ofile.writef(fmt, std::forward<As>(args)...);
            ofile.write(">; ");

            return *this;
        }

        template <typename ... As>
        ArgsWriteProxy &labelt(const std::string_view &tpl, As &&... args)  {
            return label(readTpl(tpl).c_str(), std::forward<As>(args)...);
        }

        HtmlLabelWriteProxy labelh();

        ~ArgsWriteProxy();

    protected:
        OutputFile &ofile;
        bool braces = true;
        bool disabled = false;


        ArgsWriteProxy(OutputFile &ofile_, bool braces_ = true);

        void writeEnd() const;


        friend DotOutput;
        friend EdgeFmtWriteProxy;
    };

    class EdgeFmtWriteProxy {
    public:
        EdgeFmtWriteProxy(const EdgeFmtWriteProxy &other) = delete;
        EdgeFmtWriteProxy &operator=(const EdgeFmtWriteProxy &other) = delete;

        EdgeFmtWriteProxy(EdgeFmtWriteProxy &&other);
        EdgeFmtWriteProxy &operator=(EdgeFmtWriteProxy &&other) = delete;

        ~EdgeFmtWriteProxy();

        inline EdgeFmtWriteProxy &constraint(bool value = true) {
            willConstraint = value;

            return *this;
        }

        ArgsWriteProxy to(const char *fmtTo, ...);

    protected:
        OutputFile &ofile;
        bool willConstraint = false;
        bool disabled = false;


        EdgeFmtWriteProxy(OutputFile &ofile_);

        void writeEnd() const;


        friend DotOutput;
    };

    class HtmlLabelWriteProxy {
    public:
        HtmlLabelWriteProxy(const HtmlLabelWriteProxy &other) = delete;
        HtmlLabelWriteProxy &operator=(const HtmlLabelWriteProxy &other) = delete;

        HtmlLabelWriteProxy(HtmlLabelWriteProxy &&other);
        HtmlLabelWriteProxy &operator=(HtmlLabelWriteProxy &&other) = delete;

        ArgsWriteProxy finish();

        const HtmlTag &pushTag(const char *tag);
        HtmlTag popTag();

        inline auto openTag(const char *tag);
        inline auto inlineTag(const char *tag);
        inline void closeTag(const char *tag);
        inline void closeTag();

    protected:
        ArgsWriteProxy parent;
        OutputFile &ofile;
        std::vector<HtmlTag> tagStack{};
        bool disabled = false;


        HtmlLabelWriteProxy(ArgsWriteProxy &&parent_, OutputFile &ofile_);

        void writeEnd();


        friend DotOutput;
        friend ArgsWriteProxy;
    };

};
#pragma endregion DotOutput


class DotTraceVisualizer : public TraceVisualizer {
#pragma region structs
protected:
    struct VarState {
        static constexpr unsigned BAD_IDX = -1u;

        unsigned ctorOpIdx = BAD_IDX;
        unsigned lastUseOpIdx = BAD_IDX;
        unsigned lastChangeOpIdx = BAD_IDX;
        unsigned dtorOpIdx = BAD_IDX;

        const void *lastAddr = nullptr;

        std::string expression{""};


        VarState() = default;
    };

    class Vars {
    public:
        VarState &getVarState(unsigned varIdx);
        inline VarState &getVarState(const TraceEntry::VarInfo &info) { return getVarState(info.idx); }
        inline VarState &operator[](unsigned varIdx) { return getVarState(varIdx); }
        inline VarState &operator[](const TraceEntry::VarInfo &info) { return getVarState(info); }

        void onCtor(unsigned opIdx, const TraceEntry::VarInfo &info);
        void onUsed(unsigned opIdx, const TraceEntry::VarInfo &info);
        void onDtor(unsigned opIdx, const TraceEntry::VarInfo &info);

        void onChanged(unsigned opIdx, const TraceEntry::VarInfo &info,
                       std::function<void (std::string &curExpr)> operation);
        void onChanged(unsigned opIdx, const TraceEntry::VarInfo &info,
                       const TraceEntry::VarInfo &otherInfo,
                       std::function<void (std::string &curExpr, const std::string_view &otherExpr)> operation);

    protected:
        std::vector<VarState> vars{};

    };
#pragma endregion structs

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
    struct {
        unsigned copies = 0;
        unsigned moves = 0;
    } stats{};
    unsigned nodesCnt = 0;
    std::vector<bool> nodesPresent{};
    Vars vars{};

    struct style {
        static constexpr std::string_view edge_timeline_color = "red";
        static constexpr std::string_view edge_timeline_style = "solid";

        static constexpr std::string_view edge_lifespan_color = "black";
        static constexpr std::string_view edge_lifespan_style = "dashed";

        static constexpr std::string_view edge_usage_color = "#6060C0";
        static constexpr std::string_view edge_usage_style = "solid";

        // TODO
    };

    static constexpr char NODE_FMT[] = "node_%u";


    void beginLog();
    void endLog();

    void dumpInfo();
    void dumpLegend();
    void dumpOrdering();
    void dumpPadding();

    void logVarInfo(const TraceEntry::VarInfo &info);
    void logEntry(const Trace &trace, unsigned idx);

    static std::fs::path getTmpFileName(const std::fs::path &outputFile);

    void compileOutput(bool debug = false) const;

    static std::string_view getFuncColor(unsigned level);

    // This feels crotchy...
    auto writeNode(unsigned entryIdx, bool box = false) -> decltype(dot.writeNode(""));

    std::string buildVarInfo(const TraceEntry::VarInfo &info);

};
