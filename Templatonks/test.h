#pragma once

#include <ACL/general.h>
#include <ACL/type_traits.h>
#include <concepts>
#include <functional>
#include <source_location>
#include <string_view>
#include <format>
#include <vector>  // For tests
#include <limits>
#include <string>  // For std::to_string
#include "array.h"


namespace utest {


#pragma region Logging
void log(const char *fmt, ...);
void indent();
void dedent();


struct LogBlock {
    inline LogBlock() {
        indent();
    }

    inline ~LogBlock() {
        dedent();
    }
};
#pragma endregion Logging


#pragma region Test
class Test {
public:
    struct user_fail_error {};


    inline Test(std::function<bool ()> func,
                std::string_view name,
                std::source_location loc = std::source_location::current()) :
        func{std::move(func)},
        name{std::move(name)},
        loc{std::move(loc)} {}

    Test(const Test &other) = delete;
    Test &operator=(const Test &other) = delete;

    inline Test(Test &&other) noexcept :
        func{std::move(other.func)}, name{std::move(other.name)},
        loc{std::move(other.loc)}, status{std::move(other.status)} {

        other.status = Status::dead;
    }

    inline Test &operator=(Test &&other) noexcept {
        std::swap(func, other.func);
        std::swap(name, other.name);
        std::swap(loc, other.loc);
        status = std::move(other.status);
        other.status = Status::dead;

        return *this;
    }

    bool run();

    static inline Test &current() {
        return *current_ptr;
    }

    void fail() const;
    void fail(const std::string_view &reason) const;
    void fail(const char *exc_name, const std::string_view &exc_what) const;

    template <std::derived_from<std::exception> E>
    inline void fail(const E &exc) const {
        return fail(typeid(E).name(), exc.what());
    }

    void require(bool cond, const char *stmt = nullptr) const;

    ~Test();

    static void sum_up();

    static void reset();

protected:
    std::function<bool ()> func;
    std::string_view name;
    std::source_location loc;

    mutable enum class Status {
        dead,
        not_reported,
        passed,
        failed,
    } status{Status::not_reported};

    static Test *current_ptr;
    static struct {
        unsigned fail_cnt = 0;
    } global_stats;


    void header() const;

    void pass() const;

};

#pragma region TestProxy
namespace _impl {
class TestCallback {
public:
    template <std::invocable<> F>
    TestCallback(F cb, std::source_location loc = std::source_location::current()) :
        loc{loc} {

        if constexpr (std::convertible_to<std::invoke_result_t<F>, bool>) {
            func = std::move(cb);
        } else {
            func = [cb = std::move(cb)]() {
                cb();

                return true;
            };
        }
    }

protected:
    std::function<bool ()> func{};
    std::source_location loc;


    friend struct TestProxy;
};

struct TestProxy {
    std::string_view name;

    Test operator=(TestCallback cb) {
        return Test(std::move(cb.func),
                    std::move(name),
                    std::move(cb.loc));
    }

    bool operator<<(TestCallback cb) {
        return (*this = std::move(cb)).run();
    }
};
}
#pragma endregion TestProxy

#pragma region Literal
namespace literals {
constexpr auto operator ""_test(const char *str, size_t) {
    return _impl::TestProxy(str);
}
}
#pragma endregion Literal

#pragma region Self
namespace self {

inline void fail(auto &&... args) {
    Test::current().fail(FWD(args)...);

    throw Test::user_fail_error{};
}

inline void require(bool cond, const char *stmt = nullptr) {
    Test::current().require(cond, stmt);
}

#define TEST_REQUIRE(STMT) \
    ::utest::self::require(!!(STMT), #STMT)
}
#pragma endregion Self
#pragma endregion Test


#pragma region Suite
template <bool first_fail = false,
          std::common_reference_with<Test> ... As>
void suite(const std::string_view &name, As &&...tests) {
    unsigned cnt_passed = 0;
    bool last_success = true;

    log("Suite \"%.*s\":\n", (unsigned)name.size(), name.data());

    {
        LogBlock block{};

        unsigned cnt_failed = 0;

        if constexpr (first_fail) {
            ((last_success = tests.run(),
              last_success ? ++cnt_passed : ++cnt_failed,
              last_success) && ...);
        } else {
            ((last_success = tests.run(),
              last_success ? ++cnt_passed : ++cnt_failed), ...);
        }

        log("\n"
            "Total: %u tests run,\n"
            "%u passed, %u failed.\n"
            "\n",
            cnt_passed + cnt_failed,
            cnt_passed, cnt_failed);
    }
}
#pragma endregion Suite


}  // namespace utest
