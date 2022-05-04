#pragma once

#include <ACL/general.h>
#include <ACL/defer.h>
#include <ACL/type_traits.h>
#include <concepts>
#include <memory>
#include <cstring>
#include <string>  // For interoperability
#include <string_view>
#include <initializer_list>
#include <iostream>
#include <algorithm>


#define HEAVY_NULLTERM_CHECK_ 0


namespace mylib {


namespace _impl {

template <typename Allocator>
class SharedLazySourcePtr;

template <typename T, typename CharT>
concept string_view_like =
    std::convertible_to<T, std::basic_string_view<CharT>> &&
    !std::convertible_to<T, const CharT *>;

static_assert(string_view_like<std::string_view, char>);
static_assert(!string_view_like<const char *, char>);

}


template <typename Allocator = std::allocator<char>>
class String {
public:
    #pragma region Typedefs & constants
    using value_type = char;
    using allocator_type = Allocator;
    using reference = value_type &;
    using const_reference = const value_type &;
    using pointer = value_type *;
    using const_pointer = const value_type *;
    using size_type = size_t;
    using difference_type = ptrdiff_t;

    using       iterator = value_type *;
    using const_iterator = const value_type *;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    static constexpr size_type small_string_size = sizeof(void *) / sizeof(value_type);
    static constexpr size_type npos = (size_type)-1;
    static constexpr value_type nullchr = value_type{0};

    #pragma region Protected
protected:
    using sls_ptr_type = _impl::SharedLazySourcePtr<Allocator>;

    enum class LazyState : unsigned {
        small, owned, view, shared,
    };

public:
    #pragma endregion Protected
    #pragma endregion Typedefs & constants

public:
    #pragma region Constructors & such
    String() noexcept(noexcept(allocator_type())) :
        String(allocator_type()) {}

    explicit String(const allocator_type &alloc) noexcept :
        allocator_{alloc},
        small_contents_{""},
        capacity_{small_string_size}, size_{1},  // Including the null char
        flags_{.lazy_state = LazyState::small} {}

    String(size_type count, value_type ch,
           const allocator_type &alloc = allocator_type()) noexcept :
        String(alloc) {

        assign(count, ch);
    }

    String(const String &other) :
        String(other.get_allocator()) {

        assign(other);
    }

    String &operator=(const String &other) {
        return assign(other);
    }

    /// Lazy by default, call ensure_ownership to copy properly
    String(String &other) :
        String(other.get_allocator()) {

        assign(other);
    }

    /// See the non-const copy constructor
    String &operator=(String &other) {
        return assign(other);
    }

    String(String &&other) :
        allocator_{std::move(other.allocator_)},
        ptr_{nullptr},
        capacity_{other.capacity_}, size_{other.size_},
        flags_{std::move(other.flags_)} {

        if (other.flags_.lazy_state == LazyState::shared) {
            new (&sls_ptr_) sls_ptr_type(std::move(other.sls_ptr_));
        } else {
            std::swap(ptr_, other.ptr_);
        }

        other.clear();
    }

    String &operator=(String &&other) {
        if (&other == this) {
            return *this;
        }

        this->~String();
        new (this) String(std::move(other));

        return *this;
    }

    String(const String &other,
           const allocator_type &alloc) :
        String(alloc) {

        assign(other);
    }

    String(const String &other, difference_type raw_pos,
           const allocator_type &alloc = allocator_type()) :
        String(alloc) {

        assign(other, raw_pos);
    }

    String(const String &other, difference_type raw_pos, size_type count,
           const allocator_type &alloc = allocator_type()) :
        String(alloc) {

        assign(other, raw_pos, count);
    }

    implicit String(const value_type *str,
                    const allocator_type &alloc = allocator_type()) :
        String(alloc) {

        assert(str);

        assign(str);
    }

    String(const value_type *str, size_type count,
           const allocator_type &alloc = allocator_type()) :
        String(alloc) {

        assert(str);

        assign(str, count);
    }

    template <std::input_iterator InputIt>
    String(InputIt first, InputIt last,
           const allocator_type &alloc = allocator_type()) :
        String(alloc) {

        assign(std::move(first), std::move(last));
    }

    String(std::initializer_list<value_type> ilist,
           const allocator_type &alloc = allocator_type()) :
        String(alloc) {

        assign(ilist);
    }

    ~String() {
        clear();
    }
    #pragma endregion Constructors & such

    #pragma region View factories
    /**
     * Constructs a non-owning view over a constant string. It is the caller's resposnibility
     * to ensure the source's lifetime. The string will be copied upon modification.
     * The string's size is determined as either count or strlen(s), if count == npos.
     * If count is explicitly specified, yet the string is guaranteed (by the user) to be
     * null-terminated, the last parameter, null_terminated_promise, may be set to true.
     */
    static String view(const value_type *str, size_type count = npos, bool null_terminated_promise = false) {
        assert(str);

        String result{};
        assert(result.flags_.lazy_state == LazyState::small);

        result.flags_.lazy_state = LazyState::view;
        result.flags_.is_null_terminated = false;

        if (count == npos) {
            count = strlen(str);

            result.flags_.is_null_terminated = true;
        } else if (null_terminated_promise) {
            if (str[count] != nullchr) {
                throw std::logic_error("Null terminator was promised, but is not present");
            }

            result.flags_.is_null_terminated = true;
        }

        if (result.flags_.is_null_terminated) {
            // Include the null terminator
            ++count;
        }

        result.view_ptr_ = str;
        result.size_ = count;
        result.capacity_ = count;

        return result;
    }

    static String &view(const value_type *, bool) {
        static_assert(false, "If you don't specify the length manually, "
                             "you don't have to promise null-terminatedness.");
    }

    /**
     * Analogous to the (const value_type *, size_type) overload, but deduces the size at compile time
     */
    template <size_type N>
    inline static String view(const value_type str[N]) {
        return view(str, N);
    }

    /**
     * Analogous to the (const value_type *, size_type) overload, but deduces the size automatically.
     * `null_terminated_promise` is documented for view(value_type *, ...)
     */
    template <_impl::string_view_like<value_type> StringViewLike>
    inline static String view(const StringViewLike &source,
                              bool null_terminated_promise = false) {
        std::string_view sv(source);
        return view(sv.data(), sv.size(), null_terminated_promise);
    }

    /**
     * Shares the other string's contents with this one in dynamic memory, turnig both into
     * copy-on-write views of a common source with shared ownership.
     * If the other string is already copy-on-write, this one simply gets another reference
     * to its source.
     * If the other string is a view, shares its contents directly.
     */
    static String view(String &other) {
        if (other.flags_.lazy_state == LazyState::view) {
            return view(other.as_const());
        }

        // The copy-ctor does that already
        return String(other);
    }

    /**
     * If the other string is a view, shares its contents. Otherwise, fails.
     */
    static String view(const String &other) {
        if (other.flags_.lazy_state != LazyState::view) {
            throw std::logic_error("Requested to perform a view-copy from a non-view");
        }

        String result(other);
        assert(result.flags_.lazy_state == LazyState::view);

        return result;
    }
    #pragma endregion View factories

    #pragma region Assign & clear
    String &assign(size_type count, value_type chr) {
        ensure_total_capacity(count + 1);

        *std::fill(begin(), begin() + count, chr) = nullchr;
        size_ = count + 1;

        return *this;
    }

    inline String &assign(const String &other) {
        if (&other == this) {
            return *this;
        }

        #if 0
        if (other.flags_.lazy_state == LazyState::view) {
            clear();

            view_ptr_ = other.view_ptr_;
            size_ = other.size_;
            capacity_ = other.capacity_;
            flags_ = other.flags_;

            return *this;
        }
        #endif

        return assign(other.data(), other.size());
    }

    inline String &assign(String &other) {
        if (&other == this) {
            return *this;
        }

        #if 0
        if (other.flags_.lazy_state == LazyState::view) {
            return assign(other.as_const());
        }
        #endif

        _share_from(other);
        return *this;
    }

    inline String &assign(const String &other,
                          difference_type raw_pos, size_type count = npos) {
        size_type pos = other.prepare_idx(raw_pos);

        if (pos > other.size()) {
            throw std::out_of_range("Starting pos out of string range");
        }

        return assign(other.data() + pos, std::min(count, other.size() - pos));
    }

    inline String &assign(String &&other) {
        return *this = std::move(other);
    }

    inline String &assign(const value_type *str, size_type count) {
        assert(str);

        return assign(str, str + count);
    }

    inline String &assign(const value_type *str) {
        assert(str);

        return assign(str, strlen(str));
    }

    inline String &assign(std::initializer_list<value_type> ilist) {
        return assign(ilist.begin(), ilist.end());
    }

    template <std::random_access_iterator RandIt>
    String &assign(RandIt first, RandIt last) {
        const size_type count = last - first;
        ensure_total_capacity(count + 1);

        *std::copy(first, last, begin()) = nullchr;
        size_ = count + 1;

        return *this;
    }

    template <std::input_iterator InputIt>
    String &assign(InputIt first, InputIt last) {
        ensure_ownership();

        size_ = 0;

        for (; first != last; ++first) {
            push_back(*first);
        }

        return *this;
    }

    template <_impl::string_view_like<value_type> StringViewLike>
    inline String &assign(const StringViewLike &str) {
        std::string_view sv(str);
        return assign(sv.data(), sv.size());
    }

    template <_impl::string_view_like<value_type> StringViewLike>
    inline String &assign(const StringViewLike &str,
                          size_type pos, size_type count = npos) {
        std::string_view sv(str);
        if (pos > sv.size()) {
            throw std::out_of_range("Starting pos out of string range");
        }
        return assign(sv.data() + pos, std::min(sv.size(), count));
    }

    void clear() {
        switch (flags_.lazy_state) {
        case LazyState::small:
        case LazyState::view:
            break;

        case LazyState::shared:
            sls_ptr_.~sls_ptr_type();
            break;

        case LazyState::owned:
            allocator_.deallocate(ptr_, capacity());
            break;

        NODEFAULT
        }

        // Equivalent to setting small_contents to an empty string
        ptr_ = nullptr;
        size_ = 0;
        capacity_ = small_string_size;
        flags_ = {.lazy_state = LazyState::small};
    }
    #pragma endregion Assign & clear

    #pragma region Size-related info
    constexpr size_type size() const {
        return size_with_null() - (difference_type)flags_.is_null_terminated;
    }

    constexpr size_type length() const {
        return size();
    }

    constexpr size_type size_with_null() const {
        return size_;
    }

    constexpr size_type capacity() const {
        return capacity_;
    }

    constexpr bool empty() const {
        return size() == 0;
    }

    static constexpr size_type max_size() {
        // We reserve npos as a special value just in case
        return npos - 1;
    }
    #pragma endregion Size-related info

    #pragma region Contents Access
    inline const value_type &operator[](difference_type idx) const {
        return at(idx);
    }

    #if 0
    /// This one ensures the string's mutability, so for
    /// explicitness it isn't operator[]
    inline value_type &operator()(difference_type idx) {
        return at(idx);
    }
    #endif

    const value_type &at(difference_type idx) const {
        return get_buf()[prepare_idx(idx)];
    }

    /// This one ensures the string's mutability
    value_type &at(difference_type idx) {
        return ensure_owned_buf()[prepare_idx(idx)];
    }

    const value_type *c_str() const {
        #if HEAVY_NULLTERM_CHECK_
        if (!flags_.is_null_terminated) {
            assert(flags_.lazy_state == LazyState::view);

            throw std::runtime_error("The view is not null-terminated");
        }
        #else
        assert(flags_.is_null_terminated);
        #endif

        return data();
    }

    value_type *mutable_c_str() {
        #if HEAVY_NULLTERM_CHECK_
        if (!flags_.is_null_terminated) {
            assert(flags_.lazy_state == LazyState::view);

            throw std::runtime_error("The view is not null-terminated");
        }
        #else
        assert(flags_.is_null_terminated);
        #endif

        return mutable_data();
    }

    /// May not be null-terminated for a view
    const value_type *data() const {
        return get_buf();
    }

    /// Again, may not be null-terminated for a view
    value_type *mutable_data() {
        return ensure_owned_buf();
    }

    const value_type &front() const {
        return *cbegin();
    }

    const value_type &back() const {
        return *crbegin();
    }

    value_type &mutable_front() {
        return *begin();
    }

    value_type &mutable_back() {
        return *rbegin();
    }
    #pragma endregion Contents Access

    #pragma region Iterators
    const_iterator cbegin() const {
        return get_buf();
    }

    const_iterator cend() const {
        return cbegin() + size();
    }

    const_reverse_iterator crbegin() const {
        return std::make_reverse_iterator(cend());
    }

    const_reverse_iterator crend() const {
        return std::make_reverse_iterator(cbegin());
    }

    const_iterator begin() const {
        return cbegin();
    }

    const_iterator end() const {
        return cend();
    }

    const_reverse_iterator rbegin() const {
        return crbegin();
    }

    const_reverse_iterator rend() const {
        return crend();
    }

    /// This means copying upon iteration
    /// AND SO IT SHOULD BE, though.
    /// To iterate in a guaranteed read-only mode, use either .as_const()
    /// or cbegin(), cend(), etc.
    iterator begin() {
        ensure_owned_buf();

        return const_cast<iterator>(cbegin());
    }

    iterator end() {
        ensure_owned_buf();

        return const_cast<iterator>(cend());
    }

    reverse_iterator rbegin() {
        return std::make_reverse_iterator(end());
    }

    reverse_iterator rend() {
        return std::make_reverse_iterator(begin());
    }
    #pragma endregion Iterators

    #pragma region Allocator stuff
    // For some reason STL wants this to return by value...
    allocator_type get_allocator() const {
        return allocator_;
    }
    #pragma endregion Allocator stuff

    #pragma region STL compat
    // TODO: More implicit?

    explicit String(const std::string &str) :
        String(str.data(), str.size()) {}

    explicit String(const std::string_view &str) :
        String(str.data(), str.size()) {}

    explicit operator std::string() const {
        return std::string(get_buf(), size());
    }

    explicit operator std::string_view() const {
        return std::string_view(get_buf(), size());
    }
    #pragma endregion STL compat

    #pragma region Capacity & size manipulation
    void reserve(size_type new_capacity) {
        ensure_ownership();

        if (new_capacity <= capacity()) {
            return;
        }

        _set_capacity(new_capacity);
    }

    void shrink_to_fit() {
        if (!is_owning()) {
            return;
        }

        _set_capacity(size_with_null());
    }

    void resize(size_type count, value_type chr = nullchr) {
        ensure_total_capacity(count + 1);

        if (count > size()) {
            *std::fill(end(), begin() + count, chr) = nullchr;
        }
        size_ = count + 1;
    }
    #pragma endregion Capacity & size manipulation

    #pragma region View-oriented
    /// A helper to ensure non-copying for views
    constexpr const String &as_const() const {
        return *this;
    }

    inline void ensure_ownership() {
        ensure_owned_buf();
    }

    inline String view_of_this() const {
        return view(*this);
    }

    constexpr bool is_owning() const {
        return flags_.lazy_state <= LazyState::owned;
    }

    constexpr bool is_null_terminated() const {
        return flags_.is_null_terminated;
    }
    #pragma endregion View-oriented

    #pragma region Operations
    #pragma region Back
    void push_back(value_type chr) {
        ensure_extra_capacity(1);

        size_type sz = size();

        ++size_;

        at(sz) = chr;
        at(sz + 1) = nullchr;
    }

    value_type pop_back() {
        if (size() == 0) {
            throw std::out_of_range("Cannot pop from empty container");
        }

        value_type result = std::exchange(at(-1), nullchr);

        --size_;

        return result;
    }
    #pragma endregion Back

    #pragma region Append
    String &append(size_type count, value_type chr) {
        ensure_extra_capacity(count);

        *std::fill(end(), end() + count, chr) = nullchr;
        size_ += count;

        return *this;
    }

    inline String &append(const String &other) {
        return append(other.data(), 0, other.size());
    }

    inline String &append(const String &other, difference_type raw_pos,
                          size_type count = npos) {
        size_type pos = other.prepare_idx(raw_pos);

        if (pos > other.size()) {
            throw std::out_of_range("Starting pos out of string range");
        }

        return append(other.data() + pos, std::min(count, other.size() - pos));
    }

    inline String &append(const value_type *str) {
        return append(str, strlen(str));
    }

    inline String &append(const value_type *str, size_type count) {
        return append(str, str + count);
    }

    inline String &append(std::initializer_list<value_type> ilist) {
        return append(ilist.begin(), ilist.end());
    }

    template <std::random_access_iterator RandIt>
    String &append(RandIt first, RandIt last) {
        const size_type count = last - first;
        ensure_extra_capacity(count);

        *std::copy(first, last, begin() + size()) = nullchr;
        size_ += count;

        return *this;
    }

    template <std::input_iterator InputIt>
    String &append(InputIt first, InputIt last) {
        ensure_ownership();

        for (; first != last; ++first) {
            push_back(*first);
        }

        return *this;
    }

    template <_impl::string_view_like<value_type> StringViewLike>
    inline String &append(const StringViewLike &str) {
        std::string_view sv(str);
        return append(sv.data(), sv.size());
    }

    template <_impl::string_view_like<value_type> StringViewLike>
    inline String &append(const StringViewLike &str,
                          size_type pos, size_type count = npos) {
        std::string_view sv(str);
        if (pos > sv.size()) {
            throw std::out_of_range("Starting pos out of string range");
        }
        return append(sv.data() + pos, std::min(sv.size(), count));
    }
    #pragma endregion Append

    #pragma region Operator +
    template <typename T>
    String &operator+=(T &&arg) {
        return append(std::forward<T>(arg));
    }

    template <typename T>
    String &operator+(T &&arg) & {
        String copy = *this;

        copy.append(std::forward<T>(arg));

        return copy;
    }

    template <typename T>
    String &&operator+(T &&arg) && {
        append(std::forward<T>(arg));

        return std::move(this);
    }
    #pragma endregion Operator +

    // TODO: Provide insert, substr, compare overloads, replace, find?

    #pragma region Erase
    inline String &erase(difference_type raw_pos, size_type count = npos) {
        size_type pos = prepare_idx(raw_pos);

        erase(begin() + pos, begin() + pos + std::min(count, size() - pos));

        return *this;
    }

    iterator erase(const_iterator pos) {
        iterator result = begin() + (pos - cbegin());
        *std::move(std::next(pos), end(), result) = nullchr;
        size_--;
        return result;
    }

    iterator erase(const_iterator first, const_iterator last) {
        iterator result = begin() + (first - cbegin());
        *std::move(first, last, result) = nullchr;
        size_ -= (last - first);
        return result;
    }
    #pragma endregion Erase
    #pragma endregion Operations

    #pragma region Comparison
    bool operator==(const String &other) const {
        if (size() != other.size()) {
            return false;
        }

        return std::equal(cbegin(), cend(), other.cbegin());
    }

    auto operator<=>(const String &other) const {
        return std::lexicographical_compare_three_way(
            cbegin(), cend(), other.cbegin(), other.cend()
        );
    }

    inline friend bool operator==(const String &a, std::string_view &b) {
        return a == String::view(b);
    }

    inline friend auto operator<=>(const String &a, std::string_view &b) {
        return a <=> String::view(b);
    }

    inline friend bool operator==(const std::string_view &a, String &b) {
        return String::view(a) == b;
    }

    inline friend auto operator<=>(const std::string_view &a, String &b) {
        return String::view(a) <=> b;
    }
    #pragma endregion Comparison

protected:
    #pragma region Fields
    allocator_type allocator_{};
    union {  // Null-terminated in all cases
        value_type *ptr_;
        const value_type *view_ptr_;
        sls_ptr_type sls_ptr_;
        static_assert(sizeof(sls_ptr_) <= sizeof(void *));
        value_type small_contents_[small_string_size] = {};
    };
    size_type capacity_ = 0;
    size_type size_ = 0;
    struct {
        // Whether the string is small-string-optimized, owned, or to be copied on write
        LazyState lazy_state: 2 = LazyState::owned;
        // Whether the string is known to be null-terminated. Only makes sense when
        // lazy_state == LazyState::view, otherwise it must be true
        bool is_null_terminated : 1 = true;
    } flags_;
    #pragma endregion Fields


    #pragma region Protected interface
    inline size_type prepare_idx(difference_type idx) const {
        if (idx < 0) {
            idx = idx + size();
        }

        if (idx < 0 || size_with_null() <= (size_type)idx) {
            throw std::out_of_range("Index out of range");
        }

        return (size_type)idx;
    }

    const value_type *get_buf() const {
        switch (flags_.lazy_state) {
        case LazyState::small:
            return small_contents_;

        case LazyState::view:
            return view_ptr_;

        case LazyState::shared:
            return sls_ptr_->get_buf();

        case LazyState::owned:
            return ptr_;

        NODEFAULT
        }
    }

    value_type *get_owned_buf() {
        switch (flags_.lazy_state) {
        case LazyState::small:
            return small_contents_;

        case LazyState::view:
        case LazyState::shared:
            throw std::runtime_error("Buffer is not owned");

        case LazyState::owned:
            return ptr_;

        NODEFAULT
        }
    }

    value_type *ensure_owned_buf() {
        switch (flags_.lazy_state) {
        case LazyState::view:
            _copy_from_view();
            break;

        case LazyState::shared:
            _copy_from_shared();
            break;

        default:
            break;
        }

        return get_owned_buf();
    }

    void ensure_total_capacity(size_type desired_capacity) {
        ensure_ownership();

        size_type proposed_capacity = capacity();
        if (proposed_capacity == 0) {
            proposed_capacity = 8;
        }

        while (desired_capacity > proposed_capacity) {
            proposed_capacity *= 2;
        }

        _set_capacity(proposed_capacity);
    }

    inline void ensure_extra_capacity(size_type extra_capacity) {
        ensure_total_capacity(size_with_null() + extra_capacity);
    }
    #pragma endregion Protected interface

    #pragma region Impl details
    void _copy_from(const value_type *source, size_type amount, size_type new_capacity = 0) {
        if (new_capacity == 0) {
            // Amount does not include the null terminator
            new_capacity = amount + 1;
        }

        assert(new_capacity >= amount + 1);

        value_type *new_buf = nullptr;
        bool going_small = false;

        abel::Defer release_buf_on_error([this, &new_buf, &going_small]() {
            if (!going_small && new_buf) {
                allocator_.deallocate(new_buf, capacity());
                new_buf = nullptr;  // As a sanity check
            }
        });

        if (new_capacity <= small_string_size) {
            assert(amount + 1 <= small_string_size);

            new_buf = small_contents_;
            going_small = true;
        } else {
            new_buf = allocator_.allocate(new_capacity);
            going_small = false;
        }
        std::copy(source, source + amount, new_buf);

        new_buf[amount] = nullchr;

        flags_.is_null_terminated = true;

        size_ = amount + 1;

        if (going_small) {
            flags_.lazy_state = LazyState::small;
            capacity_ = small_string_size;
        } else {
            flags_.lazy_state = LazyState::owned;
            capacity_ = new_capacity;
            ptr_ = new_buf;
        }

        new_buf = nullptr;
    }

    void _copy_from_view() {
        assert(flags_.lazy_state == LazyState::view);

        // Not size_with_null, because _copy_from should add the null byte
        // even if the view was not null-terminated
        _copy_from(view_ptr_, size());
    }

    void _copy_from_shared() {
        assert(flags_.lazy_state == LazyState::shared);

        sls_ptr_type tmp_sls_ptr = std::move(sls_ptr_);
        sls_ptr_.~sls_ptr_type();

        // As an optimization
        if (tmp_sls_ptr.is_last()) {
            (*this) = std::move(*tmp_sls_ptr);
            return;
        }

        // We rely on our size_ being the same as the *tmp_sls_ptr's ones.
        assert(tmp_sls_ptr->size() == size());
        _copy_from(tmp_sls_ptr->data(), size());
    }

    void _set_capacity(size_type new_capacity) {
        if (new_capacity == capacity()) {
            // This might be redundant, but whatever
            ensure_ownership();
            return;
        }

        value_type *old_buf = ensure_owned_buf();
        abel::Defer release_buf_eventually([this, &old_buf,
                                           was_allocated = flags_.lazy_state == LazyState::owned]() {
            if (!was_allocated) {
                return;
            }

            allocator_.deallocate(old_buf, capacity());
            old_buf = nullptr;  // As a sanity check
        });

        _copy_from(old_buf, size(), new_capacity);
    }

    void _pull_shared_info() {
        assert(flags_.lazy_state == LazyState::shared);

        size_ = sls_ptr_->size_;
        capacity_ = sls_ptr_->capacity_;
        // Pointless, but whatever
        flags_.is_null_terminated = sls_ptr_->flags_.is_null_terminated;
    }

    void _share_from(String &other) {
        if (&other == this) {
            return;
        }

        if (other.flags_.lazy_state == LazyState::view) {
            *this = view(other);
            return;
        }

        clear();

        if (other.flags_.lazy_state != LazyState::shared) {
            sls_ptr_type new_sls = sls_ptr_type::source_type::create();
            *new_sls = std::move(other);

            // Just in case
            other.clear();

            new (&other.sls_ptr_) sls_ptr_type(std::move(new_sls));

            other.flags_.lazy_state = LazyState::shared;
            other._pull_shared_info();
        }

        assert(other.flags_.lazy_state == LazyState::shared);
        new (&sls_ptr_) sls_ptr_type(std::move(other.sls_ptr_.copy()));

        flags_.lazy_state = LazyState::shared;
        _pull_shared_info();
    }
    #pragma endregion Impl details

};


#pragma region SharedLazySource
namespace _impl {

#pragma region Source
template <typename Allocator>
class SharedLazySource {
public:
    using string_type = String<Allocator>;

    template <typename Allocator_>
    friend class SharedLazySourcePtr;

    using ptr_type = SharedLazySourcePtr<Allocator>;


    static inline [[nodiscard]] ptr_type create() {
        return ptr_type(new SharedLazySource());
    }

    inline [[nodiscard]] ptr_type get_ptr() {
        return ptr_type(this);
    }

    constexpr bool is_last() const {
        return ref_cnt == 1;
    }

protected:
    string_type source{};
    unsigned ref_cnt{0};


    inline SharedLazySource() {}

    inline void die() {
        delete this;
    }

};
#pragma endregion Source

#pragma region Pointer
template <typename Allocator>
class SharedLazySourcePtr {
public:
    using string_type = String<Allocator>;

    template <typename Allocator_>
    friend class SharedLazySource;

    using source_type = SharedLazySource<Allocator>;


    constexpr SharedLazySourcePtr() : source{nullptr} {}

    SharedLazySourcePtr(const SharedLazySourcePtr &other) = delete;
    SharedLazySourcePtr &operator=(const SharedLazySourcePtr &other) = delete;

    constexpr SharedLazySourcePtr(SharedLazySourcePtr &&other) noexcept : source{other.source} {
        other.source = nullptr;
    }

    constexpr SharedLazySourcePtr &operator=(SharedLazySourcePtr &&other) {
        std::swap(source, other.source);

        return *this;
    };

    inline ~SharedLazySourcePtr() {
        if (!source) {
            return;
        }

        if (--source->ref_cnt == 0) {
            source->die();
        }

        source = nullptr;
    }

    inline string_type &operator* () const {
        assert(source);

        return source->source;
    }

    inline string_type *operator->() const {
        return &**this;
    }

    constexpr bool is_last() const {
        return source->is_last();
    }

    inline SharedLazySourcePtr copy() const {
        return source->get_ptr();
    }

protected:
    source_type *source;


    inline explicit SharedLazySourcePtr(source_type *source) :
        source{source} {

        assert(source);
        ++source->ref_cnt;
    }

};
#pragma endregion Pointer

}
#pragma endregion SharedLazySource


#pragma region Literals
namespace string_literals {

String<> operator ""_s(const char *str, size_t size) {
    return String<>(str, size);
}

String<> operator ""_sv(const char *str, size_t size) {
    return String<>::view(str, size);
}

}
#pragma endregion Literals


}


#pragma region IOStream compat
template <typename Allocator>
std::ostream &operator<<(std::ostream &out, const mylib::String<Allocator> &str) {
    return out << (std::string_view)str;
}

template <typename Allocator>
std::istream &operator<<(std::istream &out, mylib::String<Allocator> &str) {
    std::string tmp{};
    out >> tmp;
    str = std::move(tmp);

    return out;
}
#pragma endregion IOStream compat
