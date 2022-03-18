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

    return 0;
}
