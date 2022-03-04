#include "general.h"
#include "tracer.h"
#include "trace.h"
#include "visualizers/html.h"
#include "visualizers/dot.h"
#include "rvalues_testbed.h"


// Uncomment to leave only the simplest testcase
// #define TESTCASE_VERY_SIMPLE

// Uncomment to test rvalues stuff instead
// #define TESTCASE_RVALUES


using num_t = int;
using wrapper_t = Tracer<num_t>;


#ifndef TESTCASE_RVALUES
#ifndef TESTCASE_VERY_SIMPLE
static void swap(wrapper_t &a, wrapper_t &b) {
    FUNC_GUARD;

    std::swap(a, b);
}

static void bubbleSort(std::vector<wrapper_t> &buf) {
    FUNC_GUARD;

    bool swapped = false;
    size_t size = buf.size();

    do {
        swapped = false;

        for (size_t i = 1; i < size; ++i) {
            if (buf[i - 1] > buf[i]) {
                swap(buf[i - 1], buf[i]);
                swapped = true;
            }
        }

        --size;
    } while (swapped);
}

static void doMoreStuff(const std::vector<wrapper_t> &nums) {
    FUNC_GUARD;

    for (unsigned i = 1; i < nums.size(); ++i) {
        if (nums[i - 1] < nums[i]) {
            DBG("Array poorly sorted at pos %u!", i);
            break;
        }
    }

    DECL_TVAR(num_t, product, 1);

    for (const auto &num : nums) {
        product *= num;
    }

    printf("The product is: %d\n", (int)product);
}
#endif

static void doMaxsStuff() {
    FUNC_GUARD;

    DECL_TVAR(int, a, 99);
    DECL_TVAR(int, b, 3);
    DECL_TVAR(int, c, 0);

    c = a + b;

    #if 0
    Trace().getInstance().addDbgMsg("Don't even know what else to show");
    swap(a, b);
    swap(b, c);
    #endif
}
#endif


int main() {
    abel::verbosity = 2;


    #ifndef TESTCASE_RVALUES
    #if 0
    Trace::getInstance().reset();

    {
        FUNC_GUARD;

        DECL_TVAR(num_t, a, 123);
        DECL_TVAR(num_t, b, 456);

        printf("a + b = %d\n", (int)(a + b));

        #ifndef TESTCASE_VERY_SIMPLE
        Trace::getInstance().addDbgMsg("Bubble sort preparations");

        std::vector<wrapper_t> nums{};
        for (unsigned i = 0; i < 5; ++i) {
            DECL_TVAR(num_t, cur, (num_t)abel::randLL(100));

            nums.push_back(cur);
        }

        Trace::getInstance().addDbgMsg("Bubble sort");
        bubbleSort(nums);

        Trace::getInstance().addDbgMsg("Other stuff");
        doMoreStuff(nums);
        #endif

        Trace::getInstance().addDbgMsg("Max's stuff");
        doMaxsStuff();
    }

    std::fs::path logPath = "./output/";
    if (std::fs::current_path().filename() == "build") {
        logPath = "../output/";
    }

    HtmlTraceVisualizer{std::fs::path{logPath}.append("log.html")}
        .visualize(Trace::getInstance());

    #if 1
    DotTraceVisualizer{std::fs::path{logPath}.append("log.svg")}
        .visualize(Trace::getInstance());
    #endif
    #endif

    TRACE_CODE("log",
        Trace::getInstance().addDbgMsg("Hi!");

        DECL_TVAR(num_t, a, 123);
        DECL_TVAR(num_t, b, 456);

        printf("a + b = %d\n", (int)(a + b));

        #ifndef TESTCASE_VERY_SIMPLE
        Trace::getInstance().addDbgMsg("Bubble sort preparations");

        std::vector<wrapper_t> nums{};
        for (unsigned i = 0; i < 5; ++i) {
            DECL_TVAR(num_t, cur, (num_t)abel::randLL(100));

            nums.push_back(cur);
        }

        Trace::getInstance().addDbgMsg("Bubble sort");
        bubbleSort(nums);

        Trace::getInstance().addDbgMsg("Other stuff");
        doMoreStuff(nums);
        #endif

        Trace::getInstance().addDbgMsg("Max's stuff");
        doMaxsStuff();
    );
    #else  // defined(TESTCASE_RVALUES)
    testRvalues();
    #endif

    return 0;
}
