#include "visualizer.h"
#include <cstdio>
#include <cstdarg>
#include "../helpers.h"


#pragma region OutputFile
OutputFile::OutputFile(const std::fs::path &path_, bool cumulative_) :
        path{path_}, cumulative{cumulative_} {
    
    std::fs::create_directories(path.parent_path());
}

void OutputFile::open() {
    file.open(path, file.out | (cumulative ? file.app : file.trunc));

    REQUIRE(!file.bad());
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
    write(helpers::vsprintfxx(fmt, args));
}

void OutputFile::writefa(unsigned alignment, const char *fmt, ...) {
    va_list args{};
    va_start(args, fmt);
    std::string result = helpers::vsprintfxx(fmt, args);
    va_end(args);

    write(result);
    for (unsigned writtenSize = result.size(); writtenSize < alignment; ++writtenSize) {
        write(' ');
    }
}

void OutputFile::indent(unsigned amount, char chr) {
    for (unsigned i = 0; i < amount; ++i) {
        file.put(chr);
    }
}


#pragma endregion OutputFile

