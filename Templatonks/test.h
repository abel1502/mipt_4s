#pragma once

#include <ACL/general.h>
#include <ACL/type_traits.h>
#include <concepts>
#include <functional>
#include <source_location>
#include <string_view>
#include <format>
#include <vector>  // For tests
#include <limits>
#include <string>  // For std::to_string
#include "array.h"


namespace utest {


#pragma region Logging
void log(const char *fmt, ...);
void indent();
void dedent();


struct LogBlock {
    inline LogBlock() {
        indent();
    }

    inline ~LogBlock() {
        dedent();
    }
};
#pragma endregion Logging


#pragma region Test
class Test {
public:
    struct user_fail_error {};


    inline Test(std::function<bool ()> func,
                std::string_view name,
                std::source_location loc = std::source_location::current()) :
        func{std::move(func)},
        name{std::move(name)},
        loc{std::move(loc)} {}

    Test(const Test &other) = delete;
    Test &operator=(const Test &other) = delete;

    inline Test(Test &&other) noexcept :
        func{std::move(other.func)}, name{std::move(other.name)},
        loc{std::move(other.loc)}, status{std::move(other.status)} {

        other.status = Status::dead;
    }

    inline Test &operator=(Test &&other) noexcept {
        std::swap(func, other.func);
        std::swap(name, other.name);
        std::swap(loc, other.loc);
        status = std::move(other.status);
        other.status = Status::dead;

        return *this;
    }

    bool run();

    static inline Test &current() {
        return *current_ptr;
    }

    void fail() const;
    void fail(const std::string_view &reason) const;
    void fail(const char *exc_name, const std::string_view &exc_what) const;

    template <std::derived_from<std::exception> E>
    inline void fail(const E &exc) const {
        return fail(typeid(E).name(), exc.what());
    }

    void require(bool cond, const char *stmt = nullptr) const;

    ~Test();

    static void sum_up();

    static void reset();

protected:
    std::function<bool ()> func;
    std::string_view name;
    std::source_location loc;

    mutable enum class Status {
        dead,
        not_reported,
        passed,
        failed,
    } status{Status::not_reported};

    static Test *current_ptr;
    static struct {
        unsigned fail_cnt = 0;
    } global_stats;


    void header() const;

    void pass() const;

};

#pragma region TestProxy
namespace _impl {
class TestCallback {
public:
    template <std::invocable<> F>
    TestCallback(F cb, std::source_location loc = std::source_location::current()) :
        loc{loc} {

        if constexpr (std::convertible_to<std::invoke_result_t<F>, bool>) {
            func = std::move(cb);
        } else {
            func = [cb = std::move(cb)]() {
                cb();

                return true;
            };
        }
    }

protected:
    std::function<bool ()> func{};
    std::source_location loc;


    friend struct TestProxy;
};

struct TestProxy {
    std::string_view name;

    Test operator=(TestCallback cb) {
        return Test(std::move(cb.func),
                    std::move(name),
                    std::move(cb.loc));
    }

    bool operator<<(TestCallback cb) {
        return (*this = std::move(cb)).run();
    }
};
}
#pragma endregion TestProxy

#pragma region Literal
namespace literals {
constexpr auto operator ""_test(const char *str, size_t) {
    return _impl::TestProxy(str);
}
}
#pragma endregion Literal

#pragma region Self
namespace self {

inline void fail(auto &&... args) {
    Test::current().fail(FWD(args)...);

    throw Test::user_fail_error{};
}

inline void require(bool cond, const char *stmt = nullptr) {
    Test::current().require(cond, stmt);
}

#define TEST_REQUIRE(STMT) \
    ::utest::self::require(!!(STMT), #STMT)
}
#pragma endregion Self
#pragma endregion Test


#pragma region Suite
template <bool first_fail = false,
          std::common_reference_with<Test> ... As>
void suite(const std::string_view &name, As &&...tests) {
    unsigned cnt_passed = 0;
    bool last_success = true;

    log("Suite \"%.*s\":\n", (unsigned)name.size(), name.data());

    {
        LogBlock block{};

        unsigned cnt_failed = 0;

        if constexpr (first_fail) {
            ((last_success = tests.run(),
              last_success ? ++cnt_passed : ++cnt_failed,
              last_success) && ...);
        } else {
            ((last_success = tests.run(),
              last_success ? ++cnt_passed : ++cnt_failed), ...);
        }

        log("\n"
            "Total: %u tests run,\n"
            "%u passed, %u failed.\n"
            "\n",
            cnt_passed + cnt_failed,
            cnt_passed, cnt_failed);
    }
}
#pragma endregion Suite


}  // namespace utest


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
