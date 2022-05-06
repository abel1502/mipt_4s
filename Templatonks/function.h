#pragma once

#include <ACL/general.h>
#include <ACL/defer.h>
#include <ACL/type_traits.h>
#include <concepts>
#include <memory>
#include <initializer_list>
#include <stdexcept>
#include <algorithm>
#include <functional>  // For std::bad_function_call
#include <new>
#include <bit>
#include <typeinfo>


namespace mylib {


template <typename T>
class Function;


// TODO: Maybe also optimize for a shared ICallable


template <typename R, typename ... As>
class Function<R (As...)> {
public:
    #pragma region Aliases & etc.
    using result_type = R;
    #pragma endregion Aliases & etc.

    #pragma region Protected helpers
protected:
    static constexpr size_t small_func_size = 63;

    #pragma region Callable classes
    class ICallable {
    public:
        virtual R operator()(As ... args) = 0;
        virtual void copy_to(Function &new_host) const = 0;
        virtual void move_to(Function &new_host, Function &old_host) = 0;
        virtual const std::type_info &target_type() const = 0;
        virtual void *target() = 0;

        virtual ~ICallable() = default;

        inline const void *target() const {
            return const_cast<ICallable *>(this)->target();
        }
    };

    friend ICallable;

    struct Empty {
        inline explicit Empty(uint8_t = 0) {}
    } alignas(1);

    template <typename F, bool Small>
    class FunctorCallable : public ICallable {
    public:
        FunctorCallable(F functor) noexcept(std::is_nothrow_move_constructible_v<F>) :
            functor{std::move(functor)} {}

        virtual R operator()(As ... args) override {
            return std::move(functor)(std::forward<As>(args)...);
        }

        virtual void copy_to(Function &new_host) const override {
            if (new_host.get_ptr() == this) {
                return;
            }

            new_host = nullptr;

            if constexpr (Small) {
                // static_assert(sizeof(*this) <= small_func_size,
                //               "Only small functors should be marked");

                new (new_host.buf_) FunctorCallable(functor);
            } else {
                new_host.ptr_ = new FunctorCallable(functor);
            }

            new_host.is_small(Small);
        }

        virtual void move_to(Function &new_host, Function &old_host) override {
            if (new_host.get_ptr() == this) {
                return;
            }

            new_host = nullptr;

            if constexpr (Small) {
                // static_assert(sizeof(*this) <= small_func_size,
                //               "Only small functors should be marked");

                new (new_host.buf_) FunctorCallable(std::move(functor));
            } else {
                new_host.ptr_ = this;
                old_host.ptr_ = nullptr;
            }

            new_host.is_small(Small);

            old_host = nullptr;
        }

        virtual const std::type_info &target_type() const override {
            return typeid(F);
        }

        virtual void *target() override {
            return &functor;
        }

    protected:
        [[no_unique_address]] F functor{};
    };

    friend FunctorCallable;
    #pragma endregion Callable classes

public:
    #pragma endregion Protected helpers
public:
    #pragma region Constructors
    Function() noexcept {}

    Function(std::nullptr_t) noexcept {}

    Function(const Function &other) :
        Function() {
        *this = other;
    }

    Function &operator=(std::nullptr_t) {
        this->~Function();
        ptr_ = nullptr;
        is_small(false);

        return *this;
    }

    Function &operator=(const Function &other) {
        if (&other == this) {
            return *this;
        }

        ICallable *other_func = other.get_ptr();

        if (!other_func) {
            return *this = nullptr;
        }

        other_func->copy_to(*this);

        return *this;
    }

    Function(Function &&other) :
        Function() {
        *this = std::move(other);
    }

    Function &operator=(Function &&other) {
        if (&other == this) {
            return *this;
        }

        ICallable *other_func = other.get_ptr();

        if (!other_func) {
            return *this = nullptr;
        }

        other_func->move_to(*this, other);

        return *this;
    }

    template <typename F>
    Function &operator=(F &&functor) {
        this->~Function();

        new (this) Function(std::forward<F>(functor));

        return *this;
    }

    template <typename F>
    requires (!std::same_as<std::remove_reference_t<F>, Function>)
    Function(F &&functor) {
        if constexpr (sizeof(FunctorCallable<std::remove_reference_t<F>, true>) <= small_func_size) {
            new (buf_) FunctorCallable<std::remove_reference_t<F>, true >(std::forward<F>(functor));
            is_small(true);
        } else {
            ptr_ = new FunctorCallable<std::remove_reference_t<F>, false>(std::forward<F>(functor));
            is_small(false);
        }
    }
    #pragma endregion Constructors

    #pragma region Destructor
    ~Function() {
        ICallable *func = get_ptr();

        if (!func) {
            return;
        }

        if (is_small()) {
            func->~ICallable();
            return;
        }

        delete func;

        ptr_ = nullptr;
    }
    #pragma endregion Destructor

    #pragma region Call
    R operator()(As ... args) const {
        ICallable *func = get_ptr();

        if (!func) {
            throw std::bad_function_call();
        }

        return (*func)(std::forward<As>(args)...);
    }
    #pragma endregion Call

    #pragma region Basic information
    explicit operator bool() const noexcept {
        return get_ptr() != nullptr;
    }

    inline friend bool operator==(const Function &self, nullptr_t) {
        return !self;
    }

    inline friend bool operator==(nullptr_t, const Function &self) {
        return !self;
    }

    constexpr bool is_small() const {
        return buf_[small_func_size];
    }
    #pragma endregion Basic information

    #pragma region Target
    const std::type_info &target_type() const noexcept {
        ICallable *func = get_ptr();

        if (!func) {
            return typeid(void);
        }

        return func->target_type();
    }

    template <typename T>
    T *target() noexcept {
        ICallable *func = get_ptr();

        if (!func) {
            return nullptr;
        }

        if (typeid(T) != func->target_type()) {
            return nullptr;
        }

        return reinterpret_cast<T *>(func->target());
    }

    template <typename T>
    const T *target() const noexcept {
        return const_cast<Function *>(this)->target<T>();
    }
    #pragma endregion Target

protected:
    #pragma region Fields
    union {
        alignas(sizeof(void *)) ICallable *ptr_;
        alignas(sizeof(void *)) mutable uint8_t buf_[small_func_size + 1] = {};
    };
    #pragma endregion Fields


    #pragma region Protected interface
    ICallable *get_ptr() const {
        if (is_small()) {
            return reinterpret_cast<ICallable *>(buf_);
        }

        return ptr_;
    }

    constexpr void is_small(bool value) {
        buf_[small_func_size] = value;
    }
    #pragma endregion Protected interface

};


#pragma region Deduction guides
template<typename R, typename ... As>
Function(Function<R (As...)> &) -> Function<R (As...)>;

template<typename R, typename ... As>
Function(Function<R (As...)> &&) -> Function<R (As...)>;

template<typename R, typename ... As>
Function(R (*)(As...)) -> Function<R (As...)>;

#pragma region Helper
namespace _impl {

template <typename F>
struct functor_call_type;

#define FUNCTOR_CALL_TYPE_DECL_(FLAGS)                      \
    template <typename Cls, typename R, typename ... As>    \
    struct functor_call_type<R (Cls::*)(As...) FLAGS> {     \
        using type = R (As...);                             \
    };

FUNCTOR_CALL_TYPE_DECL_(MACROEMPTY)
FUNCTOR_CALL_TYPE_DECL_(noexcept)
FUNCTOR_CALL_TYPE_DECL_(const)
FUNCTOR_CALL_TYPE_DECL_(const noexcept)
FUNCTOR_CALL_TYPE_DECL_(&)
FUNCTOR_CALL_TYPE_DECL_(& noexcept)
FUNCTOR_CALL_TYPE_DECL_(const &)
FUNCTOR_CALL_TYPE_DECL_(const & noexcept)

#undef FUNCTOR_CALL_TYPE_DECL_

template <typename F>
using functor_call_type_t = typename functor_call_type<F>::type;

}
#pragma endregion Helper

template <typename F>
Function(F f) -> Function<_impl::functor_call_type_t<decltype(&F::operator())>>;

//template<typename ... As>
//Function(std::invocable<As...> auto f) -> Function<std::invoke_result_t<decltype(f), As...> (As...)>;
#pragma endregion Deduction guides


}
