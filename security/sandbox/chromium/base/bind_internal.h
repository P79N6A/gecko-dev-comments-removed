








#ifndef BASE_BIND_INTERNAL_H_
#define BASE_BIND_INTERNAL_H_

#include "base/bind_helpers.h"
#include "base/callback_internal.h"
#include "base/memory/raw_scoped_refptr_mismatch_checker.h"
#include "base/memory/weak_ptr.h"
#include "base/template_util.h"
#include "build/build_config.h"

#if defined(OS_WIN)
#include "base/bind_internal_win.h"
#endif

namespace base {
namespace internal {











































































template <typename Functor>
class RunnableAdapter;


template <typename R>
class RunnableAdapter<R(*)()> {
 public:
  typedef R (RunType)();

  explicit RunnableAdapter(R(*function)())
      : function_(function) {
  }

  R Run() {
    return function_();
  }

 private:
  R (*function_)();
};


template <typename R, typename T>
class RunnableAdapter<R(T::*)()> {
 public:
  typedef R (RunType)(T*);
  typedef true_type IsMethod;

  explicit RunnableAdapter(R(T::*method)())
      : method_(method) {
  }

  R Run(T* object) {
    return (object->*method_)();
  }

 private:
  R (T::*method_)();
};


template <typename R, typename T>
class RunnableAdapter<R(T::*)() const> {
 public:
  typedef R (RunType)(const T*);
  typedef true_type IsMethod;

  explicit RunnableAdapter(R(T::*method)() const)
      : method_(method) {
  }

  R Run(const T* object) {
    return (object->*method_)();
  }

 private:
  R (T::*method_)() const;
};


template <typename R, typename A1>
class RunnableAdapter<R(*)(A1)> {
 public:
  typedef R (RunType)(A1);

  explicit RunnableAdapter(R(*function)(A1))
      : function_(function) {
  }

  R Run(typename CallbackParamTraits<A1>::ForwardType a1) {
    return function_(CallbackForward(a1));
  }

 private:
  R (*function_)(A1);
};


template <typename R, typename T, typename A1>
class RunnableAdapter<R(T::*)(A1)> {
 public:
  typedef R (RunType)(T*, A1);
  typedef true_type IsMethod;

  explicit RunnableAdapter(R(T::*method)(A1))
      : method_(method) {
  }

  R Run(T* object, typename CallbackParamTraits<A1>::ForwardType a1) {
    return (object->*method_)(CallbackForward(a1));
  }

 private:
  R (T::*method_)(A1);
};


template <typename R, typename T, typename A1>
class RunnableAdapter<R(T::*)(A1) const> {
 public:
  typedef R (RunType)(const T*, A1);
  typedef true_type IsMethod;

  explicit RunnableAdapter(R(T::*method)(A1) const)
      : method_(method) {
  }

  R Run(const T* object, typename CallbackParamTraits<A1>::ForwardType a1) {
    return (object->*method_)(CallbackForward(a1));
  }

 private:
  R (T::*method_)(A1) const;
};


template <typename R, typename A1, typename A2>
class RunnableAdapter<R(*)(A1, A2)> {
 public:
  typedef R (RunType)(A1, A2);

  explicit RunnableAdapter(R(*function)(A1, A2))
      : function_(function) {
  }

  R Run(typename CallbackParamTraits<A1>::ForwardType a1,
      typename CallbackParamTraits<A2>::ForwardType a2) {
    return function_(CallbackForward(a1), CallbackForward(a2));
  }

 private:
  R (*function_)(A1, A2);
};


template <typename R, typename T, typename A1, typename A2>
class RunnableAdapter<R(T::*)(A1, A2)> {
 public:
  typedef R (RunType)(T*, A1, A2);
  typedef true_type IsMethod;

  explicit RunnableAdapter(R(T::*method)(A1, A2))
      : method_(method) {
  }

  R Run(T* object, typename CallbackParamTraits<A1>::ForwardType a1,
      typename CallbackParamTraits<A2>::ForwardType a2) {
    return (object->*method_)(CallbackForward(a1), CallbackForward(a2));
  }

 private:
  R (T::*method_)(A1, A2);
};


template <typename R, typename T, typename A1, typename A2>
class RunnableAdapter<R(T::*)(A1, A2) const> {
 public:
  typedef R (RunType)(const T*, A1, A2);
  typedef true_type IsMethod;

  explicit RunnableAdapter(R(T::*method)(A1, A2) const)
      : method_(method) {
  }

  R Run(const T* object, typename CallbackParamTraits<A1>::ForwardType a1,
      typename CallbackParamTraits<A2>::ForwardType a2) {
    return (object->*method_)(CallbackForward(a1), CallbackForward(a2));
  }

 private:
  R (T::*method_)(A1, A2) const;
};


template <typename R, typename A1, typename A2, typename A3>
class RunnableAdapter<R(*)(A1, A2, A3)> {
 public:
  typedef R (RunType)(A1, A2, A3);

  explicit RunnableAdapter(R(*function)(A1, A2, A3))
      : function_(function) {
  }

  R Run(typename CallbackParamTraits<A1>::ForwardType a1,
      typename CallbackParamTraits<A2>::ForwardType a2,
      typename CallbackParamTraits<A3>::ForwardType a3) {
    return function_(CallbackForward(a1), CallbackForward(a2),
        CallbackForward(a3));
  }

 private:
  R (*function_)(A1, A2, A3);
};


template <typename R, typename T, typename A1, typename A2, typename A3>
class RunnableAdapter<R(T::*)(A1, A2, A3)> {
 public:
  typedef R (RunType)(T*, A1, A2, A3);
  typedef true_type IsMethod;

  explicit RunnableAdapter(R(T::*method)(A1, A2, A3))
      : method_(method) {
  }

  R Run(T* object, typename CallbackParamTraits<A1>::ForwardType a1,
      typename CallbackParamTraits<A2>::ForwardType a2,
      typename CallbackParamTraits<A3>::ForwardType a3) {
    return (object->*method_)(CallbackForward(a1), CallbackForward(a2),
        CallbackForward(a3));
  }

 private:
  R (T::*method_)(A1, A2, A3);
};


template <typename R, typename T, typename A1, typename A2, typename A3>
class RunnableAdapter<R(T::*)(A1, A2, A3) const> {
 public:
  typedef R (RunType)(const T*, A1, A2, A3);
  typedef true_type IsMethod;

  explicit RunnableAdapter(R(T::*method)(A1, A2, A3) const)
      : method_(method) {
  }

  R Run(const T* object, typename CallbackParamTraits<A1>::ForwardType a1,
      typename CallbackParamTraits<A2>::ForwardType a2,
      typename CallbackParamTraits<A3>::ForwardType a3) {
    return (object->*method_)(CallbackForward(a1), CallbackForward(a2),
        CallbackForward(a3));
  }

 private:
  R (T::*method_)(A1, A2, A3) const;
};


template <typename R, typename A1, typename A2, typename A3, typename A4>
class RunnableAdapter<R(*)(A1, A2, A3, A4)> {
 public:
  typedef R (RunType)(A1, A2, A3, A4);

  explicit RunnableAdapter(R(*function)(A1, A2, A3, A4))
      : function_(function) {
  }

  R Run(typename CallbackParamTraits<A1>::ForwardType a1,
      typename CallbackParamTraits<A2>::ForwardType a2,
      typename CallbackParamTraits<A3>::ForwardType a3,
      typename CallbackParamTraits<A4>::ForwardType a4) {
    return function_(CallbackForward(a1), CallbackForward(a2),
        CallbackForward(a3), CallbackForward(a4));
  }

 private:
  R (*function_)(A1, A2, A3, A4);
};


template <typename R, typename T, typename A1, typename A2, typename A3,
    typename A4>
class RunnableAdapter<R(T::*)(A1, A2, A3, A4)> {
 public:
  typedef R (RunType)(T*, A1, A2, A3, A4);
  typedef true_type IsMethod;

  explicit RunnableAdapter(R(T::*method)(A1, A2, A3, A4))
      : method_(method) {
  }

  R Run(T* object, typename CallbackParamTraits<A1>::ForwardType a1,
      typename CallbackParamTraits<A2>::ForwardType a2,
      typename CallbackParamTraits<A3>::ForwardType a3,
      typename CallbackParamTraits<A4>::ForwardType a4) {
    return (object->*method_)(CallbackForward(a1), CallbackForward(a2),
        CallbackForward(a3), CallbackForward(a4));
  }

 private:
  R (T::*method_)(A1, A2, A3, A4);
};


template <typename R, typename T, typename A1, typename A2, typename A3,
    typename A4>
class RunnableAdapter<R(T::*)(A1, A2, A3, A4) const> {
 public:
  typedef R (RunType)(const T*, A1, A2, A3, A4);
  typedef true_type IsMethod;

  explicit RunnableAdapter(R(T::*method)(A1, A2, A3, A4) const)
      : method_(method) {
  }

  R Run(const T* object, typename CallbackParamTraits<A1>::ForwardType a1,
      typename CallbackParamTraits<A2>::ForwardType a2,
      typename CallbackParamTraits<A3>::ForwardType a3,
      typename CallbackParamTraits<A4>::ForwardType a4) {
    return (object->*method_)(CallbackForward(a1), CallbackForward(a2),
        CallbackForward(a3), CallbackForward(a4));
  }

 private:
  R (T::*method_)(A1, A2, A3, A4) const;
};


template <typename R, typename A1, typename A2, typename A3, typename A4,
    typename A5>
class RunnableAdapter<R(*)(A1, A2, A3, A4, A5)> {
 public:
  typedef R (RunType)(A1, A2, A3, A4, A5);

  explicit RunnableAdapter(R(*function)(A1, A2, A3, A4, A5))
      : function_(function) {
  }

  R Run(typename CallbackParamTraits<A1>::ForwardType a1,
      typename CallbackParamTraits<A2>::ForwardType a2,
      typename CallbackParamTraits<A3>::ForwardType a3,
      typename CallbackParamTraits<A4>::ForwardType a4,
      typename CallbackParamTraits<A5>::ForwardType a5) {
    return function_(CallbackForward(a1), CallbackForward(a2),
        CallbackForward(a3), CallbackForward(a4), CallbackForward(a5));
  }

 private:
  R (*function_)(A1, A2, A3, A4, A5);
};


template <typename R, typename T, typename A1, typename A2, typename A3,
    typename A4, typename A5>
class RunnableAdapter<R(T::*)(A1, A2, A3, A4, A5)> {
 public:
  typedef R (RunType)(T*, A1, A2, A3, A4, A5);
  typedef true_type IsMethod;

  explicit RunnableAdapter(R(T::*method)(A1, A2, A3, A4, A5))
      : method_(method) {
  }

  R Run(T* object, typename CallbackParamTraits<A1>::ForwardType a1,
      typename CallbackParamTraits<A2>::ForwardType a2,
      typename CallbackParamTraits<A3>::ForwardType a3,
      typename CallbackParamTraits<A4>::ForwardType a4,
      typename CallbackParamTraits<A5>::ForwardType a5) {
    return (object->*method_)(CallbackForward(a1), CallbackForward(a2),
        CallbackForward(a3), CallbackForward(a4), CallbackForward(a5));
  }

 private:
  R (T::*method_)(A1, A2, A3, A4, A5);
};


template <typename R, typename T, typename A1, typename A2, typename A3,
    typename A4, typename A5>
class RunnableAdapter<R(T::*)(A1, A2, A3, A4, A5) const> {
 public:
  typedef R (RunType)(const T*, A1, A2, A3, A4, A5);
  typedef true_type IsMethod;

  explicit RunnableAdapter(R(T::*method)(A1, A2, A3, A4, A5) const)
      : method_(method) {
  }

  R Run(const T* object, typename CallbackParamTraits<A1>::ForwardType a1,
      typename CallbackParamTraits<A2>::ForwardType a2,
      typename CallbackParamTraits<A3>::ForwardType a3,
      typename CallbackParamTraits<A4>::ForwardType a4,
      typename CallbackParamTraits<A5>::ForwardType a5) {
    return (object->*method_)(CallbackForward(a1), CallbackForward(a2),
        CallbackForward(a3), CallbackForward(a4), CallbackForward(a5));
  }

 private:
  R (T::*method_)(A1, A2, A3, A4, A5) const;
};


template <typename R, typename A1, typename A2, typename A3, typename A4,
    typename A5, typename A6>
class RunnableAdapter<R(*)(A1, A2, A3, A4, A5, A6)> {
 public:
  typedef R (RunType)(A1, A2, A3, A4, A5, A6);

  explicit RunnableAdapter(R(*function)(A1, A2, A3, A4, A5, A6))
      : function_(function) {
  }

  R Run(typename CallbackParamTraits<A1>::ForwardType a1,
      typename CallbackParamTraits<A2>::ForwardType a2,
      typename CallbackParamTraits<A3>::ForwardType a3,
      typename CallbackParamTraits<A4>::ForwardType a4,
      typename CallbackParamTraits<A5>::ForwardType a5,
      typename CallbackParamTraits<A6>::ForwardType a6) {
    return function_(CallbackForward(a1), CallbackForward(a2),
        CallbackForward(a3), CallbackForward(a4), CallbackForward(a5),
        CallbackForward(a6));
  }

 private:
  R (*function_)(A1, A2, A3, A4, A5, A6);
};


template <typename R, typename T, typename A1, typename A2, typename A3,
    typename A4, typename A5, typename A6>
class RunnableAdapter<R(T::*)(A1, A2, A3, A4, A5, A6)> {
 public:
  typedef R (RunType)(T*, A1, A2, A3, A4, A5, A6);
  typedef true_type IsMethod;

  explicit RunnableAdapter(R(T::*method)(A1, A2, A3, A4, A5, A6))
      : method_(method) {
  }

  R Run(T* object, typename CallbackParamTraits<A1>::ForwardType a1,
      typename CallbackParamTraits<A2>::ForwardType a2,
      typename CallbackParamTraits<A3>::ForwardType a3,
      typename CallbackParamTraits<A4>::ForwardType a4,
      typename CallbackParamTraits<A5>::ForwardType a5,
      typename CallbackParamTraits<A6>::ForwardType a6) {
    return (object->*method_)(CallbackForward(a1), CallbackForward(a2),
        CallbackForward(a3), CallbackForward(a4), CallbackForward(a5),
        CallbackForward(a6));
  }

 private:
  R (T::*method_)(A1, A2, A3, A4, A5, A6);
};


template <typename R, typename T, typename A1, typename A2, typename A3,
    typename A4, typename A5, typename A6>
class RunnableAdapter<R(T::*)(A1, A2, A3, A4, A5, A6) const> {
 public:
  typedef R (RunType)(const T*, A1, A2, A3, A4, A5, A6);
  typedef true_type IsMethod;

  explicit RunnableAdapter(R(T::*method)(A1, A2, A3, A4, A5, A6) const)
      : method_(method) {
  }

  R Run(const T* object, typename CallbackParamTraits<A1>::ForwardType a1,
      typename CallbackParamTraits<A2>::ForwardType a2,
      typename CallbackParamTraits<A3>::ForwardType a3,
      typename CallbackParamTraits<A4>::ForwardType a4,
      typename CallbackParamTraits<A5>::ForwardType a5,
      typename CallbackParamTraits<A6>::ForwardType a6) {
    return (object->*method_)(CallbackForward(a1), CallbackForward(a2),
        CallbackForward(a3), CallbackForward(a4), CallbackForward(a5),
        CallbackForward(a6));
  }

 private:
  R (T::*method_)(A1, A2, A3, A4, A5, A6) const;
};


template <typename R, typename A1, typename A2, typename A3, typename A4,
    typename A5, typename A6, typename A7>
class RunnableAdapter<R(*)(A1, A2, A3, A4, A5, A6, A7)> {
 public:
  typedef R (RunType)(A1, A2, A3, A4, A5, A6, A7);

  explicit RunnableAdapter(R(*function)(A1, A2, A3, A4, A5, A6, A7))
      : function_(function) {
  }

  R Run(typename CallbackParamTraits<A1>::ForwardType a1,
      typename CallbackParamTraits<A2>::ForwardType a2,
      typename CallbackParamTraits<A3>::ForwardType a3,
      typename CallbackParamTraits<A4>::ForwardType a4,
      typename CallbackParamTraits<A5>::ForwardType a5,
      typename CallbackParamTraits<A6>::ForwardType a6,
      typename CallbackParamTraits<A7>::ForwardType a7) {
    return function_(CallbackForward(a1), CallbackForward(a2),
        CallbackForward(a3), CallbackForward(a4), CallbackForward(a5),
        CallbackForward(a6), CallbackForward(a7));
  }

 private:
  R (*function_)(A1, A2, A3, A4, A5, A6, A7);
};


template <typename R, typename T, typename A1, typename A2, typename A3,
    typename A4, typename A5, typename A6, typename A7>
class RunnableAdapter<R(T::*)(A1, A2, A3, A4, A5, A6, A7)> {
 public:
  typedef R (RunType)(T*, A1, A2, A3, A4, A5, A6, A7);
  typedef true_type IsMethod;

  explicit RunnableAdapter(R(T::*method)(A1, A2, A3, A4, A5, A6, A7))
      : method_(method) {
  }

  R Run(T* object, typename CallbackParamTraits<A1>::ForwardType a1,
      typename CallbackParamTraits<A2>::ForwardType a2,
      typename CallbackParamTraits<A3>::ForwardType a3,
      typename CallbackParamTraits<A4>::ForwardType a4,
      typename CallbackParamTraits<A5>::ForwardType a5,
      typename CallbackParamTraits<A6>::ForwardType a6,
      typename CallbackParamTraits<A7>::ForwardType a7) {
    return (object->*method_)(CallbackForward(a1), CallbackForward(a2),
        CallbackForward(a3), CallbackForward(a4), CallbackForward(a5),
        CallbackForward(a6), CallbackForward(a7));
  }

 private:
  R (T::*method_)(A1, A2, A3, A4, A5, A6, A7);
};


template <typename R, typename T, typename A1, typename A2, typename A3,
    typename A4, typename A5, typename A6, typename A7>
class RunnableAdapter<R(T::*)(A1, A2, A3, A4, A5, A6, A7) const> {
 public:
  typedef R (RunType)(const T*, A1, A2, A3, A4, A5, A6, A7);
  typedef true_type IsMethod;

  explicit RunnableAdapter(R(T::*method)(A1, A2, A3, A4, A5, A6, A7) const)
      : method_(method) {
  }

  R Run(const T* object, typename CallbackParamTraits<A1>::ForwardType a1,
      typename CallbackParamTraits<A2>::ForwardType a2,
      typename CallbackParamTraits<A3>::ForwardType a3,
      typename CallbackParamTraits<A4>::ForwardType a4,
      typename CallbackParamTraits<A5>::ForwardType a5,
      typename CallbackParamTraits<A6>::ForwardType a6,
      typename CallbackParamTraits<A7>::ForwardType a7) {
    return (object->*method_)(CallbackForward(a1), CallbackForward(a2),
        CallbackForward(a3), CallbackForward(a4), CallbackForward(a5),
        CallbackForward(a6), CallbackForward(a7));
  }

 private:
  R (T::*method_)(A1, A2, A3, A4, A5, A6, A7) const;
};





template <typename Sig>
struct FunctionTraits;

template <typename R>
struct FunctionTraits<R()> {
  typedef R ReturnType;
};

template <typename R, typename A1>
struct FunctionTraits<R(A1)> {
  typedef R ReturnType;
  typedef A1 A1Type;
};

template <typename R, typename A1, typename A2>
struct FunctionTraits<R(A1, A2)> {
  typedef R ReturnType;
  typedef A1 A1Type;
  typedef A2 A2Type;
};

template <typename R, typename A1, typename A2, typename A3>
struct FunctionTraits<R(A1, A2, A3)> {
  typedef R ReturnType;
  typedef A1 A1Type;
  typedef A2 A2Type;
  typedef A3 A3Type;
};

template <typename R, typename A1, typename A2, typename A3, typename A4>
struct FunctionTraits<R(A1, A2, A3, A4)> {
  typedef R ReturnType;
  typedef A1 A1Type;
  typedef A2 A2Type;
  typedef A3 A3Type;
  typedef A4 A4Type;
};

template <typename R, typename A1, typename A2, typename A3, typename A4,
    typename A5>
struct FunctionTraits<R(A1, A2, A3, A4, A5)> {
  typedef R ReturnType;
  typedef A1 A1Type;
  typedef A2 A2Type;
  typedef A3 A3Type;
  typedef A4 A4Type;
  typedef A5 A5Type;
};

template <typename R, typename A1, typename A2, typename A3, typename A4,
    typename A5, typename A6>
struct FunctionTraits<R(A1, A2, A3, A4, A5, A6)> {
  typedef R ReturnType;
  typedef A1 A1Type;
  typedef A2 A2Type;
  typedef A3 A3Type;
  typedef A4 A4Type;
  typedef A5 A5Type;
  typedef A6 A6Type;
};

template <typename R, typename A1, typename A2, typename A3, typename A4,
    typename A5, typename A6, typename A7>
struct FunctionTraits<R(A1, A2, A3, A4, A5, A6, A7)> {
  typedef R ReturnType;
  typedef A1 A1Type;
  typedef A2 A2Type;
  typedef A3 A3Type;
  typedef A4 A4Type;
  typedef A5 A5Type;
  typedef A6 A6Type;
  typedef A7 A7Type;
};





template <typename Sig>
struct ForceVoidReturn;

template <typename R>
struct ForceVoidReturn<R()> {
  typedef void(RunType)();
};

template <typename R, typename A1>
struct ForceVoidReturn<R(A1)> {
  typedef void(RunType)(A1);
};

template <typename R, typename A1, typename A2>
struct ForceVoidReturn<R(A1, A2)> {
  typedef void(RunType)(A1, A2);
};

template <typename R, typename A1, typename A2, typename A3>
struct ForceVoidReturn<R(A1, A2, A3)> {
  typedef void(RunType)(A1, A2, A3);
};

template <typename R, typename A1, typename A2, typename A3, typename A4>
struct ForceVoidReturn<R(A1, A2, A3, A4)> {
  typedef void(RunType)(A1, A2, A3, A4);
};

template <typename R, typename A1, typename A2, typename A3, typename A4,
    typename A5>
struct ForceVoidReturn<R(A1, A2, A3, A4, A5)> {
  typedef void(RunType)(A1, A2, A3, A4, A5);
};

template <typename R, typename A1, typename A2, typename A3, typename A4,
    typename A5, typename A6>
struct ForceVoidReturn<R(A1, A2, A3, A4, A5, A6)> {
  typedef void(RunType)(A1, A2, A3, A4, A5, A6);
};

template <typename R, typename A1, typename A2, typename A3, typename A4,
    typename A5, typename A6, typename A7>
struct ForceVoidReturn<R(A1, A2, A3, A4, A5, A6, A7)> {
  typedef void(RunType)(A1, A2, A3, A4, A5, A6, A7);
};





template <typename T>
struct FunctorTraits {
  typedef RunnableAdapter<T> RunnableType;
  typedef typename RunnableType::RunType RunType;
};

template <typename T>
struct FunctorTraits<IgnoreResultHelper<T> > {
  typedef typename FunctorTraits<T>::RunnableType RunnableType;
  typedef typename ForceVoidReturn<
      typename RunnableType::RunType>::RunType RunType;
};

template <typename T>
struct FunctorTraits<Callback<T> > {
  typedef Callback<T> RunnableType;
  typedef typename Callback<T>::RunType RunType;
};






template <typename T>
typename FunctorTraits<T>::RunnableType MakeRunnable(const T& t) {
  return RunnableAdapter<T>(t);
}

template <typename T>
typename FunctorTraits<T>::RunnableType
MakeRunnable(const IgnoreResultHelper<T>& t) {
  return MakeRunnable(t.functor_);
}

template <typename T>
const typename FunctorTraits<Callback<T> >::RunnableType&
MakeRunnable(const Callback<T>& t) {
  DCHECK(!t.is_null());
  return t;
}



















template <bool IsWeakCall, typename ReturnType, typename Runnable,
          typename ArgsType>
struct InvokeHelper;

template <typename ReturnType, typename Runnable>
struct InvokeHelper<false, ReturnType, Runnable,
    void()>  {
  static ReturnType MakeItSo(Runnable runnable) {
    return runnable.Run();
  }
};

template <typename Runnable>
struct InvokeHelper<false, void, Runnable,
    void()>  {
  static void MakeItSo(Runnable runnable) {
    runnable.Run();
  }
};

template <typename ReturnType, typename Runnable,typename A1>
struct InvokeHelper<false, ReturnType, Runnable,
    void(A1)>  {
  static ReturnType MakeItSo(Runnable runnable, A1 a1) {
    return runnable.Run(CallbackForward(a1));
  }
};

template <typename Runnable,typename A1>
struct InvokeHelper<false, void, Runnable,
    void(A1)>  {
  static void MakeItSo(Runnable runnable, A1 a1) {
    runnable.Run(CallbackForward(a1));
  }
};

template <typename Runnable, typename BoundWeakPtr>
struct InvokeHelper<true, void, Runnable,
    void(BoundWeakPtr)>  {
  static void MakeItSo(Runnable runnable, BoundWeakPtr weak_ptr) {
    if (!weak_ptr.get()) {
      return;
    }
    runnable.Run(weak_ptr.get());
  }
};

template <typename ReturnType, typename Runnable,typename A1, typename A2>
struct InvokeHelper<false, ReturnType, Runnable,
    void(A1, A2)>  {
  static ReturnType MakeItSo(Runnable runnable, A1 a1, A2 a2) {
    return runnable.Run(CallbackForward(a1), CallbackForward(a2));
  }
};

template <typename Runnable,typename A1, typename A2>
struct InvokeHelper<false, void, Runnable,
    void(A1, A2)>  {
  static void MakeItSo(Runnable runnable, A1 a1, A2 a2) {
    runnable.Run(CallbackForward(a1), CallbackForward(a2));
  }
};

template <typename Runnable, typename BoundWeakPtr, typename A2>
struct InvokeHelper<true, void, Runnable,
    void(BoundWeakPtr, A2)>  {
  static void MakeItSo(Runnable runnable, BoundWeakPtr weak_ptr, A2 a2) {
    if (!weak_ptr.get()) {
      return;
    }
    runnable.Run(weak_ptr.get(), CallbackForward(a2));
  }
};

template <typename ReturnType, typename Runnable,typename A1, typename A2,
    typename A3>
struct InvokeHelper<false, ReturnType, Runnable,
    void(A1, A2, A3)>  {
  static ReturnType MakeItSo(Runnable runnable, A1 a1, A2 a2, A3 a3) {
    return runnable.Run(CallbackForward(a1), CallbackForward(a2),
        CallbackForward(a3));
  }
};

template <typename Runnable,typename A1, typename A2, typename A3>
struct InvokeHelper<false, void, Runnable,
    void(A1, A2, A3)>  {
  static void MakeItSo(Runnable runnable, A1 a1, A2 a2, A3 a3) {
    runnable.Run(CallbackForward(a1), CallbackForward(a2), CallbackForward(a3));
  }
};

template <typename Runnable, typename BoundWeakPtr, typename A2, typename A3>
struct InvokeHelper<true, void, Runnable,
    void(BoundWeakPtr, A2, A3)>  {
  static void MakeItSo(Runnable runnable, BoundWeakPtr weak_ptr, A2 a2, A3 a3) {
    if (!weak_ptr.get()) {
      return;
    }
    runnable.Run(weak_ptr.get(), CallbackForward(a2), CallbackForward(a3));
  }
};

template <typename ReturnType, typename Runnable,typename A1, typename A2,
    typename A3, typename A4>
struct InvokeHelper<false, ReturnType, Runnable,
    void(A1, A2, A3, A4)>  {
  static ReturnType MakeItSo(Runnable runnable, A1 a1, A2 a2, A3 a3, A4 a4) {
    return runnable.Run(CallbackForward(a1), CallbackForward(a2),
        CallbackForward(a3), CallbackForward(a4));
  }
};

template <typename Runnable,typename A1, typename A2, typename A3, typename A4>
struct InvokeHelper<false, void, Runnable,
    void(A1, A2, A3, A4)>  {
  static void MakeItSo(Runnable runnable, A1 a1, A2 a2, A3 a3, A4 a4) {
    runnable.Run(CallbackForward(a1), CallbackForward(a2), CallbackForward(a3),
        CallbackForward(a4));
  }
};

template <typename Runnable, typename BoundWeakPtr, typename A2, typename A3,
    typename A4>
struct InvokeHelper<true, void, Runnable,
    void(BoundWeakPtr, A2, A3, A4)>  {
  static void MakeItSo(Runnable runnable, BoundWeakPtr weak_ptr, A2 a2, A3 a3,
      A4 a4) {
    if (!weak_ptr.get()) {
      return;
    }
    runnable.Run(weak_ptr.get(), CallbackForward(a2), CallbackForward(a3),
        CallbackForward(a4));
  }
};

template <typename ReturnType, typename Runnable,typename A1, typename A2,
    typename A3, typename A4, typename A5>
struct InvokeHelper<false, ReturnType, Runnable,
    void(A1, A2, A3, A4, A5)>  {
  static ReturnType MakeItSo(Runnable runnable, A1 a1, A2 a2, A3 a3, A4 a4,
      A5 a5) {
    return runnable.Run(CallbackForward(a1), CallbackForward(a2),
        CallbackForward(a3), CallbackForward(a4), CallbackForward(a5));
  }
};

template <typename Runnable,typename A1, typename A2, typename A3, typename A4,
    typename A5>
struct InvokeHelper<false, void, Runnable,
    void(A1, A2, A3, A4, A5)>  {
  static void MakeItSo(Runnable runnable, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5) {
    runnable.Run(CallbackForward(a1), CallbackForward(a2), CallbackForward(a3),
        CallbackForward(a4), CallbackForward(a5));
  }
};

template <typename Runnable, typename BoundWeakPtr, typename A2, typename A3,
    typename A4, typename A5>
struct InvokeHelper<true, void, Runnable,
    void(BoundWeakPtr, A2, A3, A4, A5)>  {
  static void MakeItSo(Runnable runnable, BoundWeakPtr weak_ptr, A2 a2, A3 a3,
      A4 a4, A5 a5) {
    if (!weak_ptr.get()) {
      return;
    }
    runnable.Run(weak_ptr.get(), CallbackForward(a2), CallbackForward(a3),
        CallbackForward(a4), CallbackForward(a5));
  }
};

template <typename ReturnType, typename Runnable,typename A1, typename A2,
    typename A3, typename A4, typename A5, typename A6>
struct InvokeHelper<false, ReturnType, Runnable,
    void(A1, A2, A3, A4, A5, A6)>  {
  static ReturnType MakeItSo(Runnable runnable, A1 a1, A2 a2, A3 a3, A4 a4,
      A5 a5, A6 a6) {
    return runnable.Run(CallbackForward(a1), CallbackForward(a2),
        CallbackForward(a3), CallbackForward(a4), CallbackForward(a5),
        CallbackForward(a6));
  }
};

template <typename Runnable,typename A1, typename A2, typename A3, typename A4,
    typename A5, typename A6>
struct InvokeHelper<false, void, Runnable,
    void(A1, A2, A3, A4, A5, A6)>  {
  static void MakeItSo(Runnable runnable, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5,
      A6 a6) {
    runnable.Run(CallbackForward(a1), CallbackForward(a2), CallbackForward(a3),
        CallbackForward(a4), CallbackForward(a5), CallbackForward(a6));
  }
};

template <typename Runnable, typename BoundWeakPtr, typename A2, typename A3,
    typename A4, typename A5, typename A6>
struct InvokeHelper<true, void, Runnable,
    void(BoundWeakPtr, A2, A3, A4, A5, A6)>  {
  static void MakeItSo(Runnable runnable, BoundWeakPtr weak_ptr, A2 a2, A3 a3,
      A4 a4, A5 a5, A6 a6) {
    if (!weak_ptr.get()) {
      return;
    }
    runnable.Run(weak_ptr.get(), CallbackForward(a2), CallbackForward(a3),
        CallbackForward(a4), CallbackForward(a5), CallbackForward(a6));
  }
};

template <typename ReturnType, typename Runnable,typename A1, typename A2,
    typename A3, typename A4, typename A5, typename A6, typename A7>
struct InvokeHelper<false, ReturnType, Runnable,
    void(A1, A2, A3, A4, A5, A6, A7)>  {
  static ReturnType MakeItSo(Runnable runnable, A1 a1, A2 a2, A3 a3, A4 a4,
      A5 a5, A6 a6, A7 a7) {
    return runnable.Run(CallbackForward(a1), CallbackForward(a2),
        CallbackForward(a3), CallbackForward(a4), CallbackForward(a5),
        CallbackForward(a6), CallbackForward(a7));
  }
};

template <typename Runnable,typename A1, typename A2, typename A3, typename A4,
    typename A5, typename A6, typename A7>
struct InvokeHelper<false, void, Runnable,
    void(A1, A2, A3, A4, A5, A6, A7)>  {
  static void MakeItSo(Runnable runnable, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5,
      A6 a6, A7 a7) {
    runnable.Run(CallbackForward(a1), CallbackForward(a2), CallbackForward(a3),
        CallbackForward(a4), CallbackForward(a5), CallbackForward(a6),
        CallbackForward(a7));
  }
};

template <typename Runnable, typename BoundWeakPtr, typename A2, typename A3,
    typename A4, typename A5, typename A6, typename A7>
struct InvokeHelper<true, void, Runnable,
    void(BoundWeakPtr, A2, A3, A4, A5, A6, A7)>  {
  static void MakeItSo(Runnable runnable, BoundWeakPtr weak_ptr, A2 a2, A3 a3,
      A4 a4, A5 a5, A6 a6, A7 a7) {
    if (!weak_ptr.get()) {
      return;
    }
    runnable.Run(weak_ptr.get(), CallbackForward(a2), CallbackForward(a3),
        CallbackForward(a4), CallbackForward(a5), CallbackForward(a6),
        CallbackForward(a7));
  }
};

#if !defined(_MSC_VER)

template <typename ReturnType, typename Runnable, typename ArgsType>
struct InvokeHelper<true, ReturnType, Runnable, ArgsType> {
  
  
  
  COMPILE_ASSERT(is_void<ReturnType>::value,
                 weak_ptrs_can_only_bind_to_methods_without_return_values);
};

#endif




template <int NumBound, typename Storage, typename RunType>
struct Invoker;


template <typename StorageType, typename R>
struct Invoker<0, StorageType, R()> {
  typedef R(RunType)(BindStateBase*);

  typedef R(UnboundRunType)();

  static R Run(BindStateBase* base) {
    StorageType* storage = static_cast<StorageType*>(base);

    
    
    

    return InvokeHelper<StorageType::IsWeakCall::value, R,
           typename StorageType::RunnableType,
           void()>
               ::MakeItSo(storage->runnable_);
  }
};


template <typename StorageType, typename R,typename X1>
struct Invoker<0, StorageType, R(X1)> {
  typedef R(RunType)(BindStateBase*,
      typename CallbackParamTraits<X1>::ForwardType);

  typedef R(UnboundRunType)(X1);

  static R Run(BindStateBase* base,
      typename CallbackParamTraits<X1>::ForwardType x1) {
    StorageType* storage = static_cast<StorageType*>(base);

    
    
    

    return InvokeHelper<StorageType::IsWeakCall::value, R,
           typename StorageType::RunnableType,
           void(typename CallbackParamTraits<X1>::ForwardType x1)>
               ::MakeItSo(storage->runnable_, CallbackForward(x1));
  }
};


template <typename StorageType, typename R,typename X1>
struct Invoker<1, StorageType, R(X1)> {
  typedef R(RunType)(BindStateBase*);

  typedef R(UnboundRunType)();

  static R Run(BindStateBase* base) {
    StorageType* storage = static_cast<StorageType*>(base);

    
    
    
    typedef typename StorageType::Bound1UnwrapTraits Bound1UnwrapTraits;

    typename Bound1UnwrapTraits::ForwardType x1 =
        Bound1UnwrapTraits::Unwrap(storage->p1_);
    return InvokeHelper<StorageType::IsWeakCall::value, R,
           typename StorageType::RunnableType,
           void(typename Bound1UnwrapTraits::ForwardType)>
               ::MakeItSo(storage->runnable_, CallbackForward(x1));
  }
};


template <typename StorageType, typename R,typename X1, typename X2>
struct Invoker<0, StorageType, R(X1, X2)> {
  typedef R(RunType)(BindStateBase*,
      typename CallbackParamTraits<X1>::ForwardType,
      typename CallbackParamTraits<X2>::ForwardType);

  typedef R(UnboundRunType)(X1, X2);

  static R Run(BindStateBase* base,
      typename CallbackParamTraits<X1>::ForwardType x1,
      typename CallbackParamTraits<X2>::ForwardType x2) {
    StorageType* storage = static_cast<StorageType*>(base);

    
    
    

    return InvokeHelper<StorageType::IsWeakCall::value, R,
           typename StorageType::RunnableType,
           void(typename CallbackParamTraits<X1>::ForwardType x1,
               typename CallbackParamTraits<X2>::ForwardType x2)>
               ::MakeItSo(storage->runnable_, CallbackForward(x1),
                   CallbackForward(x2));
  }
};


template <typename StorageType, typename R,typename X1, typename X2>
struct Invoker<1, StorageType, R(X1, X2)> {
  typedef R(RunType)(BindStateBase*,
      typename CallbackParamTraits<X2>::ForwardType);

  typedef R(UnboundRunType)(X2);

  static R Run(BindStateBase* base,
      typename CallbackParamTraits<X2>::ForwardType x2) {
    StorageType* storage = static_cast<StorageType*>(base);

    
    
    
    typedef typename StorageType::Bound1UnwrapTraits Bound1UnwrapTraits;

    typename Bound1UnwrapTraits::ForwardType x1 =
        Bound1UnwrapTraits::Unwrap(storage->p1_);
    return InvokeHelper<StorageType::IsWeakCall::value, R,
           typename StorageType::RunnableType,
           void(typename Bound1UnwrapTraits::ForwardType,
               typename CallbackParamTraits<X2>::ForwardType x2)>
               ::MakeItSo(storage->runnable_, CallbackForward(x1),
                   CallbackForward(x2));
  }
};


template <typename StorageType, typename R,typename X1, typename X2>
struct Invoker<2, StorageType, R(X1, X2)> {
  typedef R(RunType)(BindStateBase*);

  typedef R(UnboundRunType)();

  static R Run(BindStateBase* base) {
    StorageType* storage = static_cast<StorageType*>(base);

    
    
    
    typedef typename StorageType::Bound1UnwrapTraits Bound1UnwrapTraits;
    typedef typename StorageType::Bound2UnwrapTraits Bound2UnwrapTraits;

    typename Bound1UnwrapTraits::ForwardType x1 =
        Bound1UnwrapTraits::Unwrap(storage->p1_);
    typename Bound2UnwrapTraits::ForwardType x2 =
        Bound2UnwrapTraits::Unwrap(storage->p2_);
    return InvokeHelper<StorageType::IsWeakCall::value, R,
           typename StorageType::RunnableType,
           void(typename Bound1UnwrapTraits::ForwardType,
               typename Bound2UnwrapTraits::ForwardType)>
               ::MakeItSo(storage->runnable_, CallbackForward(x1),
                   CallbackForward(x2));
  }
};


template <typename StorageType, typename R,typename X1, typename X2,
    typename X3>
struct Invoker<0, StorageType, R(X1, X2, X3)> {
  typedef R(RunType)(BindStateBase*,
      typename CallbackParamTraits<X1>::ForwardType,
      typename CallbackParamTraits<X2>::ForwardType,
      typename CallbackParamTraits<X3>::ForwardType);

  typedef R(UnboundRunType)(X1, X2, X3);

  static R Run(BindStateBase* base,
      typename CallbackParamTraits<X1>::ForwardType x1,
      typename CallbackParamTraits<X2>::ForwardType x2,
      typename CallbackParamTraits<X3>::ForwardType x3) {
    StorageType* storage = static_cast<StorageType*>(base);

    
    
    

    return InvokeHelper<StorageType::IsWeakCall::value, R,
           typename StorageType::RunnableType,
           void(typename CallbackParamTraits<X1>::ForwardType x1,
               typename CallbackParamTraits<X2>::ForwardType x2,
               typename CallbackParamTraits<X3>::ForwardType x3)>
               ::MakeItSo(storage->runnable_, CallbackForward(x1),
                   CallbackForward(x2), CallbackForward(x3));
  }
};


template <typename StorageType, typename R,typename X1, typename X2,
    typename X3>
struct Invoker<1, StorageType, R(X1, X2, X3)> {
  typedef R(RunType)(BindStateBase*,
      typename CallbackParamTraits<X2>::ForwardType,
      typename CallbackParamTraits<X3>::ForwardType);

  typedef R(UnboundRunType)(X2, X3);

  static R Run(BindStateBase* base,
      typename CallbackParamTraits<X2>::ForwardType x2,
      typename CallbackParamTraits<X3>::ForwardType x3) {
    StorageType* storage = static_cast<StorageType*>(base);

    
    
    
    typedef typename StorageType::Bound1UnwrapTraits Bound1UnwrapTraits;

    typename Bound1UnwrapTraits::ForwardType x1 =
        Bound1UnwrapTraits::Unwrap(storage->p1_);
    return InvokeHelper<StorageType::IsWeakCall::value, R,
           typename StorageType::RunnableType,
           void(typename Bound1UnwrapTraits::ForwardType,
               typename CallbackParamTraits<X2>::ForwardType x2,
               typename CallbackParamTraits<X3>::ForwardType x3)>
               ::MakeItSo(storage->runnable_, CallbackForward(x1),
                   CallbackForward(x2), CallbackForward(x3));
  }
};


template <typename StorageType, typename R,typename X1, typename X2,
    typename X3>
struct Invoker<2, StorageType, R(X1, X2, X3)> {
  typedef R(RunType)(BindStateBase*,
      typename CallbackParamTraits<X3>::ForwardType);

  typedef R(UnboundRunType)(X3);

  static R Run(BindStateBase* base,
      typename CallbackParamTraits<X3>::ForwardType x3) {
    StorageType* storage = static_cast<StorageType*>(base);

    
    
    
    typedef typename StorageType::Bound1UnwrapTraits Bound1UnwrapTraits;
    typedef typename StorageType::Bound2UnwrapTraits Bound2UnwrapTraits;

    typename Bound1UnwrapTraits::ForwardType x1 =
        Bound1UnwrapTraits::Unwrap(storage->p1_);
    typename Bound2UnwrapTraits::ForwardType x2 =
        Bound2UnwrapTraits::Unwrap(storage->p2_);
    return InvokeHelper<StorageType::IsWeakCall::value, R,
           typename StorageType::RunnableType,
           void(typename Bound1UnwrapTraits::ForwardType,
               typename Bound2UnwrapTraits::ForwardType,
               typename CallbackParamTraits<X3>::ForwardType x3)>
               ::MakeItSo(storage->runnable_, CallbackForward(x1),
                   CallbackForward(x2), CallbackForward(x3));
  }
};


template <typename StorageType, typename R,typename X1, typename X2,
    typename X3>
struct Invoker<3, StorageType, R(X1, X2, X3)> {
  typedef R(RunType)(BindStateBase*);

  typedef R(UnboundRunType)();

  static R Run(BindStateBase* base) {
    StorageType* storage = static_cast<StorageType*>(base);

    
    
    
    typedef typename StorageType::Bound1UnwrapTraits Bound1UnwrapTraits;
    typedef typename StorageType::Bound2UnwrapTraits Bound2UnwrapTraits;
    typedef typename StorageType::Bound3UnwrapTraits Bound3UnwrapTraits;

    typename Bound1UnwrapTraits::ForwardType x1 =
        Bound1UnwrapTraits::Unwrap(storage->p1_);
    typename Bound2UnwrapTraits::ForwardType x2 =
        Bound2UnwrapTraits::Unwrap(storage->p2_);
    typename Bound3UnwrapTraits::ForwardType x3 =
        Bound3UnwrapTraits::Unwrap(storage->p3_);
    return InvokeHelper<StorageType::IsWeakCall::value, R,
           typename StorageType::RunnableType,
           void(typename Bound1UnwrapTraits::ForwardType,
               typename Bound2UnwrapTraits::ForwardType,
               typename Bound3UnwrapTraits::ForwardType)>
               ::MakeItSo(storage->runnable_, CallbackForward(x1),
                   CallbackForward(x2), CallbackForward(x3));
  }
};


template <typename StorageType, typename R,typename X1, typename X2,
    typename X3, typename X4>
struct Invoker<0, StorageType, R(X1, X2, X3, X4)> {
  typedef R(RunType)(BindStateBase*,
      typename CallbackParamTraits<X1>::ForwardType,
      typename CallbackParamTraits<X2>::ForwardType,
      typename CallbackParamTraits<X3>::ForwardType,
      typename CallbackParamTraits<X4>::ForwardType);

  typedef R(UnboundRunType)(X1, X2, X3, X4);

  static R Run(BindStateBase* base,
      typename CallbackParamTraits<X1>::ForwardType x1,
      typename CallbackParamTraits<X2>::ForwardType x2,
      typename CallbackParamTraits<X3>::ForwardType x3,
      typename CallbackParamTraits<X4>::ForwardType x4) {
    StorageType* storage = static_cast<StorageType*>(base);

    
    
    

    return InvokeHelper<StorageType::IsWeakCall::value, R,
           typename StorageType::RunnableType,
           void(typename CallbackParamTraits<X1>::ForwardType x1,
               typename CallbackParamTraits<X2>::ForwardType x2,
               typename CallbackParamTraits<X3>::ForwardType x3,
               typename CallbackParamTraits<X4>::ForwardType x4)>
               ::MakeItSo(storage->runnable_, CallbackForward(x1),
                   CallbackForward(x2), CallbackForward(x3),
                   CallbackForward(x4));
  }
};


template <typename StorageType, typename R,typename X1, typename X2,
    typename X3, typename X4>
struct Invoker<1, StorageType, R(X1, X2, X3, X4)> {
  typedef R(RunType)(BindStateBase*,
      typename CallbackParamTraits<X2>::ForwardType,
      typename CallbackParamTraits<X3>::ForwardType,
      typename CallbackParamTraits<X4>::ForwardType);

  typedef R(UnboundRunType)(X2, X3, X4);

  static R Run(BindStateBase* base,
      typename CallbackParamTraits<X2>::ForwardType x2,
      typename CallbackParamTraits<X3>::ForwardType x3,
      typename CallbackParamTraits<X4>::ForwardType x4) {
    StorageType* storage = static_cast<StorageType*>(base);

    
    
    
    typedef typename StorageType::Bound1UnwrapTraits Bound1UnwrapTraits;

    typename Bound1UnwrapTraits::ForwardType x1 =
        Bound1UnwrapTraits::Unwrap(storage->p1_);
    return InvokeHelper<StorageType::IsWeakCall::value, R,
           typename StorageType::RunnableType,
           void(typename Bound1UnwrapTraits::ForwardType,
               typename CallbackParamTraits<X2>::ForwardType x2,
               typename CallbackParamTraits<X3>::ForwardType x3,
               typename CallbackParamTraits<X4>::ForwardType x4)>
               ::MakeItSo(storage->runnable_, CallbackForward(x1),
                   CallbackForward(x2), CallbackForward(x3),
                   CallbackForward(x4));
  }
};


template <typename StorageType, typename R,typename X1, typename X2,
    typename X3, typename X4>
struct Invoker<2, StorageType, R(X1, X2, X3, X4)> {
  typedef R(RunType)(BindStateBase*,
      typename CallbackParamTraits<X3>::ForwardType,
      typename CallbackParamTraits<X4>::ForwardType);

  typedef R(UnboundRunType)(X3, X4);

  static R Run(BindStateBase* base,
      typename CallbackParamTraits<X3>::ForwardType x3,
      typename CallbackParamTraits<X4>::ForwardType x4) {
    StorageType* storage = static_cast<StorageType*>(base);

    
    
    
    typedef typename StorageType::Bound1UnwrapTraits Bound1UnwrapTraits;
    typedef typename StorageType::Bound2UnwrapTraits Bound2UnwrapTraits;

    typename Bound1UnwrapTraits::ForwardType x1 =
        Bound1UnwrapTraits::Unwrap(storage->p1_);
    typename Bound2UnwrapTraits::ForwardType x2 =
        Bound2UnwrapTraits::Unwrap(storage->p2_);
    return InvokeHelper<StorageType::IsWeakCall::value, R,
           typename StorageType::RunnableType,
           void(typename Bound1UnwrapTraits::ForwardType,
               typename Bound2UnwrapTraits::ForwardType,
               typename CallbackParamTraits<X3>::ForwardType x3,
               typename CallbackParamTraits<X4>::ForwardType x4)>
               ::MakeItSo(storage->runnable_, CallbackForward(x1),
                   CallbackForward(x2), CallbackForward(x3),
                   CallbackForward(x4));
  }
};


template <typename StorageType, typename R,typename X1, typename X2,
    typename X3, typename X4>
struct Invoker<3, StorageType, R(X1, X2, X3, X4)> {
  typedef R(RunType)(BindStateBase*,
      typename CallbackParamTraits<X4>::ForwardType);

  typedef R(UnboundRunType)(X4);

  static R Run(BindStateBase* base,
      typename CallbackParamTraits<X4>::ForwardType x4) {
    StorageType* storage = static_cast<StorageType*>(base);

    
    
    
    typedef typename StorageType::Bound1UnwrapTraits Bound1UnwrapTraits;
    typedef typename StorageType::Bound2UnwrapTraits Bound2UnwrapTraits;
    typedef typename StorageType::Bound3UnwrapTraits Bound3UnwrapTraits;

    typename Bound1UnwrapTraits::ForwardType x1 =
        Bound1UnwrapTraits::Unwrap(storage->p1_);
    typename Bound2UnwrapTraits::ForwardType x2 =
        Bound2UnwrapTraits::Unwrap(storage->p2_);
    typename Bound3UnwrapTraits::ForwardType x3 =
        Bound3UnwrapTraits::Unwrap(storage->p3_);
    return InvokeHelper<StorageType::IsWeakCall::value, R,
           typename StorageType::RunnableType,
           void(typename Bound1UnwrapTraits::ForwardType,
               typename Bound2UnwrapTraits::ForwardType,
               typename Bound3UnwrapTraits::ForwardType,
               typename CallbackParamTraits<X4>::ForwardType x4)>
               ::MakeItSo(storage->runnable_, CallbackForward(x1),
                   CallbackForward(x2), CallbackForward(x3),
                   CallbackForward(x4));
  }
};


template <typename StorageType, typename R,typename X1, typename X2,
    typename X3, typename X4>
struct Invoker<4, StorageType, R(X1, X2, X3, X4)> {
  typedef R(RunType)(BindStateBase*);

  typedef R(UnboundRunType)();

  static R Run(BindStateBase* base) {
    StorageType* storage = static_cast<StorageType*>(base);

    
    
    
    typedef typename StorageType::Bound1UnwrapTraits Bound1UnwrapTraits;
    typedef typename StorageType::Bound2UnwrapTraits Bound2UnwrapTraits;
    typedef typename StorageType::Bound3UnwrapTraits Bound3UnwrapTraits;
    typedef typename StorageType::Bound4UnwrapTraits Bound4UnwrapTraits;

    typename Bound1UnwrapTraits::ForwardType x1 =
        Bound1UnwrapTraits::Unwrap(storage->p1_);
    typename Bound2UnwrapTraits::ForwardType x2 =
        Bound2UnwrapTraits::Unwrap(storage->p2_);
    typename Bound3UnwrapTraits::ForwardType x3 =
        Bound3UnwrapTraits::Unwrap(storage->p3_);
    typename Bound4UnwrapTraits::ForwardType x4 =
        Bound4UnwrapTraits::Unwrap(storage->p4_);
    return InvokeHelper<StorageType::IsWeakCall::value, R,
           typename StorageType::RunnableType,
           void(typename Bound1UnwrapTraits::ForwardType,
               typename Bound2UnwrapTraits::ForwardType,
               typename Bound3UnwrapTraits::ForwardType,
               typename Bound4UnwrapTraits::ForwardType)>
               ::MakeItSo(storage->runnable_, CallbackForward(x1),
                   CallbackForward(x2), CallbackForward(x3),
                   CallbackForward(x4));
  }
};


template <typename StorageType, typename R,typename X1, typename X2,
    typename X3, typename X4, typename X5>
struct Invoker<0, StorageType, R(X1, X2, X3, X4, X5)> {
  typedef R(RunType)(BindStateBase*,
      typename CallbackParamTraits<X1>::ForwardType,
      typename CallbackParamTraits<X2>::ForwardType,
      typename CallbackParamTraits<X3>::ForwardType,
      typename CallbackParamTraits<X4>::ForwardType,
      typename CallbackParamTraits<X5>::ForwardType);

  typedef R(UnboundRunType)(X1, X2, X3, X4, X5);

  static R Run(BindStateBase* base,
      typename CallbackParamTraits<X1>::ForwardType x1,
      typename CallbackParamTraits<X2>::ForwardType x2,
      typename CallbackParamTraits<X3>::ForwardType x3,
      typename CallbackParamTraits<X4>::ForwardType x4,
      typename CallbackParamTraits<X5>::ForwardType x5) {
    StorageType* storage = static_cast<StorageType*>(base);

    
    
    

    return InvokeHelper<StorageType::IsWeakCall::value, R,
           typename StorageType::RunnableType,
           void(typename CallbackParamTraits<X1>::ForwardType x1,
               typename CallbackParamTraits<X2>::ForwardType x2,
               typename CallbackParamTraits<X3>::ForwardType x3,
               typename CallbackParamTraits<X4>::ForwardType x4,
               typename CallbackParamTraits<X5>::ForwardType x5)>
               ::MakeItSo(storage->runnable_, CallbackForward(x1),
                   CallbackForward(x2), CallbackForward(x3),
                   CallbackForward(x4), CallbackForward(x5));
  }
};


template <typename StorageType, typename R,typename X1, typename X2,
    typename X3, typename X4, typename X5>
struct Invoker<1, StorageType, R(X1, X2, X3, X4, X5)> {
  typedef R(RunType)(BindStateBase*,
      typename CallbackParamTraits<X2>::ForwardType,
      typename CallbackParamTraits<X3>::ForwardType,
      typename CallbackParamTraits<X4>::ForwardType,
      typename CallbackParamTraits<X5>::ForwardType);

  typedef R(UnboundRunType)(X2, X3, X4, X5);

  static R Run(BindStateBase* base,
      typename CallbackParamTraits<X2>::ForwardType x2,
      typename CallbackParamTraits<X3>::ForwardType x3,
      typename CallbackParamTraits<X4>::ForwardType x4,
      typename CallbackParamTraits<X5>::ForwardType x5) {
    StorageType* storage = static_cast<StorageType*>(base);

    
    
    
    typedef typename StorageType::Bound1UnwrapTraits Bound1UnwrapTraits;

    typename Bound1UnwrapTraits::ForwardType x1 =
        Bound1UnwrapTraits::Unwrap(storage->p1_);
    return InvokeHelper<StorageType::IsWeakCall::value, R,
           typename StorageType::RunnableType,
           void(typename Bound1UnwrapTraits::ForwardType,
               typename CallbackParamTraits<X2>::ForwardType x2,
               typename CallbackParamTraits<X3>::ForwardType x3,
               typename CallbackParamTraits<X4>::ForwardType x4,
               typename CallbackParamTraits<X5>::ForwardType x5)>
               ::MakeItSo(storage->runnable_, CallbackForward(x1),
                   CallbackForward(x2), CallbackForward(x3),
                   CallbackForward(x4), CallbackForward(x5));
  }
};


template <typename StorageType, typename R,typename X1, typename X2,
    typename X3, typename X4, typename X5>
struct Invoker<2, StorageType, R(X1, X2, X3, X4, X5)> {
  typedef R(RunType)(BindStateBase*,
      typename CallbackParamTraits<X3>::ForwardType,
      typename CallbackParamTraits<X4>::ForwardType,
      typename CallbackParamTraits<X5>::ForwardType);

  typedef R(UnboundRunType)(X3, X4, X5);

  static R Run(BindStateBase* base,
      typename CallbackParamTraits<X3>::ForwardType x3,
      typename CallbackParamTraits<X4>::ForwardType x4,
      typename CallbackParamTraits<X5>::ForwardType x5) {
    StorageType* storage = static_cast<StorageType*>(base);

    
    
    
    typedef typename StorageType::Bound1UnwrapTraits Bound1UnwrapTraits;
    typedef typename StorageType::Bound2UnwrapTraits Bound2UnwrapTraits;

    typename Bound1UnwrapTraits::ForwardType x1 =
        Bound1UnwrapTraits::Unwrap(storage->p1_);
    typename Bound2UnwrapTraits::ForwardType x2 =
        Bound2UnwrapTraits::Unwrap(storage->p2_);
    return InvokeHelper<StorageType::IsWeakCall::value, R,
           typename StorageType::RunnableType,
           void(typename Bound1UnwrapTraits::ForwardType,
               typename Bound2UnwrapTraits::ForwardType,
               typename CallbackParamTraits<X3>::ForwardType x3,
               typename CallbackParamTraits<X4>::ForwardType x4,
               typename CallbackParamTraits<X5>::ForwardType x5)>
               ::MakeItSo(storage->runnable_, CallbackForward(x1),
                   CallbackForward(x2), CallbackForward(x3),
                   CallbackForward(x4), CallbackForward(x5));
  }
};


template <typename StorageType, typename R,typename X1, typename X2,
    typename X3, typename X4, typename X5>
struct Invoker<3, StorageType, R(X1, X2, X3, X4, X5)> {
  typedef R(RunType)(BindStateBase*,
      typename CallbackParamTraits<X4>::ForwardType,
      typename CallbackParamTraits<X5>::ForwardType);

  typedef R(UnboundRunType)(X4, X5);

  static R Run(BindStateBase* base,
      typename CallbackParamTraits<X4>::ForwardType x4,
      typename CallbackParamTraits<X5>::ForwardType x5) {
    StorageType* storage = static_cast<StorageType*>(base);

    
    
    
    typedef typename StorageType::Bound1UnwrapTraits Bound1UnwrapTraits;
    typedef typename StorageType::Bound2UnwrapTraits Bound2UnwrapTraits;
    typedef typename StorageType::Bound3UnwrapTraits Bound3UnwrapTraits;

    typename Bound1UnwrapTraits::ForwardType x1 =
        Bound1UnwrapTraits::Unwrap(storage->p1_);
    typename Bound2UnwrapTraits::ForwardType x2 =
        Bound2UnwrapTraits::Unwrap(storage->p2_);
    typename Bound3UnwrapTraits::ForwardType x3 =
        Bound3UnwrapTraits::Unwrap(storage->p3_);
    return InvokeHelper<StorageType::IsWeakCall::value, R,
           typename StorageType::RunnableType,
           void(typename Bound1UnwrapTraits::ForwardType,
               typename Bound2UnwrapTraits::ForwardType,
               typename Bound3UnwrapTraits::ForwardType,
               typename CallbackParamTraits<X4>::ForwardType x4,
               typename CallbackParamTraits<X5>::ForwardType x5)>
               ::MakeItSo(storage->runnable_, CallbackForward(x1),
                   CallbackForward(x2), CallbackForward(x3),
                   CallbackForward(x4), CallbackForward(x5));
  }
};


template <typename StorageType, typename R,typename X1, typename X2,
    typename X3, typename X4, typename X5>
struct Invoker<4, StorageType, R(X1, X2, X3, X4, X5)> {
  typedef R(RunType)(BindStateBase*,
      typename CallbackParamTraits<X5>::ForwardType);

  typedef R(UnboundRunType)(X5);

  static R Run(BindStateBase* base,
      typename CallbackParamTraits<X5>::ForwardType x5) {
    StorageType* storage = static_cast<StorageType*>(base);

    
    
    
    typedef typename StorageType::Bound1UnwrapTraits Bound1UnwrapTraits;
    typedef typename StorageType::Bound2UnwrapTraits Bound2UnwrapTraits;
    typedef typename StorageType::Bound3UnwrapTraits Bound3UnwrapTraits;
    typedef typename StorageType::Bound4UnwrapTraits Bound4UnwrapTraits;

    typename Bound1UnwrapTraits::ForwardType x1 =
        Bound1UnwrapTraits::Unwrap(storage->p1_);
    typename Bound2UnwrapTraits::ForwardType x2 =
        Bound2UnwrapTraits::Unwrap(storage->p2_);
    typename Bound3UnwrapTraits::ForwardType x3 =
        Bound3UnwrapTraits::Unwrap(storage->p3_);
    typename Bound4UnwrapTraits::ForwardType x4 =
        Bound4UnwrapTraits::Unwrap(storage->p4_);
    return InvokeHelper<StorageType::IsWeakCall::value, R,
           typename StorageType::RunnableType,
           void(typename Bound1UnwrapTraits::ForwardType,
               typename Bound2UnwrapTraits::ForwardType,
               typename Bound3UnwrapTraits::ForwardType,
               typename Bound4UnwrapTraits::ForwardType,
               typename CallbackParamTraits<X5>::ForwardType x5)>
               ::MakeItSo(storage->runnable_, CallbackForward(x1),
                   CallbackForward(x2), CallbackForward(x3),
                   CallbackForward(x4), CallbackForward(x5));
  }
};


template <typename StorageType, typename R,typename X1, typename X2,
    typename X3, typename X4, typename X5>
struct Invoker<5, StorageType, R(X1, X2, X3, X4, X5)> {
  typedef R(RunType)(BindStateBase*);

  typedef R(UnboundRunType)();

  static R Run(BindStateBase* base) {
    StorageType* storage = static_cast<StorageType*>(base);

    
    
    
    typedef typename StorageType::Bound1UnwrapTraits Bound1UnwrapTraits;
    typedef typename StorageType::Bound2UnwrapTraits Bound2UnwrapTraits;
    typedef typename StorageType::Bound3UnwrapTraits Bound3UnwrapTraits;
    typedef typename StorageType::Bound4UnwrapTraits Bound4UnwrapTraits;
    typedef typename StorageType::Bound5UnwrapTraits Bound5UnwrapTraits;

    typename Bound1UnwrapTraits::ForwardType x1 =
        Bound1UnwrapTraits::Unwrap(storage->p1_);
    typename Bound2UnwrapTraits::ForwardType x2 =
        Bound2UnwrapTraits::Unwrap(storage->p2_);
    typename Bound3UnwrapTraits::ForwardType x3 =
        Bound3UnwrapTraits::Unwrap(storage->p3_);
    typename Bound4UnwrapTraits::ForwardType x4 =
        Bound4UnwrapTraits::Unwrap(storage->p4_);
    typename Bound5UnwrapTraits::ForwardType x5 =
        Bound5UnwrapTraits::Unwrap(storage->p5_);
    return InvokeHelper<StorageType::IsWeakCall::value, R,
           typename StorageType::RunnableType,
           void(typename Bound1UnwrapTraits::ForwardType,
               typename Bound2UnwrapTraits::ForwardType,
               typename Bound3UnwrapTraits::ForwardType,
               typename Bound4UnwrapTraits::ForwardType,
               typename Bound5UnwrapTraits::ForwardType)>
               ::MakeItSo(storage->runnable_, CallbackForward(x1),
                   CallbackForward(x2), CallbackForward(x3),
                   CallbackForward(x4), CallbackForward(x5));
  }
};


template <typename StorageType, typename R,typename X1, typename X2,
    typename X3, typename X4, typename X5, typename X6>
struct Invoker<0, StorageType, R(X1, X2, X3, X4, X5, X6)> {
  typedef R(RunType)(BindStateBase*,
      typename CallbackParamTraits<X1>::ForwardType,
      typename CallbackParamTraits<X2>::ForwardType,
      typename CallbackParamTraits<X3>::ForwardType,
      typename CallbackParamTraits<X4>::ForwardType,
      typename CallbackParamTraits<X5>::ForwardType,
      typename CallbackParamTraits<X6>::ForwardType);

  typedef R(UnboundRunType)(X1, X2, X3, X4, X5, X6);

  static R Run(BindStateBase* base,
      typename CallbackParamTraits<X1>::ForwardType x1,
      typename CallbackParamTraits<X2>::ForwardType x2,
      typename CallbackParamTraits<X3>::ForwardType x3,
      typename CallbackParamTraits<X4>::ForwardType x4,
      typename CallbackParamTraits<X5>::ForwardType x5,
      typename CallbackParamTraits<X6>::ForwardType x6) {
    StorageType* storage = static_cast<StorageType*>(base);

    
    
    

    return InvokeHelper<StorageType::IsWeakCall::value, R,
           typename StorageType::RunnableType,
           void(typename CallbackParamTraits<X1>::ForwardType x1,
               typename CallbackParamTraits<X2>::ForwardType x2,
               typename CallbackParamTraits<X3>::ForwardType x3,
               typename CallbackParamTraits<X4>::ForwardType x4,
               typename CallbackParamTraits<X5>::ForwardType x5,
               typename CallbackParamTraits<X6>::ForwardType x6)>
               ::MakeItSo(storage->runnable_, CallbackForward(x1),
                   CallbackForward(x2), CallbackForward(x3),
                   CallbackForward(x4), CallbackForward(x5),
                   CallbackForward(x6));
  }
};


template <typename StorageType, typename R,typename X1, typename X2,
    typename X3, typename X4, typename X5, typename X6>
struct Invoker<1, StorageType, R(X1, X2, X3, X4, X5, X6)> {
  typedef R(RunType)(BindStateBase*,
      typename CallbackParamTraits<X2>::ForwardType,
      typename CallbackParamTraits<X3>::ForwardType,
      typename CallbackParamTraits<X4>::ForwardType,
      typename CallbackParamTraits<X5>::ForwardType,
      typename CallbackParamTraits<X6>::ForwardType);

  typedef R(UnboundRunType)(X2, X3, X4, X5, X6);

  static R Run(BindStateBase* base,
      typename CallbackParamTraits<X2>::ForwardType x2,
      typename CallbackParamTraits<X3>::ForwardType x3,
      typename CallbackParamTraits<X4>::ForwardType x4,
      typename CallbackParamTraits<X5>::ForwardType x5,
      typename CallbackParamTraits<X6>::ForwardType x6) {
    StorageType* storage = static_cast<StorageType*>(base);

    
    
    
    typedef typename StorageType::Bound1UnwrapTraits Bound1UnwrapTraits;

    typename Bound1UnwrapTraits::ForwardType x1 =
        Bound1UnwrapTraits::Unwrap(storage->p1_);
    return InvokeHelper<StorageType::IsWeakCall::value, R,
           typename StorageType::RunnableType,
           void(typename Bound1UnwrapTraits::ForwardType,
               typename CallbackParamTraits<X2>::ForwardType x2,
               typename CallbackParamTraits<X3>::ForwardType x3,
               typename CallbackParamTraits<X4>::ForwardType x4,
               typename CallbackParamTraits<X5>::ForwardType x5,
               typename CallbackParamTraits<X6>::ForwardType x6)>
               ::MakeItSo(storage->runnable_, CallbackForward(x1),
                   CallbackForward(x2), CallbackForward(x3),
                   CallbackForward(x4), CallbackForward(x5),
                   CallbackForward(x6));
  }
};


template <typename StorageType, typename R,typename X1, typename X2,
    typename X3, typename X4, typename X5, typename X6>
struct Invoker<2, StorageType, R(X1, X2, X3, X4, X5, X6)> {
  typedef R(RunType)(BindStateBase*,
      typename CallbackParamTraits<X3>::ForwardType,
      typename CallbackParamTraits<X4>::ForwardType,
      typename CallbackParamTraits<X5>::ForwardType,
      typename CallbackParamTraits<X6>::ForwardType);

  typedef R(UnboundRunType)(X3, X4, X5, X6);

  static R Run(BindStateBase* base,
      typename CallbackParamTraits<X3>::ForwardType x3,
      typename CallbackParamTraits<X4>::ForwardType x4,
      typename CallbackParamTraits<X5>::ForwardType x5,
      typename CallbackParamTraits<X6>::ForwardType x6) {
    StorageType* storage = static_cast<StorageType*>(base);

    
    
    
    typedef typename StorageType::Bound1UnwrapTraits Bound1UnwrapTraits;
    typedef typename StorageType::Bound2UnwrapTraits Bound2UnwrapTraits;

    typename Bound1UnwrapTraits::ForwardType x1 =
        Bound1UnwrapTraits::Unwrap(storage->p1_);
    typename Bound2UnwrapTraits::ForwardType x2 =
        Bound2UnwrapTraits::Unwrap(storage->p2_);
    return InvokeHelper<StorageType::IsWeakCall::value, R,
           typename StorageType::RunnableType,
           void(typename Bound1UnwrapTraits::ForwardType,
               typename Bound2UnwrapTraits::ForwardType,
               typename CallbackParamTraits<X3>::ForwardType x3,
               typename CallbackParamTraits<X4>::ForwardType x4,
               typename CallbackParamTraits<X5>::ForwardType x5,
               typename CallbackParamTraits<X6>::ForwardType x6)>
               ::MakeItSo(storage->runnable_, CallbackForward(x1),
                   CallbackForward(x2), CallbackForward(x3),
                   CallbackForward(x4), CallbackForward(x5),
                   CallbackForward(x6));
  }
};


template <typename StorageType, typename R,typename X1, typename X2,
    typename X3, typename X4, typename X5, typename X6>
struct Invoker<3, StorageType, R(X1, X2, X3, X4, X5, X6)> {
  typedef R(RunType)(BindStateBase*,
      typename CallbackParamTraits<X4>::ForwardType,
      typename CallbackParamTraits<X5>::ForwardType,
      typename CallbackParamTraits<X6>::ForwardType);

  typedef R(UnboundRunType)(X4, X5, X6);

  static R Run(BindStateBase* base,
      typename CallbackParamTraits<X4>::ForwardType x4,
      typename CallbackParamTraits<X5>::ForwardType x5,
      typename CallbackParamTraits<X6>::ForwardType x6) {
    StorageType* storage = static_cast<StorageType*>(base);

    
    
    
    typedef typename StorageType::Bound1UnwrapTraits Bound1UnwrapTraits;
    typedef typename StorageType::Bound2UnwrapTraits Bound2UnwrapTraits;
    typedef typename StorageType::Bound3UnwrapTraits Bound3UnwrapTraits;

    typename Bound1UnwrapTraits::ForwardType x1 =
        Bound1UnwrapTraits::Unwrap(storage->p1_);
    typename Bound2UnwrapTraits::ForwardType x2 =
        Bound2UnwrapTraits::Unwrap(storage->p2_);
    typename Bound3UnwrapTraits::ForwardType x3 =
        Bound3UnwrapTraits::Unwrap(storage->p3_);
    return InvokeHelper<StorageType::IsWeakCall::value, R,
           typename StorageType::RunnableType,
           void(typename Bound1UnwrapTraits::ForwardType,
               typename Bound2UnwrapTraits::ForwardType,
               typename Bound3UnwrapTraits::ForwardType,
               typename CallbackParamTraits<X4>::ForwardType x4,
               typename CallbackParamTraits<X5>::ForwardType x5,
               typename CallbackParamTraits<X6>::ForwardType x6)>
               ::MakeItSo(storage->runnable_, CallbackForward(x1),
                   CallbackForward(x2), CallbackForward(x3),
                   CallbackForward(x4), CallbackForward(x5),
                   CallbackForward(x6));
  }
};


template <typename StorageType, typename R,typename X1, typename X2,
    typename X3, typename X4, typename X5, typename X6>
struct Invoker<4, StorageType, R(X1, X2, X3, X4, X5, X6)> {
  typedef R(RunType)(BindStateBase*,
      typename CallbackParamTraits<X5>::ForwardType,
      typename CallbackParamTraits<X6>::ForwardType);

  typedef R(UnboundRunType)(X5, X6);

  static R Run(BindStateBase* base,
      typename CallbackParamTraits<X5>::ForwardType x5,
      typename CallbackParamTraits<X6>::ForwardType x6) {
    StorageType* storage = static_cast<StorageType*>(base);

    
    
    
    typedef typename StorageType::Bound1UnwrapTraits Bound1UnwrapTraits;
    typedef typename StorageType::Bound2UnwrapTraits Bound2UnwrapTraits;
    typedef typename StorageType::Bound3UnwrapTraits Bound3UnwrapTraits;
    typedef typename StorageType::Bound4UnwrapTraits Bound4UnwrapTraits;

    typename Bound1UnwrapTraits::ForwardType x1 =
        Bound1UnwrapTraits::Unwrap(storage->p1_);
    typename Bound2UnwrapTraits::ForwardType x2 =
        Bound2UnwrapTraits::Unwrap(storage->p2_);
    typename Bound3UnwrapTraits::ForwardType x3 =
        Bound3UnwrapTraits::Unwrap(storage->p3_);
    typename Bound4UnwrapTraits::ForwardType x4 =
        Bound4UnwrapTraits::Unwrap(storage->p4_);
    return InvokeHelper<StorageType::IsWeakCall::value, R,
           typename StorageType::RunnableType,
           void(typename Bound1UnwrapTraits::ForwardType,
               typename Bound2UnwrapTraits::ForwardType,
               typename Bound3UnwrapTraits::ForwardType,
               typename Bound4UnwrapTraits::ForwardType,
               typename CallbackParamTraits<X5>::ForwardType x5,
               typename CallbackParamTraits<X6>::ForwardType x6)>
               ::MakeItSo(storage->runnable_, CallbackForward(x1),
                   CallbackForward(x2), CallbackForward(x3),
                   CallbackForward(x4), CallbackForward(x5),
                   CallbackForward(x6));
  }
};


template <typename StorageType, typename R,typename X1, typename X2,
    typename X3, typename X4, typename X5, typename X6>
struct Invoker<5, StorageType, R(X1, X2, X3, X4, X5, X6)> {
  typedef R(RunType)(BindStateBase*,
      typename CallbackParamTraits<X6>::ForwardType);

  typedef R(UnboundRunType)(X6);

  static R Run(BindStateBase* base,
      typename CallbackParamTraits<X6>::ForwardType x6) {
    StorageType* storage = static_cast<StorageType*>(base);

    
    
    
    typedef typename StorageType::Bound1UnwrapTraits Bound1UnwrapTraits;
    typedef typename StorageType::Bound2UnwrapTraits Bound2UnwrapTraits;
    typedef typename StorageType::Bound3UnwrapTraits Bound3UnwrapTraits;
    typedef typename StorageType::Bound4UnwrapTraits Bound4UnwrapTraits;
    typedef typename StorageType::Bound5UnwrapTraits Bound5UnwrapTraits;

    typename Bound1UnwrapTraits::ForwardType x1 =
        Bound1UnwrapTraits::Unwrap(storage->p1_);
    typename Bound2UnwrapTraits::ForwardType x2 =
        Bound2UnwrapTraits::Unwrap(storage->p2_);
    typename Bound3UnwrapTraits::ForwardType x3 =
        Bound3UnwrapTraits::Unwrap(storage->p3_);
    typename Bound4UnwrapTraits::ForwardType x4 =
        Bound4UnwrapTraits::Unwrap(storage->p4_);
    typename Bound5UnwrapTraits::ForwardType x5 =
        Bound5UnwrapTraits::Unwrap(storage->p5_);
    return InvokeHelper<StorageType::IsWeakCall::value, R,
           typename StorageType::RunnableType,
           void(typename Bound1UnwrapTraits::ForwardType,
               typename Bound2UnwrapTraits::ForwardType,
               typename Bound3UnwrapTraits::ForwardType,
               typename Bound4UnwrapTraits::ForwardType,
               typename Bound5UnwrapTraits::ForwardType,
               typename CallbackParamTraits<X6>::ForwardType x6)>
               ::MakeItSo(storage->runnable_, CallbackForward(x1),
                   CallbackForward(x2), CallbackForward(x3),
                   CallbackForward(x4), CallbackForward(x5),
                   CallbackForward(x6));
  }
};


template <typename StorageType, typename R,typename X1, typename X2,
    typename X3, typename X4, typename X5, typename X6>
struct Invoker<6, StorageType, R(X1, X2, X3, X4, X5, X6)> {
  typedef R(RunType)(BindStateBase*);

  typedef R(UnboundRunType)();

  static R Run(BindStateBase* base) {
    StorageType* storage = static_cast<StorageType*>(base);

    
    
    
    typedef typename StorageType::Bound1UnwrapTraits Bound1UnwrapTraits;
    typedef typename StorageType::Bound2UnwrapTraits Bound2UnwrapTraits;
    typedef typename StorageType::Bound3UnwrapTraits Bound3UnwrapTraits;
    typedef typename StorageType::Bound4UnwrapTraits Bound4UnwrapTraits;
    typedef typename StorageType::Bound5UnwrapTraits Bound5UnwrapTraits;
    typedef typename StorageType::Bound6UnwrapTraits Bound6UnwrapTraits;

    typename Bound1UnwrapTraits::ForwardType x1 =
        Bound1UnwrapTraits::Unwrap(storage->p1_);
    typename Bound2UnwrapTraits::ForwardType x2 =
        Bound2UnwrapTraits::Unwrap(storage->p2_);
    typename Bound3UnwrapTraits::ForwardType x3 =
        Bound3UnwrapTraits::Unwrap(storage->p3_);
    typename Bound4UnwrapTraits::ForwardType x4 =
        Bound4UnwrapTraits::Unwrap(storage->p4_);
    typename Bound5UnwrapTraits::ForwardType x5 =
        Bound5UnwrapTraits::Unwrap(storage->p5_);
    typename Bound6UnwrapTraits::ForwardType x6 =
        Bound6UnwrapTraits::Unwrap(storage->p6_);
    return InvokeHelper<StorageType::IsWeakCall::value, R,
           typename StorageType::RunnableType,
           void(typename Bound1UnwrapTraits::ForwardType,
               typename Bound2UnwrapTraits::ForwardType,
               typename Bound3UnwrapTraits::ForwardType,
               typename Bound4UnwrapTraits::ForwardType,
               typename Bound5UnwrapTraits::ForwardType,
               typename Bound6UnwrapTraits::ForwardType)>
               ::MakeItSo(storage->runnable_, CallbackForward(x1),
                   CallbackForward(x2), CallbackForward(x3),
                   CallbackForward(x4), CallbackForward(x5),
                   CallbackForward(x6));
  }
};


template <typename StorageType, typename R,typename X1, typename X2,
    typename X3, typename X4, typename X5, typename X6, typename X7>
struct Invoker<0, StorageType, R(X1, X2, X3, X4, X5, X6, X7)> {
  typedef R(RunType)(BindStateBase*,
      typename CallbackParamTraits<X1>::ForwardType,
      typename CallbackParamTraits<X2>::ForwardType,
      typename CallbackParamTraits<X3>::ForwardType,
      typename CallbackParamTraits<X4>::ForwardType,
      typename CallbackParamTraits<X5>::ForwardType,
      typename CallbackParamTraits<X6>::ForwardType,
      typename CallbackParamTraits<X7>::ForwardType);

  typedef R(UnboundRunType)(X1, X2, X3, X4, X5, X6, X7);

  static R Run(BindStateBase* base,
      typename CallbackParamTraits<X1>::ForwardType x1,
      typename CallbackParamTraits<X2>::ForwardType x2,
      typename CallbackParamTraits<X3>::ForwardType x3,
      typename CallbackParamTraits<X4>::ForwardType x4,
      typename CallbackParamTraits<X5>::ForwardType x5,
      typename CallbackParamTraits<X6>::ForwardType x6,
      typename CallbackParamTraits<X7>::ForwardType x7) {
    StorageType* storage = static_cast<StorageType*>(base);

    
    
    

    return InvokeHelper<StorageType::IsWeakCall::value, R,
           typename StorageType::RunnableType,
           void(typename CallbackParamTraits<X1>::ForwardType x1,
               typename CallbackParamTraits<X2>::ForwardType x2,
               typename CallbackParamTraits<X3>::ForwardType x3,
               typename CallbackParamTraits<X4>::ForwardType x4,
               typename CallbackParamTraits<X5>::ForwardType x5,
               typename CallbackParamTraits<X6>::ForwardType x6,
               typename CallbackParamTraits<X7>::ForwardType x7)>
               ::MakeItSo(storage->runnable_, CallbackForward(x1),
                   CallbackForward(x2), CallbackForward(x3),
                   CallbackForward(x4), CallbackForward(x5),
                   CallbackForward(x6), CallbackForward(x7));
  }
};


template <typename StorageType, typename R,typename X1, typename X2,
    typename X3, typename X4, typename X5, typename X6, typename X7>
struct Invoker<1, StorageType, R(X1, X2, X3, X4, X5, X6, X7)> {
  typedef R(RunType)(BindStateBase*,
      typename CallbackParamTraits<X2>::ForwardType,
      typename CallbackParamTraits<X3>::ForwardType,
      typename CallbackParamTraits<X4>::ForwardType,
      typename CallbackParamTraits<X5>::ForwardType,
      typename CallbackParamTraits<X6>::ForwardType,
      typename CallbackParamTraits<X7>::ForwardType);

  typedef R(UnboundRunType)(X2, X3, X4, X5, X6, X7);

  static R Run(BindStateBase* base,
      typename CallbackParamTraits<X2>::ForwardType x2,
      typename CallbackParamTraits<X3>::ForwardType x3,
      typename CallbackParamTraits<X4>::ForwardType x4,
      typename CallbackParamTraits<X5>::ForwardType x5,
      typename CallbackParamTraits<X6>::ForwardType x6,
      typename CallbackParamTraits<X7>::ForwardType x7) {
    StorageType* storage = static_cast<StorageType*>(base);

    
    
    
    typedef typename StorageType::Bound1UnwrapTraits Bound1UnwrapTraits;

    typename Bound1UnwrapTraits::ForwardType x1 =
        Bound1UnwrapTraits::Unwrap(storage->p1_);
    return InvokeHelper<StorageType::IsWeakCall::value, R,
           typename StorageType::RunnableType,
           void(typename Bound1UnwrapTraits::ForwardType,
               typename CallbackParamTraits<X2>::ForwardType x2,
               typename CallbackParamTraits<X3>::ForwardType x3,
               typename CallbackParamTraits<X4>::ForwardType x4,
               typename CallbackParamTraits<X5>::ForwardType x5,
               typename CallbackParamTraits<X6>::ForwardType x6,
               typename CallbackParamTraits<X7>::ForwardType x7)>
               ::MakeItSo(storage->runnable_, CallbackForward(x1),
                   CallbackForward(x2), CallbackForward(x3),
                   CallbackForward(x4), CallbackForward(x5),
                   CallbackForward(x6), CallbackForward(x7));
  }
};


template <typename StorageType, typename R,typename X1, typename X2,
    typename X3, typename X4, typename X5, typename X6, typename X7>
struct Invoker<2, StorageType, R(X1, X2, X3, X4, X5, X6, X7)> {
  typedef R(RunType)(BindStateBase*,
      typename CallbackParamTraits<X3>::ForwardType,
      typename CallbackParamTraits<X4>::ForwardType,
      typename CallbackParamTraits<X5>::ForwardType,
      typename CallbackParamTraits<X6>::ForwardType,
      typename CallbackParamTraits<X7>::ForwardType);

  typedef R(UnboundRunType)(X3, X4, X5, X6, X7);

  static R Run(BindStateBase* base,
      typename CallbackParamTraits<X3>::ForwardType x3,
      typename CallbackParamTraits<X4>::ForwardType x4,
      typename CallbackParamTraits<X5>::ForwardType x5,
      typename CallbackParamTraits<X6>::ForwardType x6,
      typename CallbackParamTraits<X7>::ForwardType x7) {
    StorageType* storage = static_cast<StorageType*>(base);

    
    
    
    typedef typename StorageType::Bound1UnwrapTraits Bound1UnwrapTraits;
    typedef typename StorageType::Bound2UnwrapTraits Bound2UnwrapTraits;

    typename Bound1UnwrapTraits::ForwardType x1 =
        Bound1UnwrapTraits::Unwrap(storage->p1_);
    typename Bound2UnwrapTraits::ForwardType x2 =
        Bound2UnwrapTraits::Unwrap(storage->p2_);
    return InvokeHelper<StorageType::IsWeakCall::value, R,
           typename StorageType::RunnableType,
           void(typename Bound1UnwrapTraits::ForwardType,
               typename Bound2UnwrapTraits::ForwardType,
               typename CallbackParamTraits<X3>::ForwardType x3,
               typename CallbackParamTraits<X4>::ForwardType x4,
               typename CallbackParamTraits<X5>::ForwardType x5,
               typename CallbackParamTraits<X6>::ForwardType x6,
               typename CallbackParamTraits<X7>::ForwardType x7)>
               ::MakeItSo(storage->runnable_, CallbackForward(x1),
                   CallbackForward(x2), CallbackForward(x3),
                   CallbackForward(x4), CallbackForward(x5),
                   CallbackForward(x6), CallbackForward(x7));
  }
};


template <typename StorageType, typename R,typename X1, typename X2,
    typename X3, typename X4, typename X5, typename X6, typename X7>
struct Invoker<3, StorageType, R(X1, X2, X3, X4, X5, X6, X7)> {
  typedef R(RunType)(BindStateBase*,
      typename CallbackParamTraits<X4>::ForwardType,
      typename CallbackParamTraits<X5>::ForwardType,
      typename CallbackParamTraits<X6>::ForwardType,
      typename CallbackParamTraits<X7>::ForwardType);

  typedef R(UnboundRunType)(X4, X5, X6, X7);

  static R Run(BindStateBase* base,
      typename CallbackParamTraits<X4>::ForwardType x4,
      typename CallbackParamTraits<X5>::ForwardType x5,
      typename CallbackParamTraits<X6>::ForwardType x6,
      typename CallbackParamTraits<X7>::ForwardType x7) {
    StorageType* storage = static_cast<StorageType*>(base);

    
    
    
    typedef typename StorageType::Bound1UnwrapTraits Bound1UnwrapTraits;
    typedef typename StorageType::Bound2UnwrapTraits Bound2UnwrapTraits;
    typedef typename StorageType::Bound3UnwrapTraits Bound3UnwrapTraits;

    typename Bound1UnwrapTraits::ForwardType x1 =
        Bound1UnwrapTraits::Unwrap(storage->p1_);
    typename Bound2UnwrapTraits::ForwardType x2 =
        Bound2UnwrapTraits::Unwrap(storage->p2_);
    typename Bound3UnwrapTraits::ForwardType x3 =
        Bound3UnwrapTraits::Unwrap(storage->p3_);
    return InvokeHelper<StorageType::IsWeakCall::value, R,
           typename StorageType::RunnableType,
           void(typename Bound1UnwrapTraits::ForwardType,
               typename Bound2UnwrapTraits::ForwardType,
               typename Bound3UnwrapTraits::ForwardType,
               typename CallbackParamTraits<X4>::ForwardType x4,
               typename CallbackParamTraits<X5>::ForwardType x5,
               typename CallbackParamTraits<X6>::ForwardType x6,
               typename CallbackParamTraits<X7>::ForwardType x7)>
               ::MakeItSo(storage->runnable_, CallbackForward(x1),
                   CallbackForward(x2), CallbackForward(x3),
                   CallbackForward(x4), CallbackForward(x5),
                   CallbackForward(x6), CallbackForward(x7));
  }
};


template <typename StorageType, typename R,typename X1, typename X2,
    typename X3, typename X4, typename X5, typename X6, typename X7>
struct Invoker<4, StorageType, R(X1, X2, X3, X4, X5, X6, X7)> {
  typedef R(RunType)(BindStateBase*,
      typename CallbackParamTraits<X5>::ForwardType,
      typename CallbackParamTraits<X6>::ForwardType,
      typename CallbackParamTraits<X7>::ForwardType);

  typedef R(UnboundRunType)(X5, X6, X7);

  static R Run(BindStateBase* base,
      typename CallbackParamTraits<X5>::ForwardType x5,
      typename CallbackParamTraits<X6>::ForwardType x6,
      typename CallbackParamTraits<X7>::ForwardType x7) {
    StorageType* storage = static_cast<StorageType*>(base);

    
    
    
    typedef typename StorageType::Bound1UnwrapTraits Bound1UnwrapTraits;
    typedef typename StorageType::Bound2UnwrapTraits Bound2UnwrapTraits;
    typedef typename StorageType::Bound3UnwrapTraits Bound3UnwrapTraits;
    typedef typename StorageType::Bound4UnwrapTraits Bound4UnwrapTraits;

    typename Bound1UnwrapTraits::ForwardType x1 =
        Bound1UnwrapTraits::Unwrap(storage->p1_);
    typename Bound2UnwrapTraits::ForwardType x2 =
        Bound2UnwrapTraits::Unwrap(storage->p2_);
    typename Bound3UnwrapTraits::ForwardType x3 =
        Bound3UnwrapTraits::Unwrap(storage->p3_);
    typename Bound4UnwrapTraits::ForwardType x4 =
        Bound4UnwrapTraits::Unwrap(storage->p4_);
    return InvokeHelper<StorageType::IsWeakCall::value, R,
           typename StorageType::RunnableType,
           void(typename Bound1UnwrapTraits::ForwardType,
               typename Bound2UnwrapTraits::ForwardType,
               typename Bound3UnwrapTraits::ForwardType,
               typename Bound4UnwrapTraits::ForwardType,
               typename CallbackParamTraits<X5>::ForwardType x5,
               typename CallbackParamTraits<X6>::ForwardType x6,
               typename CallbackParamTraits<X7>::ForwardType x7)>
               ::MakeItSo(storage->runnable_, CallbackForward(x1),
                   CallbackForward(x2), CallbackForward(x3),
                   CallbackForward(x4), CallbackForward(x5),
                   CallbackForward(x6), CallbackForward(x7));
  }
};


template <typename StorageType, typename R,typename X1, typename X2,
    typename X3, typename X4, typename X5, typename X6, typename X7>
struct Invoker<5, StorageType, R(X1, X2, X3, X4, X5, X6, X7)> {
  typedef R(RunType)(BindStateBase*,
      typename CallbackParamTraits<X6>::ForwardType,
      typename CallbackParamTraits<X7>::ForwardType);

  typedef R(UnboundRunType)(X6, X7);

  static R Run(BindStateBase* base,
      typename CallbackParamTraits<X6>::ForwardType x6,
      typename CallbackParamTraits<X7>::ForwardType x7) {
    StorageType* storage = static_cast<StorageType*>(base);

    
    
    
    typedef typename StorageType::Bound1UnwrapTraits Bound1UnwrapTraits;
    typedef typename StorageType::Bound2UnwrapTraits Bound2UnwrapTraits;
    typedef typename StorageType::Bound3UnwrapTraits Bound3UnwrapTraits;
    typedef typename StorageType::Bound4UnwrapTraits Bound4UnwrapTraits;
    typedef typename StorageType::Bound5UnwrapTraits Bound5UnwrapTraits;

    typename Bound1UnwrapTraits::ForwardType x1 =
        Bound1UnwrapTraits::Unwrap(storage->p1_);
    typename Bound2UnwrapTraits::ForwardType x2 =
        Bound2UnwrapTraits::Unwrap(storage->p2_);
    typename Bound3UnwrapTraits::ForwardType x3 =
        Bound3UnwrapTraits::Unwrap(storage->p3_);
    typename Bound4UnwrapTraits::ForwardType x4 =
        Bound4UnwrapTraits::Unwrap(storage->p4_);
    typename Bound5UnwrapTraits::ForwardType x5 =
        Bound5UnwrapTraits::Unwrap(storage->p5_);
    return InvokeHelper<StorageType::IsWeakCall::value, R,
           typename StorageType::RunnableType,
           void(typename Bound1UnwrapTraits::ForwardType,
               typename Bound2UnwrapTraits::ForwardType,
               typename Bound3UnwrapTraits::ForwardType,
               typename Bound4UnwrapTraits::ForwardType,
               typename Bound5UnwrapTraits::ForwardType,
               typename CallbackParamTraits<X6>::ForwardType x6,
               typename CallbackParamTraits<X7>::ForwardType x7)>
               ::MakeItSo(storage->runnable_, CallbackForward(x1),
                   CallbackForward(x2), CallbackForward(x3),
                   CallbackForward(x4), CallbackForward(x5),
                   CallbackForward(x6), CallbackForward(x7));
  }
};


template <typename StorageType, typename R,typename X1, typename X2,
    typename X3, typename X4, typename X5, typename X6, typename X7>
struct Invoker<6, StorageType, R(X1, X2, X3, X4, X5, X6, X7)> {
  typedef R(RunType)(BindStateBase*,
      typename CallbackParamTraits<X7>::ForwardType);

  typedef R(UnboundRunType)(X7);

  static R Run(BindStateBase* base,
      typename CallbackParamTraits<X7>::ForwardType x7) {
    StorageType* storage = static_cast<StorageType*>(base);

    
    
    
    typedef typename StorageType::Bound1UnwrapTraits Bound1UnwrapTraits;
    typedef typename StorageType::Bound2UnwrapTraits Bound2UnwrapTraits;
    typedef typename StorageType::Bound3UnwrapTraits Bound3UnwrapTraits;
    typedef typename StorageType::Bound4UnwrapTraits Bound4UnwrapTraits;
    typedef typename StorageType::Bound5UnwrapTraits Bound5UnwrapTraits;
    typedef typename StorageType::Bound6UnwrapTraits Bound6UnwrapTraits;

    typename Bound1UnwrapTraits::ForwardType x1 =
        Bound1UnwrapTraits::Unwrap(storage->p1_);
    typename Bound2UnwrapTraits::ForwardType x2 =
        Bound2UnwrapTraits::Unwrap(storage->p2_);
    typename Bound3UnwrapTraits::ForwardType x3 =
        Bound3UnwrapTraits::Unwrap(storage->p3_);
    typename Bound4UnwrapTraits::ForwardType x4 =
        Bound4UnwrapTraits::Unwrap(storage->p4_);
    typename Bound5UnwrapTraits::ForwardType x5 =
        Bound5UnwrapTraits::Unwrap(storage->p5_);
    typename Bound6UnwrapTraits::ForwardType x6 =
        Bound6UnwrapTraits::Unwrap(storage->p6_);
    return InvokeHelper<StorageType::IsWeakCall::value, R,
           typename StorageType::RunnableType,
           void(typename Bound1UnwrapTraits::ForwardType,
               typename Bound2UnwrapTraits::ForwardType,
               typename Bound3UnwrapTraits::ForwardType,
               typename Bound4UnwrapTraits::ForwardType,
               typename Bound5UnwrapTraits::ForwardType,
               typename Bound6UnwrapTraits::ForwardType,
               typename CallbackParamTraits<X7>::ForwardType x7)>
               ::MakeItSo(storage->runnable_, CallbackForward(x1),
                   CallbackForward(x2), CallbackForward(x3),
                   CallbackForward(x4), CallbackForward(x5),
                   CallbackForward(x6), CallbackForward(x7));
  }
};


template <typename StorageType, typename R,typename X1, typename X2,
    typename X3, typename X4, typename X5, typename X6, typename X7>
struct Invoker<7, StorageType, R(X1, X2, X3, X4, X5, X6, X7)> {
  typedef R(RunType)(BindStateBase*);

  typedef R(UnboundRunType)();

  static R Run(BindStateBase* base) {
    StorageType* storage = static_cast<StorageType*>(base);

    
    
    
    typedef typename StorageType::Bound1UnwrapTraits Bound1UnwrapTraits;
    typedef typename StorageType::Bound2UnwrapTraits Bound2UnwrapTraits;
    typedef typename StorageType::Bound3UnwrapTraits Bound3UnwrapTraits;
    typedef typename StorageType::Bound4UnwrapTraits Bound4UnwrapTraits;
    typedef typename StorageType::Bound5UnwrapTraits Bound5UnwrapTraits;
    typedef typename StorageType::Bound6UnwrapTraits Bound6UnwrapTraits;
    typedef typename StorageType::Bound7UnwrapTraits Bound7UnwrapTraits;

    typename Bound1UnwrapTraits::ForwardType x1 =
        Bound1UnwrapTraits::Unwrap(storage->p1_);
    typename Bound2UnwrapTraits::ForwardType x2 =
        Bound2UnwrapTraits::Unwrap(storage->p2_);
    typename Bound3UnwrapTraits::ForwardType x3 =
        Bound3UnwrapTraits::Unwrap(storage->p3_);
    typename Bound4UnwrapTraits::ForwardType x4 =
        Bound4UnwrapTraits::Unwrap(storage->p4_);
    typename Bound5UnwrapTraits::ForwardType x5 =
        Bound5UnwrapTraits::Unwrap(storage->p5_);
    typename Bound6UnwrapTraits::ForwardType x6 =
        Bound6UnwrapTraits::Unwrap(storage->p6_);
    typename Bound7UnwrapTraits::ForwardType x7 =
        Bound7UnwrapTraits::Unwrap(storage->p7_);
    return InvokeHelper<StorageType::IsWeakCall::value, R,
           typename StorageType::RunnableType,
           void(typename Bound1UnwrapTraits::ForwardType,
               typename Bound2UnwrapTraits::ForwardType,
               typename Bound3UnwrapTraits::ForwardType,
               typename Bound4UnwrapTraits::ForwardType,
               typename Bound5UnwrapTraits::ForwardType,
               typename Bound6UnwrapTraits::ForwardType,
               typename Bound7UnwrapTraits::ForwardType)>
               ::MakeItSo(storage->runnable_, CallbackForward(x1),
                   CallbackForward(x2), CallbackForward(x3),
                   CallbackForward(x4), CallbackForward(x5),
                   CallbackForward(x6), CallbackForward(x7));
  }
};














template <typename Runnable, typename RunType, typename BoundArgsType>
struct BindState;

template <typename Runnable, typename RunType>
struct BindState<Runnable, RunType, void()> : public BindStateBase {
  typedef Runnable RunnableType;
  typedef false_type IsWeakCall;
  typedef Invoker<0, BindState, RunType> InvokerType;
  typedef typename InvokerType::UnboundRunType UnboundRunType;
  explicit BindState(const Runnable& runnable)
      : runnable_(runnable) {
  }

  virtual ~BindState() {  }

  RunnableType runnable_;
};

template <typename Runnable, typename RunType, typename P1>
struct BindState<Runnable, RunType, void(P1)> : public BindStateBase {
  typedef Runnable RunnableType;
  typedef IsWeakMethod<HasIsMethodTag<Runnable>::value, P1> IsWeakCall;
  typedef Invoker<1, BindState, RunType> InvokerType;
  typedef typename InvokerType::UnboundRunType UnboundRunType;

  
  typedef UnwrapTraits<P1> Bound1UnwrapTraits;

  BindState(const Runnable& runnable, const P1& p1)
      : runnable_(runnable),
        p1_(p1) {
    MaybeRefcount<HasIsMethodTag<Runnable>::value, P1>::AddRef(p1_);
  }

  virtual ~BindState() {    MaybeRefcount<HasIsMethodTag<Runnable>::value,
      P1>::Release(p1_);  }

  RunnableType runnable_;
  P1 p1_;
};

template <typename Runnable, typename RunType, typename P1, typename P2>
struct BindState<Runnable, RunType, void(P1, P2)> : public BindStateBase {
  typedef Runnable RunnableType;
  typedef IsWeakMethod<HasIsMethodTag<Runnable>::value, P1> IsWeakCall;
  typedef Invoker<2, BindState, RunType> InvokerType;
  typedef typename InvokerType::UnboundRunType UnboundRunType;

  
  typedef UnwrapTraits<P1> Bound1UnwrapTraits;
  typedef UnwrapTraits<P2> Bound2UnwrapTraits;

  BindState(const Runnable& runnable, const P1& p1, const P2& p2)
      : runnable_(runnable),
        p1_(p1),
        p2_(p2) {
    MaybeRefcount<HasIsMethodTag<Runnable>::value, P1>::AddRef(p1_);
  }

  virtual ~BindState() {    MaybeRefcount<HasIsMethodTag<Runnable>::value,
      P1>::Release(p1_);  }

  RunnableType runnable_;
  P1 p1_;
  P2 p2_;
};

template <typename Runnable, typename RunType, typename P1, typename P2,
    typename P3>
struct BindState<Runnable, RunType, void(P1, P2, P3)> : public BindStateBase {
  typedef Runnable RunnableType;
  typedef IsWeakMethod<HasIsMethodTag<Runnable>::value, P1> IsWeakCall;
  typedef Invoker<3, BindState, RunType> InvokerType;
  typedef typename InvokerType::UnboundRunType UnboundRunType;

  
  typedef UnwrapTraits<P1> Bound1UnwrapTraits;
  typedef UnwrapTraits<P2> Bound2UnwrapTraits;
  typedef UnwrapTraits<P3> Bound3UnwrapTraits;

  BindState(const Runnable& runnable, const P1& p1, const P2& p2, const P3& p3)
      : runnable_(runnable),
        p1_(p1),
        p2_(p2),
        p3_(p3) {
    MaybeRefcount<HasIsMethodTag<Runnable>::value, P1>::AddRef(p1_);
  }

  virtual ~BindState() {    MaybeRefcount<HasIsMethodTag<Runnable>::value,
      P1>::Release(p1_);  }

  RunnableType runnable_;
  P1 p1_;
  P2 p2_;
  P3 p3_;
};

template <typename Runnable, typename RunType, typename P1, typename P2,
    typename P3, typename P4>
struct BindState<Runnable, RunType, void(P1, P2, P3,
    P4)> : public BindStateBase {
  typedef Runnable RunnableType;
  typedef IsWeakMethod<HasIsMethodTag<Runnable>::value, P1> IsWeakCall;
  typedef Invoker<4, BindState, RunType> InvokerType;
  typedef typename InvokerType::UnboundRunType UnboundRunType;

  
  typedef UnwrapTraits<P1> Bound1UnwrapTraits;
  typedef UnwrapTraits<P2> Bound2UnwrapTraits;
  typedef UnwrapTraits<P3> Bound3UnwrapTraits;
  typedef UnwrapTraits<P4> Bound4UnwrapTraits;

  BindState(const Runnable& runnable, const P1& p1, const P2& p2, const P3& p3,
      const P4& p4)
      : runnable_(runnable),
        p1_(p1),
        p2_(p2),
        p3_(p3),
        p4_(p4) {
    MaybeRefcount<HasIsMethodTag<Runnable>::value, P1>::AddRef(p1_);
  }

  virtual ~BindState() {    MaybeRefcount<HasIsMethodTag<Runnable>::value,
      P1>::Release(p1_);  }

  RunnableType runnable_;
  P1 p1_;
  P2 p2_;
  P3 p3_;
  P4 p4_;
};

template <typename Runnable, typename RunType, typename P1, typename P2,
    typename P3, typename P4, typename P5>
struct BindState<Runnable, RunType, void(P1, P2, P3, P4,
    P5)> : public BindStateBase {
  typedef Runnable RunnableType;
  typedef IsWeakMethod<HasIsMethodTag<Runnable>::value, P1> IsWeakCall;
  typedef Invoker<5, BindState, RunType> InvokerType;
  typedef typename InvokerType::UnboundRunType UnboundRunType;

  
  typedef UnwrapTraits<P1> Bound1UnwrapTraits;
  typedef UnwrapTraits<P2> Bound2UnwrapTraits;
  typedef UnwrapTraits<P3> Bound3UnwrapTraits;
  typedef UnwrapTraits<P4> Bound4UnwrapTraits;
  typedef UnwrapTraits<P5> Bound5UnwrapTraits;

  BindState(const Runnable& runnable, const P1& p1, const P2& p2, const P3& p3,
      const P4& p4, const P5& p5)
      : runnable_(runnable),
        p1_(p1),
        p2_(p2),
        p3_(p3),
        p4_(p4),
        p5_(p5) {
    MaybeRefcount<HasIsMethodTag<Runnable>::value, P1>::AddRef(p1_);
  }

  virtual ~BindState() {    MaybeRefcount<HasIsMethodTag<Runnable>::value,
      P1>::Release(p1_);  }

  RunnableType runnable_;
  P1 p1_;
  P2 p2_;
  P3 p3_;
  P4 p4_;
  P5 p5_;
};

template <typename Runnable, typename RunType, typename P1, typename P2,
    typename P3, typename P4, typename P5, typename P6>
struct BindState<Runnable, RunType, void(P1, P2, P3, P4, P5,
    P6)> : public BindStateBase {
  typedef Runnable RunnableType;
  typedef IsWeakMethod<HasIsMethodTag<Runnable>::value, P1> IsWeakCall;
  typedef Invoker<6, BindState, RunType> InvokerType;
  typedef typename InvokerType::UnboundRunType UnboundRunType;

  
  typedef UnwrapTraits<P1> Bound1UnwrapTraits;
  typedef UnwrapTraits<P2> Bound2UnwrapTraits;
  typedef UnwrapTraits<P3> Bound3UnwrapTraits;
  typedef UnwrapTraits<P4> Bound4UnwrapTraits;
  typedef UnwrapTraits<P5> Bound5UnwrapTraits;
  typedef UnwrapTraits<P6> Bound6UnwrapTraits;

  BindState(const Runnable& runnable, const P1& p1, const P2& p2, const P3& p3,
      const P4& p4, const P5& p5, const P6& p6)
      : runnable_(runnable),
        p1_(p1),
        p2_(p2),
        p3_(p3),
        p4_(p4),
        p5_(p5),
        p6_(p6) {
    MaybeRefcount<HasIsMethodTag<Runnable>::value, P1>::AddRef(p1_);
  }

  virtual ~BindState() {    MaybeRefcount<HasIsMethodTag<Runnable>::value,
      P1>::Release(p1_);  }

  RunnableType runnable_;
  P1 p1_;
  P2 p2_;
  P3 p3_;
  P4 p4_;
  P5 p5_;
  P6 p6_;
};

template <typename Runnable, typename RunType, typename P1, typename P2,
    typename P3, typename P4, typename P5, typename P6, typename P7>
struct BindState<Runnable, RunType, void(P1, P2, P3, P4, P5, P6,
    P7)> : public BindStateBase {
  typedef Runnable RunnableType;
  typedef IsWeakMethod<HasIsMethodTag<Runnable>::value, P1> IsWeakCall;
  typedef Invoker<7, BindState, RunType> InvokerType;
  typedef typename InvokerType::UnboundRunType UnboundRunType;

  
  typedef UnwrapTraits<P1> Bound1UnwrapTraits;
  typedef UnwrapTraits<P2> Bound2UnwrapTraits;
  typedef UnwrapTraits<P3> Bound3UnwrapTraits;
  typedef UnwrapTraits<P4> Bound4UnwrapTraits;
  typedef UnwrapTraits<P5> Bound5UnwrapTraits;
  typedef UnwrapTraits<P6> Bound6UnwrapTraits;
  typedef UnwrapTraits<P7> Bound7UnwrapTraits;

  BindState(const Runnable& runnable, const P1& p1, const P2& p2, const P3& p3,
      const P4& p4, const P5& p5, const P6& p6, const P7& p7)
      : runnable_(runnable),
        p1_(p1),
        p2_(p2),
        p3_(p3),
        p4_(p4),
        p5_(p5),
        p6_(p6),
        p7_(p7) {
    MaybeRefcount<HasIsMethodTag<Runnable>::value, P1>::AddRef(p1_);
  }

  virtual ~BindState() {    MaybeRefcount<HasIsMethodTag<Runnable>::value,
      P1>::Release(p1_);  }

  RunnableType runnable_;
  P1 p1_;
  P2 p2_;
  P3 p3_;
  P4 p4_;
  P5 p5_;
  P6 p6_;
  P7 p7_;
};

}  
}  

#endif  
