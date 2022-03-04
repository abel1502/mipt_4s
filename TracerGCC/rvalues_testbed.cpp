#include "rvalues_testbed.h"
#include "general.h"
#include "tracer.h"
#include "trace.h"
#include "visualizers/html.h"
#include "visualizers/dot.h"
#include <type_traits>


// Uncomment to replace custom move and forward with std ones
// #define TESTCASE_USE_STD_UTILS


namespace {

#pragma region TestValue
struct TestValue {
public:
    using num_t = int;
    static_assert(std::is_arithmetic_v<num_t>);

    constexpr TestValue(num_t val_ = 0) :
        val{val_} {};
    
    constexpr TestValue(const TestValue &other) noexcept :
        val{other.val} {}
    
    constexpr TestValue &operator=(const TestValue &other) noexcept {
        val = other.val;

        return *this;
    }

    constexpr TestValue(TestValue &&other) noexcept :
        val{other.val} {
        
        other.val = 0;
    }
    
    constexpr TestValue &operator=(TestValue &&other) noexcept {
        val = other.val;
        other.val = 0;

        return *this;
    }

    // Implicit on purpose
    constexpr operator num_t() const {
        return val;
    }

protected:
    num_t val;
};
#pragma endregion TestValue

using val_t = TestValue;
using wrapper_t = Tracer<val_t>;

#pragma region util
namespace util {

#ifdef TESTCASE_USE_STD_UTILS
using std::remove_reference;
using std::remove_reference_t;
using std::move;
using std::forward;
#else // !defined(TESTCASE_USE_STD_UTILS)
template <typename T>
struct remove_reference {
    using type = T;
};

template <typename T>
struct remove_reference<T &> {
    using type = T;
};

template <typename T>
struct remove_reference<T &&> {
    using type = T;
};

template <typename T>
using remove_reference_t = typename remove_reference<T>::type;

template <typename T>
[[nodiscard]] constexpr
decltype(auto) move(T &&val) {
    return static_cast<remove_reference_t<T> &&>(val);
}

template <typename T>
[[nodiscard]] constexpr
T &&forward(remove_reference_t<T> &val) {
    return static_cast<T &&>(val);
}

template <typename T>
[[nodiscard]] constexpr
T &&forward(remove_reference_t<T> &&val) {
    static_assert(!std::is_lvalue_reference_v<T>);

    return static_cast<T &&>(val);
}
#endif

}
#pragma endregion util

#pragma region TestGuard
class TestGuard {
public:
    inline TestGuard(const char *name, const std::source_location &place =
                     std::source_location::current()) :
        traceGuard{place}, name{name} {}

protected:
    #pragma region Resetter
    struct Resetter {
        TestGuard &master;

        inline Resetter(TestGuard &master_) :
            master{master_} {

            Trace::getInstance().reset();
        }

        inline ~Resetter() {
            master.dump();
        }
    } _resetter [[no_unique_address]] {*this};

    friend Resetter;
    #pragma endregion Resetter
    TraceFuncGuard traceGuard;
    const char *name;


    void dump() const {
        std::fs::path logPath = "./output/";
        if (std::fs::current_path().filename() == "build") {
            logPath = "../output/";
        }

        logPath.append("rvalues");
        logPath.append(name);

        HtmlTraceVisualizer{std::fs::path{logPath}.append("log.html")}
            .visualize(Trace::getInstance());

        DotTraceVisualizer{std::fs::path{logPath}.append("log.svg")}
            .visualize(Trace::getInstance());
    }

};
#pragma endregion TestGuard

#pragma region consume
template <typename T>
void consume_plain(T &&obj) {
    volatile auto result = obj;
}

template <typename T>
void consume_move(T &&obj) {
    volatile auto result = util::move(obj);
}

template <typename T>
void consume_fwd(T &&obj) {
    volatile auto result = util::forward<T>(obj);
}
#pragma endregion consume

#pragma region tests
constexpr val_t TEST_VALUE = 123;


#define TEST_FUNC_BODY_(TYPE)                               \
    void test_##TYPE() {                                    \
        TestGuard guard{#TYPE};                             \
                                                            \
        if constexpr (1) {                                  \
            Trace::getInstance()                            \
                .addDbgMsg("Intended copy");                \
                                                            \
            DECL_TVAR(val_t, obj, TEST_VALUE);              \
                                                            \
            const val_t was = (val_t)obj;                   \
                                                            \
            consume_##TYPE(obj);                            \
                                                            \
            const val_t is = (val_t)obj;                    \
                                                            \
            if (is != was) {                                \
                Trace::getInstance()                        \
                    .addDbgMsg("Obj is broken!");           \
            }                                               \
        }                                                   \
                                                            \
        if constexpr (1) {                                  \
            Trace::getInstance()                            \
                .addDbgMsg("Implicitly intended move");     \
                                                            \
            consume_##TYPE(wrapper_t{TEST_VALUE, "obj"});   \
        }                                                   \
                                                            \
        if constexpr (1) {                                  \
            Trace::getInstance()                            \
                .addDbgMsg("Explicitly intended move");     \
                                                            \
            DECL_TVAR(val_t, obj, TEST_VALUE);              \
                                                            \
            consume_##TYPE(util::move(obj));                \
        }                                                   \
    }

TEST_FUNC_BODY_(plain)
TEST_FUNC_BODY_(move)
TEST_FUNC_BODY_(fwd)

#undef TEST_FUNC_BODY_
#pragma endregion tests

}


void testRvalues() {
    test_plain();
    test_move();
    test_fwd();
}
