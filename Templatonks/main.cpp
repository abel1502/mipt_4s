#include <ACL/general.h>
#include <iostream>
#include <string_view>
#include "array.h"
#include "string.h"
#include "dot.h"
#include "test.h"


int main() {
    abel::verbosity = 2;

    DBG("It works!");

    #if 1
    mylib::String str{"abc"};
    assert(str[0] == 'a');
    assert(str[1] == 'b');
    assert(str[2] == 'c');
    assert(str[3] == 0);
    mylib::String str2{"Hello there, dear citizen!!"};
    str = str2;
    std::cout << str << "\n" << str2 << "\n" << "\n";
    str.at(0) = 'B';
    str.at(15) = 'e';
    std::cout << str << "\n" << str2 << "\n" << "\n";
    std::cout << "str " << (str < str2 ? "<" : ">") << " str2\n";
    std::cout << "str " << (str < "Bobr" ? "<" : ">") << " \"Bobr\"\n";
    std::cout << "str " << (str < "Babr" ? "<" : ">") << " \"Babr\"\n";

    {
        std::string_view stl_str_view = "Hello!";
     
        str = str.view(stl_str_view, true);
        std::cout << str << "\n";
        stl_str_view = "Bye!";
        std::cout << str << "\n";
        str = str.view("Or not");
        std::cout << str << "\n";
    }

    {
        std::string stl_str = "Hello!";
     
        str = str.view(stl_str, true);
        std::cout << str << "\n";
        stl_str[0] = 'B';
        std::cout << str << "\n";
        str = str.view("Or not");
        std::cout << str << "\n";
    }

    #else
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
    #endif

    return 0;
}
