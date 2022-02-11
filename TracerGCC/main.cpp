#include "general.h"
#include "tracer.h"
#include "trace.h"


using num_t = int;
using wrapper_t = Tracer<num_t>;


int main() {
    abel::verbosity = 2;
    DBG("!!!!");

    FUNC_GUARD;

    DECL_TVAR(num_t, a, 123);
    DECL_TVAR(num_t, b, 456);

    printf("%d\n", (int)(a + b));

    return 0;
}
