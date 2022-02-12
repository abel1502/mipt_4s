#pragma once
#include "../general.h"
#include "../trace.h"
#include "../stdfs.h"
#include <fstream>



class OutputFile {
public:
    OutputFile(const std::fs::path &path_, bool cumulative_ = false);

    inline bool isOpen() const {
        return curFile.is_open();
    }

    void open();
    void forceFlush();
    void close();

    void write  (const std::string_view &data);
    void writeln(const std::string_view &data);
    void writeln();

    void indent(unsigned amount, char chr = ' ');

protected:
    std::fs::path path;
    bool cumulative = false;

    std::ofstream curFile{};

};


class TraceVisualizer {
public:
    virtual void visualize(const Trace &trace) = 0;

    virtual ~TraceVisualizer();

protected:
    OutputFile ofile;


    TraceVisualizer(const std::fs::path &path, bool cumulative = false);
    
};

