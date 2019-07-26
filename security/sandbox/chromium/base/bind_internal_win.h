











#ifndef BASE_BIND_INTERNAL_WIN_H_
#define BASE_BIND_INTERNAL_WIN_H_




#if !defined(ARCH_CPU_X86_64)

namespace base {
namespace internal {

template <typename Functor>
class RunnableAdapter;


template <typename R>
class RunnableAdapter<R(__stdcall *)()> {
 public:
  typedef R (RunType)();

  explicit RunnableAdapter(R(__stdcall *function)())
      : function_(function) {
  }

  R Run() {
    return function_();
  }

 private:
  R (__stdcall *function_)();
};


template <typename R>
class RunnableAdapter<R(__fastcall *)()> {
 public:
  typedef R (RunType)();

  explicit RunnableAdapter(R(__fastcall *function)())
      : function_(function) {
  }

  R Run() {
    return function_();
  }

 private:
  R (__fastcall *function_)();
};


template <typename R, typename A1>
class RunnableAdapter<R(__stdcall *)(A1)> {
 public:
  typedef R (RunType)(A1);

  explicit RunnableAdapter(R(__stdcall *function)(A1))
      : function_(function) {
  }

  R Run(typename CallbackParamTraits<A1>::ForwardType a1) {
    return function_(a1);
  }

 private:
  R (__stdcall *function_)(A1);
};


template <typename R, typename A1>
class RunnableAdapter<R(__fastcall *)(A1)> {
 public:
  typedef R (RunType)(A1);

  explicit RunnableAdapter(R(__fastcall *function)(A1))
      : function_(function) {
  }

  R Run(typename CallbackParamTraits<A1>::ForwardType a1) {
    return function_(a1);
  }

 private:
  R (__fastcall *function_)(A1);
};


template <typename R, typename A1, typename A2>
class RunnableAdapter<R(__stdcall *)(A1, A2)> {
 public:
  typedef R (RunType)(A1, A2);

  explicit RunnableAdapter(R(__stdcall *function)(A1, A2))
      : function_(function) {
  }

  R Run(typename CallbackParamTraits<A1>::ForwardType a1,
      typename CallbackParamTraits<A2>::ForwardType a2) {
    return function_(a1, a2);
  }

 private:
  R (__stdcall *function_)(A1, A2);
};


template <typename R, typename A1, typename A2>
class RunnableAdapter<R(__fastcall *)(A1, A2)> {
 public:
  typedef R (RunType)(A1, A2);

  explicit RunnableAdapter(R(__fastcall *function)(A1, A2))
      : function_(function) {
  }

  R Run(typename CallbackParamTraits<A1>::ForwardType a1,
      typename CallbackParamTraits<A2>::ForwardType a2) {
    return function_(a1, a2);
  }

 private:
  R (__fastcall *function_)(A1, A2);
};


template <typename R, typename A1, typename A2, typename A3>
class RunnableAdapter<R(__stdcall *)(A1, A2, A3)> {
 public:
  typedef R (RunType)(A1, A2, A3);

  explicit RunnableAdapter(R(__stdcall *function)(A1, A2, A3))
      : function_(function) {
  }

  R Run(typename CallbackParamTraits<A1>::ForwardType a1,
      typename CallbackParamTraits<A2>::ForwardType a2,
      typename CallbackParamTraits<A3>::ForwardType a3) {
    return function_(a1, a2, a3);
  }

 private:
  R (__stdcall *function_)(A1, A2, A3);
};


template <typename R, typename A1, typename A2, typename A3>
class RunnableAdapter<R(__fastcall *)(A1, A2, A3)> {
 public:
  typedef R (RunType)(A1, A2, A3);

  explicit RunnableAdapter(R(__fastcall *function)(A1, A2, A3))
      : function_(function) {
  }

  R Run(typename CallbackParamTraits<A1>::ForwardType a1,
      typename CallbackParamTraits<A2>::ForwardType a2,
      typename CallbackParamTraits<A3>::ForwardType a3) {
    return function_(a1, a2, a3);
  }

 private:
  R (__fastcall *function_)(A1, A2, A3);
};


template <typename R, typename A1, typename A2, typename A3, typename A4>
class RunnableAdapter<R(__stdcall *)(A1, A2, A3, A4)> {
 public:
  typedef R (RunType)(A1, A2, A3, A4);

  explicit RunnableAdapter(R(__stdcall *function)(A1, A2, A3, A4))
      : function_(function) {
  }

  R Run(typename CallbackParamTraits<A1>::ForwardType a1,
      typename CallbackParamTraits<A2>::ForwardType a2,
      typename CallbackParamTraits<A3>::ForwardType a3,
      typename CallbackParamTraits<A4>::ForwardType a4) {
    return function_(a1, a2, a3, a4);
  }

 private:
  R (__stdcall *function_)(A1, A2, A3, A4);
};


template <typename R, typename A1, typename A2, typename A3, typename A4>
class RunnableAdapter<R(__fastcall *)(A1, A2, A3, A4)> {
 public:
  typedef R (RunType)(A1, A2, A3, A4);

  explicit RunnableAdapter(R(__fastcall *function)(A1, A2, A3, A4))
      : function_(function) {
  }

  R Run(typename CallbackParamTraits<A1>::ForwardType a1,
      typename CallbackParamTraits<A2>::ForwardType a2,
      typename CallbackParamTraits<A3>::ForwardType a3,
      typename CallbackParamTraits<A4>::ForwardType a4) {
    return function_(a1, a2, a3, a4);
  }

 private:
  R (__fastcall *function_)(A1, A2, A3, A4);
};


template <typename R, typename A1, typename A2, typename A3, typename A4,
    typename A5>
class RunnableAdapter<R(__stdcall *)(A1, A2, A3, A4, A5)> {
 public:
  typedef R (RunType)(A1, A2, A3, A4, A5);

  explicit RunnableAdapter(R(__stdcall *function)(A1, A2, A3, A4, A5))
      : function_(function) {
  }

  R Run(typename CallbackParamTraits<A1>::ForwardType a1,
      typename CallbackParamTraits<A2>::ForwardType a2,
      typename CallbackParamTraits<A3>::ForwardType a3,
      typename CallbackParamTraits<A4>::ForwardType a4,
      typename CallbackParamTraits<A5>::ForwardType a5) {
    return function_(a1, a2, a3, a4, a5);
  }

 private:
  R (__stdcall *function_)(A1, A2, A3, A4, A5);
};


template <typename R, typename A1, typename A2, typename A3, typename A4,
    typename A5>
class RunnableAdapter<R(__fastcall *)(A1, A2, A3, A4, A5)> {
 public:
  typedef R (RunType)(A1, A2, A3, A4, A5);

  explicit RunnableAdapter(R(__fastcall *function)(A1, A2, A3, A4, A5))
      : function_(function) {
  }

  R Run(typename CallbackParamTraits<A1>::ForwardType a1,
      typename CallbackParamTraits<A2>::ForwardType a2,
      typename CallbackParamTraits<A3>::ForwardType a3,
      typename CallbackParamTraits<A4>::ForwardType a4,
      typename CallbackParamTraits<A5>::ForwardType a5) {
    return function_(a1, a2, a3, a4, a5);
  }

 private:
  R (__fastcall *function_)(A1, A2, A3, A4, A5);
};


template <typename R, typename A1, typename A2, typename A3, typename A4,
    typename A5, typename A6>
class RunnableAdapter<R(__stdcall *)(A1, A2, A3, A4, A5, A6)> {
 public:
  typedef R (RunType)(A1, A2, A3, A4, A5, A6);

  explicit RunnableAdapter(R(__stdcall *function)(A1, A2, A3, A4, A5, A6))
      : function_(function) {
  }

  R Run(typename CallbackParamTraits<A1>::ForwardType a1,
      typename CallbackParamTraits<A2>::ForwardType a2,
      typename CallbackParamTraits<A3>::ForwardType a3,
      typename CallbackParamTraits<A4>::ForwardType a4,
      typename CallbackParamTraits<A5>::ForwardType a5,
      typename CallbackParamTraits<A6>::ForwardType a6) {
    return function_(a1, a2, a3, a4, a5, a6);
  }

 private:
  R (__stdcall *function_)(A1, A2, A3, A4, A5, A6);
};


template <typename R, typename A1, typename A2, typename A3, typename A4,
    typename A5, typename A6>
class RunnableAdapter<R(__fastcall *)(A1, A2, A3, A4, A5, A6)> {
 public:
  typedef R (RunType)(A1, A2, A3, A4, A5, A6);

  explicit RunnableAdapter(R(__fastcall *function)(A1, A2, A3, A4, A5, A6))
      : function_(function) {
  }

  R Run(typename CallbackParamTraits<A1>::ForwardType a1,
      typename CallbackParamTraits<A2>::ForwardType a2,
      typename CallbackParamTraits<A3>::ForwardType a3,
      typename CallbackParamTraits<A4>::ForwardType a4,
      typename CallbackParamTraits<A5>::ForwardType a5,
      typename CallbackParamTraits<A6>::ForwardType a6) {
    return function_(a1, a2, a3, a4, a5, a6);
  }

 private:
  R (__fastcall *function_)(A1, A2, A3, A4, A5, A6);
};


template <typename R, typename A1, typename A2, typename A3, typename A4,
    typename A5, typename A6, typename A7>
class RunnableAdapter<R(__stdcall *)(A1, A2, A3, A4, A5, A6, A7)> {
 public:
  typedef R (RunType)(A1, A2, A3, A4, A5, A6, A7);

  explicit RunnableAdapter(R(__stdcall *function)(A1, A2, A3, A4, A5, A6, A7))
      : function_(function) {
  }

  R Run(typename CallbackParamTraits<A1>::ForwardType a1,
      typename CallbackParamTraits<A2>::ForwardType a2,
      typename CallbackParamTraits<A3>::ForwardType a3,
      typename CallbackParamTraits<A4>::ForwardType a4,
      typename CallbackParamTraits<A5>::ForwardType a5,
      typename CallbackParamTraits<A6>::ForwardType a6,
      typename CallbackParamTraits<A7>::ForwardType a7) {
    return function_(a1, a2, a3, a4, a5, a6, a7);
  }

 private:
  R (__stdcall *function_)(A1, A2, A3, A4, A5, A6, A7);
};


template <typename R, typename A1, typename A2, typename A3, typename A4,
    typename A5, typename A6, typename A7>
class RunnableAdapter<R(__fastcall *)(A1, A2, A3, A4, A5, A6, A7)> {
 public:
  typedef R (RunType)(A1, A2, A3, A4, A5, A6, A7);

  explicit RunnableAdapter(R(__fastcall *function)(A1, A2, A3, A4, A5, A6, A7))
      : function_(function) {
  }

  R Run(typename CallbackParamTraits<A1>::ForwardType a1,
      typename CallbackParamTraits<A2>::ForwardType a2,
      typename CallbackParamTraits<A3>::ForwardType a3,
      typename CallbackParamTraits<A4>::ForwardType a4,
      typename CallbackParamTraits<A5>::ForwardType a5,
      typename CallbackParamTraits<A6>::ForwardType a6,
      typename CallbackParamTraits<A7>::ForwardType a7) {
    return function_(a1, a2, a3, a4, a5, a6, a7);
  }

 private:
  R (__fastcall *function_)(A1, A2, A3, A4, A5, A6, A7);
};

}  
}  

#endif  

#endif  
