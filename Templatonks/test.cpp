#include "test.h"
#include <cstdarg>
#include <ACL/defer.h>
#include <sstream>
#include <exception>
#include <filesystem>


namespace utest {


#pragma region Logging
static unsigned indentation = 0;
static constexpr unsigned INDENTATION_STEP = 2;

void log(const char *fmt, ...) {
    va_list args{};
    va_start(args, fmt);
    std::string result = abel::vsprintfxx(fmt, args);
    va_end(args);

    std::ostringstream indented{};
    static bool should_indent = true;
    for (char c : result) {
        if (should_indent) {
            for (unsigned i = 0; i < indentation; ++i) {
                indented.put(' ');
            }

            should_indent = false;
        }

        indented.put(c);

        if (c == '\n') {
            should_indent = true;
        }
    }

    auto view = indented.view();

    fwrite(view.data(), sizeof(char), view.size(), stdout);
    fflush(stdout);
}

void indent() {
    indentation += INDENTATION_STEP;
}

void dedent() {
    indentation -= INDENTATION_STEP;
}
#pragma endregion Logging


#pragma region Test
Test *Test::current_ptr = nullptr;
decltype(Test::global_stats) Test::global_stats{};


bool Test::run() {
    assert(status != Status::dead);

    bool result = false;

    REQUIRE(!current_ptr);

    current_ptr = this;
    abel::Defer reset_ptr{[this]() {
        current_ptr = nullptr;
    }};

    header();

    try {
        result = func();
    } catch (const user_fail_error &) {
        return false;
    } catch (const std::exception &e) {
        fail(e);
        return false;
    } catch (...) {
        fail("Unknown object was thrown.");
        return false;
    }

    if (status == Status::not_reported) {
        result ? pass() : fail();
    }

    return result;
}

static constexpr const char *COLOR_RED   = "\033[31m";
static constexpr const char *COLOR_GREEN = "\033[32m";
static constexpr const char *COLOR_NONE  = "\033[0m";

void Test::header() const {
    assert(status != Status::dead);

    log("Test [%.*s] at %s():%d ... ",
        (unsigned)name.size(), name.data(),
        loc.function_name(), loc.line());
}

void Test::pass() const {
    assert(status != Status::dead);

    //log("ok\n");
    log("%sok%s\n", COLOR_GREEN, COLOR_NONE);
    status = Status::passed;
}

void Test::fail() const {
    assert(status != Status::dead);

    //log("FAIL!\n");
    log("%sFAIL!%s\n", COLOR_RED, COLOR_NONE);
    status = Status::failed;

    ++global_stats.fail_cnt;
}

void Test::fail(const std::string_view &reason) const {
    fail();
    log("  (%.*s)\n",
        (unsigned)reason.size(), reason.data());
}

void Test::fail(const char *exc_name, const std::string_view &exc_what) const {
    fail();
    log("  (%s(\"%.*s\") was thrown)\n",
        exc_name,
        (unsigned)exc_what.size(), exc_what.data());
}

void Test::require(bool cond, const char *stmt) const {
    assert(status != Status::dead);

    if (cond) {
        return;
    }

    constexpr const char *FMT = "Requirement %snot satisfied";

    std::string msg = abel::sprintfxx("Requirement %snot satisfied",
                                      stmt ? abel::sprintfxx("\"%s\" ", stmt).c_str() : "");

    fail(msg);
    throw user_fail_error{};
}

Test::~Test() {
    if (status == Status::not_reported) {
        log("Warning: test [%.*s] at %s:%d created, but never run\n",
            (unsigned)name.size(), name.data(),
            loc.file_name(), loc.file_name());
    }
}

void Test::sum_up() {
    if (global_stats.fail_cnt == 0) {
        log("\n%sAll tests passed!%s\n\n", COLOR_GREEN, COLOR_NONE);
    } else {
        log("\n%s%u tests failed!%s\n\n", COLOR_RED, global_stats.fail_cnt, COLOR_NONE);

        PAUSE();
    }
}

void Test::reset() {
    global_stats = {};
}
#pragma endregion Test


}
