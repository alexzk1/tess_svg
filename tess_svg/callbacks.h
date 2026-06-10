#ifndef CALLBACKS_H
#define CALLBACKS_H

#include <cstddef>
#include <functional>

template <typename Function>
struct function_traits : public function_traits<decltype(&Function::operator())>
{
};

template <typename ClassType, typename ReturnType, typename... Args>
struct function_traits<ReturnType (ClassType::*)(Args...) const>
{
    using pointer = ReturnType (*)(Args...);
    using function = std::function<ReturnType(Args...)>;
};

template <typename Function>
typename function_traits<Function>::pointer to_function_pointer(const Function &lambda)
{
    return static_cast<typename function_traits<Function>::pointer>(lambda);
}

template <typename Function>
typename function_traits<Function>::function to_function(const Function &lambda)
{
    return static_cast<typename function_traits<Function>::function>(lambda);
}

// some more black magic

template <const size_t UniqueId, typename Res, typename... ArgTypes>
struct fun_ptr_helper
{
  public:
    using function_type = std::function<Res(ArgTypes...)>;

    static void bind(function_type &&f)
    {
        instance().fn_.swap(std::move(f));
    }

    static void bind(const function_type &f)
    {
        instance().fn_ = f;
    }

    static Res invoke(ArgTypes... args)
    {
        return instance().fn_(args...);
    }

    using pointer_type = decltype(&fun_ptr_helper::invoke);
    static pointer_type ptr()
    {
        return &invoke;
    }

  private:
    static fun_ptr_helper &instance()
    {
        static fun_ptr_helper inst_;
        return inst_;
    }

    fun_ptr_helper()
    {
    }

    function_type fn_;
};

// UniqueId must be manually set at compile time to unique value, that generates unique static stub
template <const size_t UniqueId, typename Res, typename... ArgTypes>
typename fun_ptr_helper<UniqueId, Res, ArgTypes...>::pointer_type
get_fn_ptr(const std::function<Res(ArgTypes...)> &f)
{
    fun_ptr_helper<UniqueId, Res, ArgTypes...>::bind(f);
    return fun_ptr_helper<UniqueId, Res, ArgTypes...>::ptr();
}

using funcptr_type = void (*)();
template <const size_t UniqueId, typename Function>
funcptr_type to_glu_callback(const Function &lambda)
{
    // NOLINTNEXTLINE
    return reinterpret_cast<funcptr_type>(get_fn_ptr<UniqueId>(to_function(lambda)));
}

#endif // CALLBACKS_H
