#include <ACL/general.h>
#include "array.h"
#include "storage.h"


int main() {
    abel::verbosity = 2;

    DBG("It works!");

    mylib::Array<int, mylib::DynamicLinearStorage> arr{};

    arr.push_back(123);

    DBG("arr[-1] == %d", arr[-1]);

    arr.pop_back();

    return 0;
}