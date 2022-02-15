#pragma once
#include "../general.h"
#include "../trace.h"
#include "../stdfs.h"
#include <fstream>



class OutputFile {
public:
    OutputFile(const std::fs::path &path_, bool cumulative_ = false);

    inline bool isOpen() const {
        return file.is_open();
    }

    void open();
    void forceFlush();
    void close();

    void writeln();
    void writef(const char *fmt, ...);
    void writefv(const char *fmt, va_list args);
    void writefa(unsigned alignment, const char *fmt, ...);
    
    template <typename T>
    void write(const T &obj) {
        file << obj;
    }

    template <typename T>
    void writeln(const T &obj) {
        write(obj);
        writeln();
    }

    void indent(unsigned amount, char chr = ' ');

protected:
    std::fs::path path;
    bool cumulative = false;

    std::ofstream file{};

};


class TraceVisualizer {
public:
    virtual void visualize(const Trace &trace) = 0;

    virtual ~TraceVisualizer() = default;

protected:
    OutputFile ofile;


    inline TraceVisualizer(const std::fs::path &path, bool cumulative = false) :
        ofile{path, cumulative} {}
    
};
