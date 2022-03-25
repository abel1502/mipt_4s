#pragma once

#include <ACL/general.h>
#include <ACL/type_traits.h>
#include <concepts>
#include <stdexcept>
#include <algorithm>
#include <initializer_list>
#include <optional>


namespace mylib {


#pragma region Storage concept
template <typename St, typename T>
concept Storage = ((requires (std::remove_cvref_t<St> storage,
                            const std::remove_cvref_t<St> const_storage,
                            T item, size_t idx) {
    // General requirements
    St{};
    storage.item(idx) = std::move(item);
    { const_storage.item(idx) } -> std::convertible_to<const T &>;
    { const_storage.size() } -> std::unsigned_integral;
    { St::is_dynamic } -> std::convertible_to<bool>;
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
    std::bool_constant<(St::size(), true)>();
    // std::bool_constant<(std::declval<const St>().item(1), true)>();
    // std::bool_constant<(std::declval<const St>().item(1) = std::move(std::declval<T>()), true)>();
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

    DynamicLinearStorage(const DynamicLinearStorage &other) {
        static_assert(std::is_copy_constructible_v<T>);

        ensure_capacity(other.capacity_);

        for (; size < other.size; ++size) {
            new (&data_[size]) T(other.data_[size]);
        }
    }

    DynamicLinearStorage &operator=(const DynamicLinearStorage &other) noexcept {
        static_assert(std::is_copy_constructible_v<T>);

        // TODO: Could be optimized, but who cares, really...
        clear();

        ensure_capacity(other.capacity_);

        for (; size < other.size; ++size) {
            new (&data_[size]) T(other.data_[size]);
        }
    }

    DynamicLinearStorage(DynamicLinearStorage &&other) noexcept :
        data_{other.data_}, size_{other.size_}, capacity_{other.capacity_} {

        other.data_ = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;
    }

    DynamicLinearStorage &operator=(DynamicLinearStorage &&other) noexcept {
        std::swap(data_    , other.data_    );
        std::swap(size_    , other.size_    );
        std::swap(capacity_, other.capacity_);
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
                throw std::bad_alloc();
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
            throw std::bad_alloc();
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


#pragma region StaticLinearStorage
template <typename T, size_t Size>
class StaticLinearStorage {
public:
    static constexpr bool is_dynamic = false;


    StaticLinearStorage() :
        data_{} {}

    template <typename ... Ts>
    requires ((std::convertible_to<Ts, T> && ...) &&
              sizeof...(Ts) <= Size)
    StaticLinearStorage(Ts &&... values) :
        data_{std::forward<Ts>(values)...} {}

    StaticLinearStorage(std::initializer_list<T> values) :
        data_{} {

        if (values.size() > size()) {
            throw std::overflow_error("Too many values in initializer list");
        }

        for (unsigned i = 0; i < values.size(); ++i) {
            data_[i] = std::data(values)[i];
        }
    }

    StaticLinearStorage(const StaticLinearStorage &other) = default;
    StaticLinearStorage &operator=(const StaticLinearStorage &other) = default;
    StaticLinearStorage(StaticLinearStorage &&other)
        noexcept(std::is_nothrow_move_constructible_v<T>) = default;

    StaticLinearStorage &operator=(StaticLinearStorage &&other)
        noexcept(std::swappable<T>) {

        [&]<size_t ... Ns>(std::index_sequence<Ns...>) {
            (std::swap(data_[Ns], other.data_[Ns]), ...);
        }(std::make_index_sequence<Size>());

        return *this;
    }

    ~StaticLinearStorage() = default;

    constexpr T &item(size_t idx) {
        if (idx >= size()) {
            throw std::out_of_range("Index too large");
        }

        return data_[idx];
    }

    constexpr const T &item(size_t idx) const {
        if (idx >= size()) {
            throw std::out_of_range("Index too large");
        }

        return data_[idx];
    }

    static constexpr size_t size() noexcept {
        return Size;
    }

protected:
    T data_[Size];

};

template <size_t Size>
struct StaticLinearStorageAdapter {
    template <typename T>
    using type = StaticLinearStorage<T, Size>;
};
#pragma endregion StaticLinearStorage


#pragma region DynamicChunkedStorage
// IMPORTANT: Should always maintain at least one chunk, okay if empty
template <typename T, size_t ChunkSize>
class DynamicChunkedStorage {
    #pragma region Protected stuff
    static_assert(std::is_copy_constructible_v<T>);

    static constexpr size_t DEFAULT_CHUNKS = 4;

    static constexpr size_t chunk_size = ChunkSize;
    static_assert(chunk_size > 0);

    static constexpr size_t _chunk_idx   (size_t pos) { return pos / chunk_size; }
    static constexpr size_t _chunk_offset(size_t pos) { return pos % chunk_size; }
    #pragma endregion Protected stuff

public:
    static constexpr bool is_dynamic = true;


    DynamicChunkedStorage() :
        chunks_(1), last_size_{0}, default_item_{} {}

    DynamicChunkedStorage(size_t new_size) :
        chunks_(new_size / chunk_size + 1),
        last_size_{new_size % chunk_size},
        default_item_{} {

        static_assert(std::is_default_constructible_v<T>);
    }

    DynamicChunkedStorage(size_t new_size, const T &val) :
        DynamicChunkedStorage(new_size) {

        static_assert(std::is_copy_constructible_v<T>);

        default_item_ = val;
    }

    DynamicChunkedStorage(std::initializer_list<T> values) :
        DynamicChunkedStorage() {

        for (auto &val : values) {
            new (expand_one()) T(std::move(val));
        }
    }

    DynamicChunkedStorage(const DynamicChunkedStorage &other) :
        DynamicChunkedStorage() {

        static_assert(std::is_copy_constructible_v<T>);

        while (size < other.size) {
            new (expand_one()) T(other.item(size));
        }
    }

    DynamicChunkedStorage &operator=(const DynamicChunkedStorage &other) noexcept {
        static_assert(std::is_copy_constructible_v<T>);

        // TODO: Could be optimized, but who cares, really...
        clear();

        while (size < other.size) {
            new (expand_one()) T(other.item(size));
        }
    }

    DynamicChunkedStorage(DynamicChunkedStorage &&other) noexcept :
        chunks_{std::move(other.chunks_)}, last_size_{other.last_size_},
        default_item_{std::move(other.default_item_)} {

        other.last_size_ = 0;
    }

    DynamicChunkedStorage &operator=(DynamicChunkedStorage &&other) noexcept {
        std::swap(chunks_      , other.chunks_      );
        std::swap(last_size_   , other.last_size_   );
        std::swap(default_item_, other.default_item_);
    }

    ~DynamicChunkedStorage() noexcept {
        clear();
    }

    T &item(size_t idx) {
        if (idx >= size()) {
            throw std::out_of_range("Index too large");
        }

        ensure_chunk_for(idx);

        return chunks_.item(_chunk_idx(idx))[_chunk_offset(idx)];
    }

    inline const T &item(size_t idx) const {
        return const_cast<DynamicLinearStorage *>(this)->data_(idx);
    }

    T *expand_one() {
        ++last_size_;

        if (last_size_ == chunk_size) {
            last_size_ = 0;
            *chunks_.expand_one() = nullptr;
        }

        ensure_chunk_for(size() - 1, false);

        return &item(size() - 1);
    }

    void remove_one() {
        if (size() == 0) {
            throw std::overflow_error("Cannot remove from empty storage");
        }

        if (last_size_ == 0) {
            clear_chunk(chunks_.size() - 1);

            last_size_ = chunk_size;
            chunks_.remove_one();
        }
        assert(chunks_.size() > 0);

        last_size_--;

        T *cur_chunk = chunks_.item(chunks_.size() - 1);

        if (cur_chunk) {
            cur_chunk[last_size_].~T();
        }
    }

    void clear() noexcept {
        for (unsigned i = 0; i < chunks_.size() - 1; ++i) {
            clear_chunk(i);
        }

        chunks_.clear();
        *chunks_.expand_one() = nullptr;
        last_size_ = 0;
        default_item_.reset();
    }

    inline size_t size() const {
        assert(chunks_.size() > 0);

        return (chunks_.size() - 1) * chunk_size + last_size_;
    }

    void allocate_now() {
        size_t sz = size();
        for (size_t i = 0; i < size(); ++i) {
            ensure_chunk_for(i);
        }
    }

protected:
    DynamicLinearStorage<T *> chunks_;
    size_t last_size_;
    std::optional<T> default_item_{};


    size_t get_chunk_size(size_t idx) {
        assert(idx < chunks_.size());

        size_t result = chunk_size;
        if (idx + 1 == chunks_.size()) {
            result = last_size_;
        }

        assert(result <= chunk_size);

        return result;
    }

    void clear_chunk(size_t idx) {
        T *cur_chunk = chunks_.item(idx);

        if (!cur_chunk) {
            return;
        }

        size_t cur_chunk_size = get_chunk_size(idx);

        for (size_t i = 0; i < cur_chunk_size; ++i) {
            cur_chunk[i].~T();
        }

        free(cur_chunk);

        chunks_.item(idx) = nullptr;
    }

    void ensure_chunk_for(size_t idx, bool with_last = true) {
        assert(idx < size());

        size_t cur_chunk_idx = _chunk_idx(idx);
        T *cur_chunk = chunks_.item(cur_chunk_idx);

        if (cur_chunk) {
            return;
        }

        bool haveTemplate = default_item_.has_value();

        assert(( haveTemplate && std::is_copy_constructible_v   <T>) ||
               (!haveTemplate && std::is_default_constructible_v<T>));

        cur_chunk = (T *)calloc(chunk_size, sizeof(T));
        if (!cur_chunk) {
            throw std::bad_alloc();
        }
        chunks_.item(cur_chunk_idx) = cur_chunk;

        size_t cur_chunk_size = get_chunk_size(cur_chunk_idx);
        assert(cur_chunk_size > 0);

        if (!with_last) {
            --cur_chunk_size;
        }

        for (size_t i = 0; i < cur_chunk_size; ++i) {
            // Okay, on second thought, screw exception handling
            // for weird edge cases
            if (haveTemplate) {
                new (&cur_chunk[i]) T(default_item_.value());
            } else {
                new (&cur_chunk[i]) T();
            }
        }
    }

};

template <size_t ChunkSize>
struct DynamicChunkedStorageAdapter {
    template <typename T>
    using type = DynamicChunkedStorage<T, ChunkSize>;
};
#pragma endregion DynamicChunkedStorage


}
