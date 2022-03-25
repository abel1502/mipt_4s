#pragma once

#include <ACL/general.h>
#include <ACL/type_traits.h>
#include <concepts>
#include "array.h"


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
            return value_type{abel::randLL()};
        } else if constexpr (std::is_floating_point_v<value_type>) {
            return value_type{abel::randDouble()};
        } else {
            return default_val();
        }
    }


    void test() {
        if constexpr (is_dynamic) {
            test_dynamic();
        } else {
            test_static();
        }

        // TODO: Common tests
    }

protected:
    void test_dynamic() {
        test_dynamic_size();
    }

    void test_static() {
        //
    }

    void test_dynamic_size() {
        //
    }

};
