#include <ACL/general.h>
#include "array.h"


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
        mylib::CArray<int, 5> arr{1, 2, 3, 4, 5};
        mylib::CArray<int, arr.size()> arr2 = arr;

        DBG("arr2[3] == %d", arr2[3]);
    }

    {
        mylib::Vector<bool> arr(15);

        arr[2] = true;
        arr[3] = true;
        arr[5] = true;
        arr[7] = true;
        arr[11] = true;
        arr[13] = true;

        for (unsigned i = 0; i < arr.size(); ++i) {
            DBG("arr[%u] = %s", i, arr[i] ? "true" : "false");
        }
    }

    {
        math_test::Vector<long, 7>
            a{1, 2, 3, 4, 5, 6, 7},
            b{7, 6, 5, 4, 3, 2, 1};

        auto res = a * b;
        DBG("Dot product is %ld", res);
    }

    return 0;
}
