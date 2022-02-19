/**
 * This header aims to provide a basic (inefficient) alternative to std::format,
 * which is, sadly, not available in my current environment at the time of writing
 * this. Hopefully, this will be adapted to ACL (my favorite library. If you haven't
 * heared of it, you should definitely check it out :) ).
 * 
 * Somewhat inspired by MSVC STL's implementation.
 * 
 * TODO: Lots of finishing
 */

#pragma once
#include "general.h"
#include <tuple>
#include <memory>
#include <variant>
#include <cstdint>
#include <string_view>
#include <string>
#include <type_traits>
#include <cstring>
#include <iterator>
#include <concepts>
#include "named_arg.h"


namespace abel::fmt {


DECLARE_ERROR(format_error, error);


enum class _format_arg_type : size_t {
    #define FMT_TYPE_(NAME, TYPE) \
        NAME,
    
    #include "format.dsl.h"
};


class format_parse_context {
public:
    //

protected:
    //

};


template <typename T>
class formatter {
public:
    //

protected:
    //

};


#pragma region concepts
template <typename T, typename Context>
concept has_formatter = requires(T &val, Context &ctx) {
    std::declval<typename Context::template formatter_type<std::remove_cvref_t<T>>>()
        .format(val, ctx);
};

template <typename T, typename Context>
concept has_const_formatter = has_formatter<const std::remove_reference_t<T>, Context>;
#pragma endregion concepts


template <typename Context>
class format_arg {
public:
    class handle {
    public:
        inline void format(format_parse_context &parse_ctx,
                           Context& format_ctx) const;

        template <typename T>
        explicit inline handle(T &&val) :
            obj{&val},
            format_func{[](format_parse_context &parse_ctx,
                           Context &format_ctx, const void *obj) {
                using value_t = std::remove_cvref_t<T>;
                typename Context::template formatter_type<value_t> formatter{};
                using qual_value_t = std::conditional_t<has_const_formatter<value_t, Context>,
                                                        const value_t, value_t>;

                parce_ctx.advance_to(formatter.parse(parse_ctx));
                format_ctx.advance_to(formatter.format(
                    *const_cast<qual_value_t *>((value_t *)obj), format_ctx
                ));

                return;
            }} {
            
            assert(obj);
            static_assert(has_const_formatter<T, Context> || !is_const_v<remove_reference_t<_Ty>>);
        }

    protected:
        const void *obj;
        void (*format_func)(format_parse_context &parse_ctx, Context &format_ctx, const void *);

    };

    
    format_arg() noexcept {}

    template <typename T>
    explicit format_arg(const T arg_) :
        arg{arg_} {}

    explicit operator bool() const noexcept {
        return get_type() == _format_arg_type::at_none;
    }

protected:
    /// Has to be in the same order as _format_arg_type!
    std::variant<
        #define FMT_TYPE_LAST_(NAME, TYPE) \
            TYPE
        
        #define FMT_TYPE_(NAME, TYPE) \
            FMT_TYPE_LAST_(NAME, TYPE),
    
        #include "format.dsl.h"
    > arg{};


    inline _format_arg_type get_type() const {
        return (_format_arg_type)arg.index();
    }

};


#pragma region arg_helper
template <typename Context>
struct _format_arg_helper {
protected:
    template <has_formatter<Context> T>
    static auto _create_fake_inst(T &&) {
        using StrippedT = std::remove_cvref_t<T>;

        if constexpr (std::is_same_v<StrippedT, bool>) {
            return static_cast<bool>(1);
        } else if constexpr (std::is_same_v<StrippedT, char>) {
            return static_cast<char>(1);
        } else if constexpr (std::signed_integral<StrippedT> && 
                             sizeof(StrippedT) <= sizeof(int)) {
            return static_cast<int>(1);
        } else if constexpr (std::unsigned_integral<StrippedT> && 
                             sizeof(StrippedT) <= sizeof(unsigned int)) {
            return static_cast<unsigned int>(1);
        } else if constexpr (std::signed_integral<StrippedT> && 
                             sizeof(StrippedT) <= sizeof(long long)) {
            return static_cast<long long>(1);
        } else if constexpr (std::unsigned_integral<StrippedT> &&
                             sizeof(StrippedT) <= sizeof(unsigned long long)) {
            return static_cast<unsigned long long>(1);
        } else {
            // The type-erased type doesn't matter, because it is erased)
            return typename format_arg<Context>::handle{1};
        }
    }

    static auto _create_fake_inst(float) -> float;
    static auto _create_fake_inst(double) -> double;
    static auto _create_fake_inst(long double) -> long double;
    static auto _create_fake_inst(const char *) -> const char *;
    static auto _create_fake_inst(std::string_view) -> std::string_view;
    static auto _create_fake_inst(const std::string &) -> std::string_view;
    static auto _create_fake_inst(nullptr_t) -> const void *;

    template <class T> requires is_void_v<T>
    static auto _create_fake_inst(T *) -> const void *;

public:
    template <typename T>
    using storage_type = decltype(_create_fake_inst(std::declval<T>()));

    template <typename T>
    static constexpr size_t storage_size = sizeof(storage_type<std::remove_cvref_t<T>>);

};
#pragma endregion arg_helper


struct _format_arg_offset {
    /// Offset into the _format_arg_store::buf
    unsigned offset : (sizeof(unsigned) * 8 - 4);
    /// Arg type
    _format_arg_type type : 4;


    _format_arg_offset() noexcept = default;
    _format_arg_offset(size_t offset_, _format_arg_type type_ = _format_arg_type::at_none) noexcept :
        offset{offset_}, type{type_} {}
};


struct _format_arg_name {
    const char *name;
    unsigned idx;
};


#pragma region arg_store
template <typename Context, typename ... As>
struct _format_arg_store {
protected:
    using _helper = _format_arg_helper<Context>;

    static constexpr unsigned count = sizeof...(As);
    static constexpr size_t mem_size = (_helper::template storage_size<As> + ...);
    static constexpr unsigned count_named = named_args_count_v<As...>;


    _format_arg_name names[count_named] = {};
    _format_arg_offset offsets[count] = {};
    unsigned char buf[mem_size] = {};
    
    template <typename T>
    void storeErased(size_t idx, _format_arg_type type, const std::type_identity<T> &val) noexcept {
        assert(idx < count);

        const size_t offset = offsets[idx].offset;
        assert(offset + sizeof(T) <= mem_size);

        if (idx + 1 < count) {
            offsets[idx + 1].offset = offset + sizeof(T);
        }

        memcpy(buf + offset, &val, sizeof(T));
        offsets[idx].type = type;
    }

    template <typename T>
    void store(size_t idx, T &&val) noexcept {
        using ErasedT = typename _helper::template storage_type<std::remove_cvref_t<T>>;

        _format_arg_type type = _format_arg_type::at_none;

        #define FMT_TYPE_(NAME, TYPE) \
            if constexpr (std::is_same_v<ErasedT, TYPE>) { \
                type = _format_arg_type::NAME; \
            } else
        
        #include "format.dsl.h"
        /* else */ {
            static_assert(false, "Shouldn't be reachable");
        }

        storeErased<ErasedT>(idx, type, static_cast<ErasedT>(val));
    }

public:
    _format_arg_store(As &... args) noexcept {
        size_t idx = 0;

        (store(idx++, args), ...);
    }

} __attribute__((packed));


// A specialization for no arguments
template <typename Context>
struct _format_arg_store {};
#pragma endregion arg_store


#pragma region args
template <typename Context>
class format_args {
public:
    format_args() noexcept = default;

    // Specialized overload for no arguments
    format_args(const _format_arg_store<Context> &) noexcept {}

    template <typename ... As>
    format_args(const _format_arg_store<Context, As...> &store_) noexcept :
        count{sizeof...(As)}, store{store_.offsets} {}

    format_arg<Context> get(size_t i) const noexcept {
        if (i >= count) {
            return format_arg<Context>{};
        }

        const _format_arg_offset offset = *get_arg_offset_ptr((unsigned)i);
        const unsigned char *arg_place = get_arg_storage_ptr() + offset.offset;

        switch (offset.type) {
            #define FMT_TYPE_(NAME, TYPE) \
                case _format_arg_type::NAME:
                    return format_arg<Context>{*reinterpret_cast<TYPE *>(arg_place)};

            #include "format.dsl.h"

        NODEFAULT
        }
    }

    format_arg<Context> get(const std::string_view &name) const noexcept {
        const _format_arg_name *arg_name = get_arg_name_ptr(0);

        // TODO: Lookup idx by name, get arg by idx
        for (unsigned i = 0; i < count_named; ++i, ++arg_name) {
            if (arg_name->name == name) {
                return get(arg_name->idx);
            }
        }

        return format_arg<Context>{};
    }

protected:
    unsigned count = 0;
    unsigned count_named = 0;
    const _format_arg_offset *store = nullptr;


    inline const _format_arg_offset *get_arg_offset_ptr(unsigned idx) const noexcept {
        assert(idx < count);
        return &store[idx];
    }

    inline const _format_arg_name *get_arg_name_ptr(unsigned idx) const noexcept {
        assert(idx < count_named);
        return &reinterpret_cast<_format_arg_name *>(store)[-(ptrdiff_t)(count_named - idx)];
    }

    inline const char *get_arg_storage_ptr() const noexcept {
        reinterpret_cast<const unsigned char*>(store + count);
    }

};
#pragma endregion args


#pragma region context
template <typename OutputIt>
requires std::output_iterator<OutputIt, char>
class format_context {
public:
    using iterator = OutputIt;
    using char_type = char;
    
    template <typename T>
    using formatter_type = formatter<T>;


    constexpr format_context(OutputIt it_, format_args<format_context> args_) :
        it{std::move(it_)}, args{args_} {}

    inline format_arg<format_context> arg(size_t idx) const {
        return args.get(idx);
    }

    inline format_arg<format_context> arg(const std::string_view &name) const {
        return args.get(name);
    }

    [[nodiscard]] inline iterator out() {
        return {std::move(it)};
    }

    inline void advance_to(iterator to) {
        it = std::move(to);
    }

    const format_args<format_context> &_get_args() const noexcept {
        return args;
    }

protected:
    format_args<format_context> args;
    iterator it{};

};
#pragma endregion context


// TODO:
//    What                                      Where (in MSVC STL's <format>)
//  - _format_write(OutputIt it, * value)       (L1967-L2722)
//  - struct _format_specs                      (L1036)
//  - visit_format_arg                          (L348)
//  - the rest of the stuff                     (L2722+)

// If I don't finish this by tomorrow (19.02.2022) evening, I should put it aside
// in favor of the rest of the project

}
