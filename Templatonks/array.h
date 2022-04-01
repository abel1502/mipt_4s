#pragma once

#include <ACL/general.h>
#include <ACL/type_traits.h>
#include <concepts>
#include <memory>
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

public:
    #pragma endregion Protected helpers

    Array() : storage() {}

    Array(size_type size) : storage(size) {
        static_assert(storage.is_dynamic);
    }

    Array(size_type size, const value_type &value) :
        storage(size, value) {
        static_assert(storage.is_dynamic);
    }

    Array(std::initializer_list<value_type> values) :
        storage(std::move(values)) {}

    constexpr reference operator[](difference_type idx) {
        size_type sz = size();

        if (!(-(difference_type)sz <= idx && idx < (difference_type)sz)) {
            throw std::out_of_range("Index out of range");
        }

        return storage.item((idx + sz) % sz);
    }

    constexpr inline const_reference operator[](size_type idx) const {
        return (*const_cast<Array *>(this))[idx];
    }

    void push_back(abel::universal_ref<value_type> auto &&value) {
        static_assert(storage.is_dynamic);

        value_type *cell = storage.expand_one();
        assert(cell);

        std::construct_at(cell, FWD(value));
    }

    template <typename ... As>
    void emplace_back(As &&... args) {
        static_assert(storage.is_dynamic);

        value_type *cell = storage.expand_one();
        assert(cell);

        std::construct_at(cell, std::forward<As>(args)...);
    }

    void pop_back() {
        static_assert(storage.is_dynamic);

        if (size() == 0) {
            throw std::overflow_error("Cannot remove from empty container");
        }

        storage.remove_one();
    }

    void clear() {
        if constexpr (storage.is_dynamic) {
            storage.clear();
            assert(size() == 0);
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

    inline reference front() {
        return (*this)[0];
    }

    inline const_reference front() const {
        return const_cast<Array *>(this)->front();
    }

    inline reference back() {
        return (*this)[-1];
    }

    inline const_reference back() const {
        return const_cast<Array *>(this)->back();
    }

    void swap(Array &other) {
        std::swap(storage, other.storage);
    }

    inline void allocate_now() {
        if constexpr (requires (storage_type storage) { storage.allocate_now(); }) {
            storage.allocate_now();
        }
    }

    #pragma region Arithmetics
    #define ELEMENTWISE_OP_(OP)                                                     \
        template <typename OtherT, template <typename T> typename OtherStorage>     \
        requires (std::convertible_to<OtherT, value_type> &&                        \
                  (storage_type::is_dynamic || OtherStorage<OtherT>::is_dynamic ||  \
                   storage_type::size() == OtherStorage<OtherT>::size()))           \
        Array &operator OP##=(const Array<OtherT, OtherStorage> &other) {           \
            size_type sz = size();                                                  \
                                                                                    \
            if constexpr (storage_type::is_dynamic ||                               \
                          other.storage_type::is_dynamic) {                         \
                if (sz != other.size()) {                                           \
                    throw std::length_error("Argument size mismatch");              \
                }                                                                   \
            }                                                                       \
                                                                                    \
            for (size_type idx = 0; idx < sz; ++idx) {                              \
                (*this)[idx] OP##= other[idx];                                      \
            }                                                                       \
                                                                                    \
            return *this;                                                           \
        }                                                                           \
                                                                                    \
        template <typename OtherT, template <typename T> typename OtherStorage>     \
        Array operator OP(const Array<OtherT, OtherStorage> &other) const & {       \
            Array result = *this;                                                   \
                                                                                    \
            return result OP##= other;                                              \
        }                                                                           \
                                                                                    \
        template <typename OtherT, template <typename T> typename OtherStorage>     \
        Array &&operator OP(const Array<OtherT, OtherStorage> &other) && {          \
            *this OP##= other;                                                      \
                                                                                    \
            return std::move(*this);                                                \
        }

    ELEMENTWISE_OP_(+)
    ELEMENTWISE_OP_(-)
    ELEMENTWISE_OP_(/)
    ELEMENTWISE_OP_(*)
    ELEMENTWISE_OP_(%)

    #undef ELEMENTWISE_OP_

    template <typename OtherT, template <typename T> typename OtherStorage>
    requires (std::convertible_to<OtherT, value_type> &&
                (storage_type::is_dynamic || OtherStorage<OtherT>::is_dynamic ||
                storage_type::size() == OtherStorage<OtherT>::size()))
    value_type dot(const Array<OtherT, OtherStorage> &other) const {
        size_type sz = size();

        if constexpr (storage_type::is_dynamic || other.storage_type::is_dynamic) {
            if (sz != other.size()) {
                throw std::length_error("Argument size mismatch");
            }
        }

        // Workaround for non-default-constructible types
        if (sz == 0) {
            return value_type{};
        }

        value_type result{(*this)[0] * other[0]};

        for (size_type idx = 1; idx < sz; ++idx) {
            result += (*this)[idx] * other[idx];
        }

        return *this;
    }
    #pragma endregion Arithmetics

protected:
    storage_type storage;

};
#pragma endregion Array


#pragma region Array<bool>
template <template <typename T> typename Storage_>
requires (Storage<Storage_<unsigned char>, unsigned char>)
class Array<bool, Storage_> : protected Array<unsigned char, Storage_> {
    #pragma region Protected stuff
protected:
    using Base = Array<unsigned char, Storage_>;
    class ReferenceProxy;

    static constexpr size_t _bytes_ceil (size_t bits) { return (bits + 7) >> 3; }
    static constexpr size_t _bytes_floor(size_t bits) { return (bits    ) >> 3; }

    using Base::storage_type;
    using Base::storage;
    #pragma endregion Private stuff

public:
    using value_type = bool;
    using reference = ReferenceProxy;
    using const_reference = value_type;  // Intentionally
    using size_type = Base::size_type;
    using difference_type = Base::difference_type;

    Array() : Base() {}

    Array(size_type size) : Base(_bytes_floor(size) + 1), bits_last{size % 8} {}

    Array(size_type size, value_type value) :
        Base(_bytes_floor(size) + 1, value ? 0xff : 0x00),
        bits_last{size % 8} {

        if (size % 8 && value) {
            storage.item(_bytes_floor(size)) = (unsigned char)~((-1u) << (size % 8));
        }
    }

    Array(std::initializer_list<value_type> values) :
        Base(_bytes_floor(values.size()) + 1), bits_last{values.size() % 8} {

        uint8_t cur_bit = 0;
        unsigned cur_bit_size = 0;
        for (size_t i = 0; i < values.size(); ++i) {
            if (cur_bit_size == 8) {
                storage.item(_bytes_floor(i)) = cur_bit;
                cur_bit = 0;
                cur_bit_size = 0;
            }

            ++cur_bit_size;
            cur_bit = (cur_bit << 1) | (uint8_t)std::data(values)[i];
        }

        if (cur_bit_size) {
            storage.item(-1) = cur_bit;
        }
    }

    constexpr reference operator[](difference_type idx) {
        size_type sz = size();

        if (!(-(difference_type)sz <= idx && idx < (difference_type)sz)) {
            throw std::out_of_range("Index out of range");
        }

        size_type phys_idx = (idx + sz) % sz;

        uint8_t *byte = &storage.item(_bytes_floor(phys_idx));

        return ReferenceProxy(byte, (uint8_t)(phys_idx % 8));
    }

    constexpr inline const_reference operator[](size_type idx) const {
        return (*const_cast<Array *>(this))[idx];
    }

    void push_back(value_type value) {
        static_assert(storage.is_dynamic);

        ++bits_last;
        if (bits_last == 8) {
            *storage.expand_one() = 0x00;
            bits_last = 0;
        }

        (*this)[-1] = value;
    }

    template <typename ... As>
    void emplace_back(As &&... args) {
        push_back(value_type{std::forward<As>(args)...});
    }

    void pop_back() {
        static_assert(storage.is_dynamic);

        if (bits_last == 0) {
            if (size() == 0) {
                throw std::overflow_error("Cannot remove from empty container");
            }

            storage.remove_one();
            bits_last = 8;
        }
        --bits_last;
    }

    void clear() {
        Base::clear();
        bits_last = 0;
    }

    constexpr size_type size() const {
        size_type bytes = storage.size();

        if (!bytes) {
            // TODO: May be broken, theoretically...
            return bits_last;
        }

        return (bytes - 1) * 8 + bits_last;
    }

    inline bool empty() const {
        static_assert(storage.is_dynamic);

        return size() == 0;
    }

    inline reference front() {
        return (*this)[0];
    }

    inline const_reference front() const {
        return (*this)[0];
    }

    inline reference back() {
        return (*this)[-1];
    }

    inline const_reference back() const {
        return (*this)[-1];
    }

    void swap(Array &other) {
        Base::swap(other);
        std::swap(bits_last, other.bits_last);
    }

    using Base::allocate_now;

protected:
    uint8_t bits_last{0};


    #pragma region ReferenceProxy
    class ReferenceProxy {
    public:
        inline operator bool() const {
            return (*byte >> offset) & 1;
        }

        inline void operator=(bool value) {
            *byte &= ~(1     << offset);
            *byte |=  (value << offset);
        }

    protected:
        uint8_t *byte;
        uint8_t offset;

        friend class Array;

        ReferenceProxy(uint8_t *byte_, uint8_t offset_) :
            byte{byte_}, offset{offset_} {}

    };
    #pragma endregion ReferenceProxy

};
#pragma endregion Array<bool>


#pragma region Aliases
template <typename T>
using Vector = Array<T, DynamicLinearStorage>;

#pragma region CArray impl
namespace _impl {

template <typename T, size_t Size>
struct _helper_CArray {
    using type = Array<T, StaticLinearStorageAdapter<Size>::template type>;
};

template <size_t Size>
struct _helper_CArray<bool, Size> {
    // To account for the optimization on bool arrays
    using type = Array<bool, StaticLinearStorageAdapter<(Size + 7 / 8)>::template type>;
};

}
#pragma endregion CArray impl

template <typename T, size_t Size>
using CArray = typename _impl::_helper_CArray<T, Size>::type;

template <typename T, size_t ChunkSize>
using ChunkedArray = Array<T, DynamicChunkedStorageAdapter<ChunkSize>::template type>;
#pragma endregion Aliases


}
