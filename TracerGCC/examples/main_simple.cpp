#include "general.h"
#include "tracer.h"
#include "trace.h"
#include "visualizers/html.h"
#include "visualizers/dot.h"


// Uncomment to leave only the simplest testcase
#define TESTCASE_VERY_SIMPLE


using num_t = int;
using wrapper_t = Tracer<num_t>;


static void doMaxsStuff() {
    FUNC_GUARD;

    DECL_TVAR(int, a, 99);
    DECL_TVAR(int, b, 3);
    DECL_TVAR(int, c, 0);

    c = a + b;
}


int main() {
    abel::verbosity = 2;

    Trace::getInstance().reset();

    {
        FUNC_GUARD;

        DECL_TVAR(num_t, a, 123);
        DECL_TVAR(num_t, b, 456);

        printf("a + b = %d\n", (int)(a + b));

        Trace::getInstance().addDbgMsg("Max's stuff");
        doMaxsStuff();
    }

    std::fs::path logPath = "./output/";
    if (std::fs::current_path().filename() == "build") {
        logPath = "../output/";
    }

    HtmlTraceVisualizer{std::fs::path{logPath}.append("log.html")}
        .visualize(Trace::getInstance());

    DotTraceVisualizer{std::fs::path{logPath}.append("log.svg")}
        .visualize(Trace::getInstance());

    return 0;
}