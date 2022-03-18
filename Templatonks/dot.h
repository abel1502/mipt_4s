#pragma once

#include <ACL/general.h>
#include <ACL/type_traits.h>
#include <concepts>


// Uncomment to employ a cooler trick
#define MATH_TEST_COOLER_DOT_PRODUCT


namespace math_test {


#ifndef MATH_TEST_COOLER_DOT_PRODUCT
template <typename T, size_t N>
requires (std::signed_integral<T> || std::floating_point<T>)
T _dot_product(T *data_a, T *data_b) {
    if constexpr (N == 0) {
        return T{};
    } else {
        return data_a * data_b + _dot_product<T, N - 1>(data_a + 1, data_b + 1);
    }
}
#endif


template <typename T, size_t N>
requires (std::signed_integral<T> || std::floating_point<T>)
struct Vector {
    static constexpr size_t size = N;

    T data[N];


    T operator *(const Vector &other) {
        static_assert(size == other.size,
                      "I must have misunderstood how implicit template args work");

        #ifndef MATH_TEST_COOLER_DOT_PRODUCT
        return _dot_product<T, N>(data, other.data);
        #else
        return [&]<size_t ... Ns>(std::index_sequence<Ns...>) {
            return ((data[Ns] * other.data[Ns]) + ...);
        }(std::make_index_sequence<N>());
        #endif
    }

};


}
