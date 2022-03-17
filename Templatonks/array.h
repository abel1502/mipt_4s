#pragma once

#include <ACL/general.h>
#include <ACL/type_traits.h>


namespace mylib {


template <typename T_, template <typename T> typename Storage_>
requires (!std::is_reference_v<T_>)
class Array {
public:
    using value_type = T_;
    using reference = value_type &;
    using const_reference = const value_type &;
    using size_type = unsigned;
    using difference_type = int;

    #pragma region Protected helpers
protected:
    using storage_type = Storage_<value_type>;
    
    template <typename T>
    using _if_dynamic = std::enable_if_t<storage_type::is_dynamic, T>;
public:
    #pragma endregion Protected helpers

    Array() : storage{} {}

    Array(size_type size) : storage{size} {}

    Array(size_type size, const value_type &value) : 
        storage{size, value} {}

    reference operator[](size_type idx) {
        return storage.item(idx);
    }

    const_reference operator[](size_type idx) const {
        return (*const_cast<Array *>(this))[idx];
    }

    _if_dynamic<void> push_back(abel::universal_ref<value_type> auto &&val) const {
        storage.data(storage.size(), true) = std::forward<decltype(val)>(val);
    }

protected:
    storage_type storage;

};


}
