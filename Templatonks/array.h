#pragma once

#include <ACL/general.h>
#include <ACL/type_traits.h>
#include <concepts>
#include <new>
#include "storage.h"


namespace mylib {


#pragma region Array
template <typename T_, template <typename T> typename Storage_>
requires (!std::is_reference_v<T_> &&
          Storage<Storage_<T_>, T_>)
class Array {
public:
    using value_type = T_;
    using reference = value_type &;
    using const_reference = const value_type &;
    using size_type = size_t;
    using difference_type = ptrdiff_t;

    #pragma region Protected helpers
protected:
    using storage_type = Storage_<value_type>;

    // template <typename T>
    // using _if_dynamic = std::enable_if_t<storage_type::is_dynamic, T>;
    //
    // static_assert(!storage_type::is_dynamic || std::is_same_v<_if_dynamic<void>, void>,
    //               "Unexpected _if_dynamic behavior");
public:
    #pragma endregion Protected helpers

    Array() : storage() {}

    Array(size_type size) : storage{size} {
        static_assert(storage.is_dynamic);
    }

    Array(size_type size, const value_type &value) :
        storage(size, value) {
        static_assert(storage.is_dynamic);
    }

    Array(std::initializer_list<value_type> values) :
        storage(std::move(values)) {}

    constexpr reference operator[](difference_type idx) {
        size_type sz = storage.size();

        REQUIRE(-(difference_type)sz <= idx && idx < (difference_type)sz);

        return storage.item((idx + sz) % sz);
    }

    constexpr inline const_reference operator[](size_type idx) const {
        return (*const_cast<Array *>(this))[idx];
    }

    void push_back(abel::universal_ref<value_type> auto &&val) {
        static_assert(storage.is_dynamic);

        value_type *cell = storage.expand_one();
        assert(cell);

        new (cell) value_type(FWD(val));
    }

    template <typename ... As>
    void emplace_back(As &&... args) {
        static_assert(storage.is_dynamic);

        value_type *cell = storage.expand_one();
        assert(cell);

        new (cell) value_type(std::forward<As>(args)...);
    }

    void pop_back() {
        static_assert(storage.is_dynamic);

        storage.remove_one();
    }

    void clear() {
        if constexpr (storage.is_dynamic) {
            storage.clear();
            assert(storage.size() == 0);
        } else {
            static_assert(std::is_default_constructible_v<value_type>);

            for (size_type i = 0; i < storage.size(); ++i) {
                storage.item(i) = value_type{};
            }
        }
    }

    constexpr size_type size() const {
        return storage.size();
    }

    inline bool empty() const {
        static_assert(storage.is_dynamic);

        return size() == 0;
    }

protected:
    storage_type storage;

};
#pragma endregion Array


}
