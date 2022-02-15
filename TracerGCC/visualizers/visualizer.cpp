#include "visualizer.h"
#include <cstdio>
#include <cstdarg>


#pragma region OutputFile
OutputFile::OutputFile(const std::fs::path &path_, bool cumulative_) :
        path{path_}, cumulative{cumulative_} {
    
    std::fs::create_directories(path.parent_path());
}

void OutputFile::open() {
    file.open(path, file.out | (cumulative ? file.app : file.trunc));
}

void OutputFile::close() {
    file.close();
}

void OutputFile::forceFlush() {
    file.flush();
}

void OutputFile::writeln() {
    file.put('\n');
}

void OutputFile::writef(const char *fmt, ...) {
    va_list args{};

    va_start(args, fmt);
    writefv(fmt, args);
    va_end(args);
}

void OutputFile::writefv(const char *fmt, va_list args) {
    va_list args2{};

    va_copy(args2, args);
    int reqSize = vsnprintf(nullptr, 0, fmt, args2);
    va_end(args2);

    assert(reqSize >= 0);
    
    std::string result{};
    result.resize(reqSize + 1);

    int writtenSize = vsnprintf(result.data(), reqSize + 1, fmt, args);

    assert(writtenSize > 0 && writtenSize <= reqSize);

    result.resize(writtenSize);

    write(result);
}

void OutputFile::writefa(unsigned alignment, const char *fmt, ...) {
    va_list args{};

    va_start(args, fmt);
    int reqSize = vsnprintf(nullptr, 0, fmt, args);
    va_end(args);

    assert(reqSize >= 0);
    
    std::string result{};
    result.resize(reqSize + 1);

    va_start(args, fmt);
    int writtenSize = vsnprintf(result.data(), reqSize + 1, fmt, args);
    va_end(args);

    assert(writtenSize > 0 && writtenSize <= reqSize);

    result.resize(writtenSize);

    write(result);
    for (; (unsigned)writtenSize < alignment; ++writtenSize) {
        write(' ');
    }
}

void OutputFile::indent(unsigned amount, char chr) {
    for (unsigned i = 0; i < amount; ++i) {
        file.put(chr);
    }
}


#pragma endregion OutputFile

