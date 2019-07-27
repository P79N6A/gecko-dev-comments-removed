






#ifndef BASE_BIND_INTERNAL_WIN_H_
#define BASE_BIND_INTERNAL_WIN_H_




#if !defined(ARCH_CPU_X86_64)

namespace base {
namespace internal {

template <typename Functor>
class RunnableAdapter;


template <typename R, typename... Args>
class RunnableAdapter<R(__stdcall *)(Args...)> {
 public:
  typedef R (RunType)(Args...);

  explicit RunnableAdapter(R(__stdcall *function)(Args...))
      : function_(function) {
  }

  R Run(typename CallbackParamTraits<Args>::ForwardType... args) {
    return function_(args...);
  }

 private:
  R (__stdcall *function_)(Args...);
};


template <typename R, typename... Args>
class RunnableAdapter<R(__fastcall *)(Args...)> {
 public:
  typedef R (RunType)(Args...);

  explicit RunnableAdapter(R(__fastcall *function)(Args...))
      : function_(function) {
  }

  R Run(typename CallbackParamTraits<Args>::ForwardType... args) {
    return function_(args...);
  }

 private:
  R (__fastcall *function_)(Args...);
};

}  
}  

#endif  

#endif  
