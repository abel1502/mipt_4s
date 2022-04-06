#include <ACL/general.h>
#include "array.h"
#include "dot.h"
#include "test.h"


int main() {
    abel::verbosity = 2;

    DBG("It works!");

    {
        mylib::Vector<int> arr{};

        arr.push_back(123);
        DBG("arr[-1] == %d", arr[-1]);
        arr.pop_back();
    }

    {
        mylib::CArray<int, 5> arr{1, 5, 4, 7, 2};
        mylib::CArray<int, arr.size()> arr2 = arr;

        for (const auto &val : arr2) {
            DBG("arr2[.] == %d", val);
        }

        std::sort(arr2.begin(), arr2.end());

        for (const auto &val : arr2) {
            DBG("arr2[.] == %d", val);
        }
    }

    {
        mylib::Vector<bool> arr(15);

        DBG("arr.size() = %zu", arr.size());

        arr[2] = true;
        arr[3] = true;
        arr[5] = true;
        arr[7] = true;
        arr[11] = true;
        arr[13] = true;

        arr.push_back(false);
        arr.push_back(true);

        for (unsigned i = 0; i < arr.size(); ++i) {
            DBG("arr[%u] = %s", i, arr[i] ? "true" : "false");
        }
    }

    {
        DBG(" --- ChunkedArray test start");


        DBG("Default {1 2 3}");
        mylib::ChunkedArray<int, 32 / sizeof(int)> arr{1, 2, 3};

        for (unsigned i = 0; i < 7; ++i) {
            int val = i + 4;

            arr.push_back(val);
            DBG("Pushing %d", val);
        }

        for (const auto &val : arr) {
            DBG("arr[.] = %d", val);
        }

        while (!arr.empty()) {
            DBG("Popping %d", arr[-1]);
            arr.pop_back();
        }

        DBG(" --- ChunkedArray test end");
    }

    {
        math_test::Vector<long, 7>
            a{1, 2, 3, 4, 5, 6, 7},
            b{7, 6, 5, 4, 3, 2, 1};

        auto res = a * b;
        DBG("Dot product is %ld", res);
    }

    /* {
        ArrayTester<mylib::Vector<int>> tester{};

        tester.test();
    } */

    return 0;
}
