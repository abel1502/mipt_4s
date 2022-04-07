#include <ACL/general.h>
#include "array.h"
#include "dot.h"
#include "test.h"


int main() {
    abel::verbosity = 2;

    DBG("It works!");

    ArrayTester<mylib::Vector<int>>().test();
    ArrayTester<mylib::CArray<int, 3>>().test();
    ArrayTester<mylib::CArray<int, 7>>().test();
    ArrayTester<mylib::Vector<bool>>().test();
    ArrayTester<mylib::ChunkedArray<int, 32 / sizeof(int)>>().test();

    {
        math_test::Vector<long, 7>
            a{1, 2, 3, 4, 5, 6, 7},
            b{7, 6, 5, 4, 3, 2, 1};

        auto res = a * b;
        DBG("Dot product is %ld", res);
    }

    return 0;
}
