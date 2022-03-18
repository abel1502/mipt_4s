#pragma once

#include <ACL/general.h>
#include <ACL/type_traits.h>
#include <concepts>
#include <stdexcept>
#include <algorithm>
#include <initializer_list>


namespace mylib {


#pragma region Storage concept
template <typename St, typename T>
concept Storage = true || ((requires (std::remove_cvref_t<St> storage,
                            const std::remove_cvref_t<St> const_storage,
                            T item, size_t idx) {
    // General requirements
    St{};
    storage.item(idx) = std::move(item);
    { const_storage.item(idx) } -> std::convertible_to<const T &>;
    { const_storage.size() } -> std::unsigned_integral;
    { St::is_dynamic } -> std::same_as<bool>;
}) && (!St::is_dynamic ||
      requires (std::remove_cvref_t<St> storage,
                const std::remove_cvref_t<St> const_storage,
                T item, size_t idx) {
    // Dynamic requirements
    St{(size_t)1};
    St{(size_t)1, (const T &)item};
    { storage.expand_one() } -> std::same_as<T *>;
    storage.remove_one();
    storage.clear();
}) && (St::is_dynamic ||
      requires (std::remove_cvref_t<St> storage,
                const std::remove_cvref_t<St> const_storage,
                T item, size_t idx) {
    // Static requirements
    // (Mostly just ensuring constexpr-ness)
    std::bool_constant<(std::declval<St>().size(), true)>();
    std::bool_constant<(std::declval<const St>().item(1), true)>();
    std::bool_constant<(std::declval<const St>().item(1) = std::move(std::declval<T>()), true)>();
}));
#pragma endregion Storage concept


#pragma region DynamicLinearStorage
template <typename T>
class DynamicLinearStorage {
protected:
    static constexpr size_t DEFAULT_CAPACITY = 4;
    static_assert(std::is_move_constructible_v<T>);

public:
    static constexpr bool is_dynamic = true;


    DynamicLinearStorage() = default;

    DynamicLinearStorage(size_t new_size) {
        static_assert(std::is_default_constructible_v<T>);

        ensure_capacity(new_size);

        for (size_t i = 0; i < new_size; ++i) {
            new (&data_[i]) T();
            ++size_;
        }
    }

    DynamicLinearStorage(size_t new_size, const T &val) {
        static_assert(std::is_copy_constructible_v<T>);

        ensure_capacity(new_size);

        for (size_t i = 0; i < new_size; ++i) {
            new (&data_[i]) T(val);
            ++size_;
        }
    }

    DynamicLinearStorage(std::initializer_list<T> values) {
        ensure_capacity(values.size());

        for (auto &val : values) {
            new (&data_[size_++]) T(std::move(val));
        }
    }

    ~DynamicLinearStorage() noexcept {
        clear();
        assert(!data_);
    }

    T &item(size_t idx) {
        if (idx >= size_) {
            throw std::out_of_range("Index too large");
        }

        return data_[idx];
    }

    inline const T &item(size_t idx) const {
        return const_cast<DynamicLinearStorage *>(this)->data_(idx);
    }

    T *expand_one() {
        ensure_capacity(size_ + 1);

        return &data_[size_++];
    }

    void remove_one() {
        if (size_ == 0) {
            throw std::overflow_error("Cannot remove from empty storage");
        }

        data_[--size_].~T();
    }

    void clear() noexcept {
        for (unsigned i = 0; i < size_; ++i) {
            data_[i].~T();
        }

        size_ = 0;
        capacity_ = 0;
        free(data_);
        data_ = nullptr;
    }

    inline size_t size() const {
        return size_;
    }

protected:
    T *data_ = nullptr;
    size_t size_ = 0;
    size_t capacity_ = 0;


    void ensure_capacity(size_t desired) {
        if (desired == 0) {
            return;
        }

        if (!data_) {
            assert(size_ == 0);
            assert(capacity_ == 0);

            capacity_ = std::max(desired, DEFAULT_CAPACITY);
            data_ = (T *)calloc(capacity_, sizeof(T));

            if (!data_) {
                capacity_ = 0;
                throw std::overflow_error("Out of memory");
            }

            return;
        }

        if (desired < capacity_) {
            return;
        }

        size_t new_capacity = capacity_ * 2;
        while (new_capacity < desired) {
            new_capacity *= 2;
        }

        T *new_data = (T *)calloc(new_capacity, sizeof(T));
        if (!new_data) {
            throw std::overflow_error("Out of memory");
        }

        assert(new_capacity >= size_);

        for (size_t i = 0; i < size_; ++i) {
            try {
                new (&new_data[i]) T(std::move(data_[i]));
            } catch (...) {
                // Special cleanup in case of an error during
                // broken-invariant state

                for (size_t j = 0; j < i; ++j) {
                    new_data[j].~T();
                }

                for (size_t j = i; j < size_; ++j) {
                    data_[j].~T();
                }

                size_ = 0;

                throw;
            }

            data_[i].~T();
        }

        free(data_);
        data_ = new_data;
        capacity_ = new_capacity;
    }

};
#pragma endregion DynamicLinearStorage


}