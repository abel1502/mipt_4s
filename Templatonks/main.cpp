#include <ACL/general.h>
#include <iostream>
#include <iomanip>
#include <string_view>
#include "array.h"
#include "string.h"
#include "dot.h"
#include "test.h"


#pragma region ArrayTester
template <typename T>
class ArrayTester;


template <typename T, template <typename T> typename Storage>
class ArrayTester<mylib::Array<T, Storage>> {
public:
    using array_type = mylib::Array<T, Storage>;
    using value_type = array_type::value_type;
    static constexpr bool is_dynamic = Storage<T>::is_dynamic;


    static constexpr value_type default_val() {
        return value_type{};
    }

    static value_type random_val() {
        if constexpr (std::is_integral_v<value_type>) {
            return (value_type)(abel::randLL() % (((size_t)std::numeric_limits<value_type>::max()) + 1));
        } else if constexpr (std::is_floating_point_v<value_type>) {
            return (value_type)abel::randDouble();
        } else {
            return default_val();
        }
    }


    void test() {
        utest::Test::reset();

        utest::log("Testing %s\n", typeid(array_type).name());

        test_common();

        if constexpr (is_dynamic) {
            test_dynamic();
        } else {
            test_static();
        }

        utest::Test::sum_up();
    }

protected:
    void test_common() {
        using namespace utest::literals;

        // "fail"_test << [=]() {
        //     TEST_REQUIRE(false);
        // };

        //
    }

    void test_dynamic() {
        using namespace utest::literals;

        utest::suite("size_empty",
            "default"_test = [this]() {
                array_type arr;
                TEST_REQUIRE(arr.size() == 0);
            },
            "initlist"_test = [this]() {
                array_type arr{};
                TEST_REQUIRE(arr.size() == 0);
            }
        );

        for (unsigned sz : {0, 1, 2, 7, 8, 9, 15, 16, 17, 10001}) {
            utest::suite(std::format("size_{}", sz),
                "explicit_size"_test = [=]() {
                    array_type arr(sz);
                    TEST_REQUIRE(arr.size() == sz);
                },
                "explicit_size_val"_test = [=]() {
                    array_type arr(sz, random_val());
                    TEST_REQUIRE(arr.size() == sz);
                }
            );
        }

        "access"_test << [=]() {
            const size_t test_size = 17;

            std::vector<value_type> items(test_size);
            array_type arr(test_size);
            for (unsigned i = 0; i < test_size; ++i) {
                arr[i] = items[i] = random_val();
            }

            for (unsigned i = 0; i < test_size; ++i) {
                TEST_REQUIRE(arr[i] == items[i]);
            }

            for (int i = 1; i <= (int)test_size; ++i) {
                TEST_REQUIRE(arr[-i] == items[test_size - i]);
            }
        };

        constexpr auto populate = [](const size_t test_size,
                                     std::vector<value_type> &items,
                                     array_type &arr) {
            items.resize(test_size);
            arr = array_type(test_size);

            for (unsigned i = 0; i < test_size; ++i) {
                arr[i] = items[i] = random_val();
            }
        };

        constexpr auto compare = [](const std::vector<value_type> &items,
                                    const array_type &arr, bool dump = false) {
            TEST_REQUIRE(arr.size() == items.size());

            for (size_t i = 0; i < arr.size(); ++i) {
                if (dump && arr[i] != items[i]) {
                    utest::log("<%s> [%zu/%zu]: %s != %s\n",
                               typeid(value_type).name(),
                               i, arr.size(),
                               std::to_string(arr[i]).c_str(),
                               std::to_string(items[i]).c_str());
                }
                TEST_REQUIRE(arr[i] == items[i]);
            }
        };

        "iteration"_test << [=]() {
            const size_t test_size = 17;
            std::vector<value_type> items;
            array_type arr;
            populate(test_size, items, arr);

            unsigned i = 0;
            for (const auto &elem : arr) {
                TEST_REQUIRE(elem == items[i]);
                ++i;
            }
        };

        "push_pop"_test << [=]() {
            const size_t test_size = 17;

            std::vector<value_type> items{};
            array_type arr{};
            for (unsigned i = 0; i < test_size; ++i) {
                value_type val = random_val();

                items.push_back(val);
                arr.push_back(val);
            }

            compare(items, arr);


            for (unsigned i = 0; i < test_size; ++i) {
                value_type val = items.back();
                value_type my_val = arr.back();

                TEST_REQUIRE(val == my_val);

                items.pop_back();
                arr.pop_back();
            }

            compare(items, arr);
        };

        "swap"_test << [=]() {
            const size_t test_size = 17;
            std::vector<value_type> items;
            array_type arr;
            populate(test_size, items, arr);

            std::swap(arr[0], arr[7]);
            //std::iter_swap(arr.begin(), arr.begin() + 7);
            // std, apparently, has no swap overload for these
            value_type tmp = items[0];
            items[0] = items[7];
            items[7] = tmp;

            compare(items, arr);
        };

        "sort"_test << [=]() {
            const size_t test_size = 17;
            std::vector<value_type> items;
            array_type arr;
            populate(test_size, items, arr);

            #if 1
            std::sort(items.begin(), items.end());
            std::sort(arr.begin(), arr.end());
            #else
            std::copy(arr.begin(), arr.end(), items.begin());
            #endif

            compare(items, arr);
        };

        "copy"_test << [=]() {
            const size_t test_size = 17;
            std::vector<value_type> items;
            array_type arr;
            populate(test_size, items, arr);

            items.clear();

            std::copy(arr.begin(), arr.end(), std::back_inserter(items));

            compare(items, arr);
        };

        "find"_test << [=]() {
            const size_t test_size = 17;
            std::vector<value_type> items;
            array_type arr;
            populate(test_size, items, arr);

            for (unsigned i = 0; i < 500; ++i) {
                value_type item = random_val();
                TEST_REQUIRE(std::find(arr.begin(), arr.end(), item) - arr.begin() ==
                             std::find(items.begin(), items.end(), item) - items.begin());
            }
        };

        // TODO: Also perhaps std::transform?
    }

    void test_static() {
        using namespace utest::literals;

        constexpr size_t static_size = Storage<T>::size();

        //
    }

};
#pragma endregion ArrayTester


#pragma region StringTester
class StringTester {
public:
    using string_type = mylib::String<>;
    using char_type = string_type::value_type;
    static_assert(std::is_same_v<char_type, char>, "Tester not suitable for non-char strings");


    void test() {
        utest::Test::reset();

        utest::log("Testing %s\n", typeid(string_type).name());

        test_all();

        utest::Test::sum_up();
    }

    void test_all() {
        using namespace utest::literals;

        "basic"_test << [=]() {
            mylib::String str{"abc"};
            TEST_REQUIRE(str.size() == 3);
            TEST_REQUIRE(str.size_with_null() == 4);
            TEST_REQUIRE(str.is_owning());
            TEST_REQUIRE(str[0] == 'a');
            TEST_REQUIRE(str[1] == 'b');
            TEST_REQUIRE(str[2] == 'c');
            TEST_REQUIRE(str[3] == 0);
        };

        "access"_test << [=]() {
            constexpr std::string_view data = "Hello, dear world!";

            string_type mystr = data.data();
            TEST_REQUIRE(mystr.is_owning());

            TEST_REQUIRE(data.size() == mystr.size());

            for (size_t i = 0; i < data.size(); ++i) {
                TEST_REQUIRE(mystr[i] == data[i]);
            }

            for (int i = 1; i <= (int)data.size(); ++i) {
                TEST_REQUIRE(mystr[-i] == data[data.size() - i]);
            }

            TEST_REQUIRE(mystr[data.size()] == '\0');
        };

        constexpr auto compare = [](const std::string_view &data,
                                    const string_type &mystr, bool dump = false) {
            TEST_REQUIRE(data.size() == mystr.size());

            for (size_t i = 0; i < data.size(); ++i) {
                if (dump && data[i] != mystr[i]) {
                    utest::log("[%zu/%zu]: %c != %c\n",
                               i, data.size(),
                               data[i], mystr[i]);
                }
                TEST_REQUIRE(data[i] == mystr[i]);
            }
        };

        "iteration"_test << [=]() {
            constexpr std::string_view data = "Hello, dear world!";

            string_type mystr = data.data();
            TEST_REQUIRE(mystr.is_owning());

            TEST_REQUIRE(mystr.size() == (size_t)(mystr.cend() - mystr.cbegin()));

            size_t i = 0;
            for (const auto &elem : mystr) {
                TEST_REQUIRE(elem == data[i]);
                ++i;
            }
        };

        "copy_on_write"_test << [=]() {
            constexpr std::string_view data =
                "I'm soooo long, I sure hope nobody copies me...";
            string_type str1{data};
            TEST_REQUIRE(str1.is_owning());

            string_type str2 = str1;
            TEST_REQUIRE(!str1.is_owning());
            TEST_REQUIRE(!str2.is_owning());

            compare(data, str1);
            compare(data, str2);

            constexpr std::string_view extra =
                ", but most of all, Samy is my hero.";

            str1.append(extra);
            TEST_REQUIRE(str1.is_owning());
            TEST_REQUIRE(!str2.is_owning());
            compare(data, str2);
        };

        "no_copy_on_write"_test << [=]() {
            string_type str1{"I'm soooo long, I sure hope nobody copies me..."};
            TEST_REQUIRE(str1.is_owning());

            string_type str2 = str1.as_const();
            TEST_REQUIRE(str1.is_owning());
            TEST_REQUIRE(str2.is_owning());
        };

        "small_cow"_test << [=]() {
            constexpr std::string_view data =
                "smolcow";
            static_assert((data.size() + 1) * sizeof(char) <= sizeof(void *));

            string_type str1{data};
            TEST_REQUIRE(str1.is_owning());

            string_type str2 = str1;
            TEST_REQUIRE(!str1.is_owning());
            TEST_REQUIRE(!str2.is_owning());

            compare(data, str1);
            compare(data, str2);

            str1.erase(-3);
            TEST_REQUIRE(str1.is_owning());
            TEST_REQUIRE(!str2.is_owning());
            compare(data.substr(0, data.size() - 3), str1);
            compare(data, str2);
        };

        "push_pop"_test << [=]() {
            constexpr std::string_view data = "Abacado or whatever, I'm not into algorithms";

            string_type str{};
            for (size_t i = 0; i < data.size(); ++i) {
                str.push_back(data[i]);
            }

            compare(data, str);


            for (size_t i = 0; i < data.size(); ++i) {
                char_type chr = data[data.size() - 1 - i];
                char_type my_chr = str.back();

                TEST_REQUIRE(chr == my_chr);

                str.pop_back();
            }

            compare("", str);
        };

        test_compare_eq("Hello, dear world");
        test_compare_eq("");
        test_compare_eq(std::string_view("Hi there, pal!"));

        "sort"_test << [=]() {
            constexpr std::string_view data = "Hello, dear world!";

            std::string str = data.data();
            string_type mystr = data.data();
            TEST_REQUIRE(mystr.is_owning());

            std::sort(str.begin(), str.end());
            std::sort(mystr.begin(), mystr.end());

            compare(str, mystr);
        };

        "copy"_test << [=]() {
            constexpr std::string_view data = "Hello, dear world!";

            std::string str{};
            string_type mystr = data.data();
            TEST_REQUIRE(mystr.is_owning());

            std::copy(mystr.begin(), mystr.end(), std::back_inserter(str));

            compare(str, mystr);
        };

        "find"_test << [=]() {
            constexpr std::string_view data = "Hello, dear world!";

            string_type mystr = data.data();
            TEST_REQUIRE(mystr.is_owning());

            using namespace std::string_view_literals;

            for (char_type chr : "ABCabcHh0\0!1,"sv) {
                TEST_REQUIRE(std::find(mystr.begin(), mystr.end(), chr) - mystr.begin() ==
                             std::find(data.begin(), data.end(), chr) - data.begin());
            }
        };

        "literals"_test << [=]() {
            using namespace mylib::string_literals;
            using namespace std::string_view_literals;

            constexpr std::string_view data = "Hell\0o!"sv;

            auto str = "Hell\0o!"_s;
            static_assert(std::is_same_v<decltype(str), string_type>);
            TEST_REQUIRE(str.is_owning());

            compare(data, str);

            auto str_view = "Hell\0o!"_sv;
            static_assert(std::is_same_v<decltype(str_view), string_type>);
            TEST_REQUIRE(!str_view.is_owning());

            compare(data, str_view);
        };

        // TODO: Tests for views, ordered-comparison
    }

    void test_compare_eq(const auto &source) {
        using namespace utest::literals;

        "compare_eq"_test << [=]() {
            string_type str{source};

            TEST_REQUIRE(source == str);
            TEST_REQUIRE(str == source);
        };
    }
};
#pragma endregion StringTester


int main() {
    abel::verbosity = 2;

    DBG("It works!");

    #if 1
    StringTester().test();
    #elif 0
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
    #elif 0
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
