





#ifndef nsThreadUtils_h__
#define nsThreadUtils_h__

#include "prthread.h"
#include "prinrval.h"
#include "MainThreadUtils.h"
#include "nsIThreadManager.h"
#include "nsIThread.h"
#include "nsIRunnable.h"
#include "nsICancelableRunnable.h"
#include "nsStringGlue.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "mozilla/Atomics.h"
#include "mozilla/Likely.h"








extern void NS_SetThreadName(nsIThread* aThread, const nsACString& aName);





template<size_t LEN>
inline void
NS_SetThreadName(nsIThread* aThread, const char (&aName)[LEN])
{
  static_assert(LEN <= 16,
                "Thread name must be no more than 16 characters");
  NS_SetThreadName(aThread, nsDependentCString(aName));
}














extern NS_METHOD
NS_NewThread(nsIThread** aResult,
             nsIRunnable* aInitialEvent = nullptr,
             uint32_t aStackSize = nsIThreadManager::DEFAULT_STACK_SIZE);




template<size_t LEN>
inline NS_METHOD
NS_NewNamedThread(const char (&aName)[LEN],
                  nsIThread** aResult,
                  nsIRunnable* aInitialEvent = nullptr,
                  uint32_t aStackSize = nsIThreadManager::DEFAULT_STACK_SIZE)
{
  
  nsCOMPtr<nsIThread> thread;
  nsresult rv = NS_NewThread(getter_AddRefs(thread), nullptr, aStackSize);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }
  NS_SetThreadName<LEN>(thread, aName);
  if (aInitialEvent) {
    rv = thread->Dispatch(aInitialEvent, NS_DISPATCH_NORMAL);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Initial event dispatch failed");
  }

  *aResult = nullptr;
  thread.swap(*aResult);
  return rv;
}







extern NS_METHOD NS_GetCurrentThread(nsIThread** aResult);










extern NS_METHOD NS_DispatchToCurrentThread(nsIRunnable* aEvent);












extern NS_METHOD
NS_DispatchToMainThread(nsIRunnable* aEvent,
                        uint32_t aDispatchFlags = NS_DISPATCH_NORMAL);

#ifndef XPCOM_GLUE_AVOID_NSPR















extern NS_METHOD
NS_ProcessPendingEvents(nsIThread* aThread,
                        PRIntervalTime aTimeout = PR_INTERVAL_NO_TIMEOUT);
#endif















extern bool NS_HasPendingEvents(nsIThread* aThread = nullptr);


















extern bool NS_ProcessNextEvent(nsIThread* aThread = nullptr,
                                bool aMayWait = true);




inline already_AddRefed<nsIThread>
do_GetCurrentThread()
{
  nsIThread* thread = nullptr;
  NS_GetCurrentThread(&thread);
  return already_AddRefed<nsIThread>(thread);
}

inline already_AddRefed<nsIThread>
do_GetMainThread()
{
  nsIThread* thread = nullptr;
  NS_GetMainThread(&thread);
  return already_AddRefed<nsIThread>(thread);
}



#ifdef MOZILLA_INTERNAL_API




extern nsIThread* NS_GetCurrentThread();
#endif



#ifndef XPCOM_GLUE_AVOID_NSPR


class nsRunnable : public nsIRunnable
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIRUNNABLE

  nsRunnable() {}

protected:
  virtual ~nsRunnable() {}
};


class nsCancelableRunnable : public nsICancelableRunnable
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIRUNNABLE
  NS_DECL_NSICANCELABLERUNNABLE

  nsCancelableRunnable() {}

protected:
  virtual ~nsCancelableRunnable() {}
};




template<class ClassType,
         typename ReturnType = void,
         bool Owning = true>
class nsRunnableMethod : public nsRunnable
{
public:
  virtual void Revoke() = 0;

  
  
  
  template<typename OtherReturnType>
  class ReturnTypeEnforcer
  {
  public:
    typedef int ReturnTypeIsSafe;
  };

  template<class T>
  class ReturnTypeEnforcer<already_AddRefed<T>>
  {
    
  };

  
  typedef typename ReturnTypeEnforcer<ReturnType>::ReturnTypeIsSafe check;
};

template<class ClassType, bool Owning>
struct nsRunnableMethodReceiver
{
  nsRefPtr<ClassType> mObj;
  explicit nsRunnableMethodReceiver(ClassType* aObj) : mObj(aObj) {}
  ~nsRunnableMethodReceiver() { Revoke(); }
  ClassType* Get() const { return mObj.get(); }
  void Revoke() { mObj = nullptr; }
};

template<class ClassType>
struct nsRunnableMethodReceiver<ClassType, false>
{
  ClassType* MOZ_NON_OWNING_REF mObj;
  explicit nsRunnableMethodReceiver(ClassType* aObj) : mObj(aObj) {}
  ClassType* Get() const { return mObj; }
  void Revoke() { mObj = nullptr; }
};

template<typename Method, bool Owning> struct nsRunnableMethodTraits;

template<class C, typename R, bool Owning, typename... As>
struct nsRunnableMethodTraits<R(C::*)(As...), Owning>
{
  typedef C class_type;
  typedef R return_type;
  typedef nsRunnableMethod<C, R, Owning> base_type;
};

#ifdef NS_HAVE_STDCALL
template<class C, typename R, bool Owning, typename... As>
struct nsRunnableMethodTraits<R(__stdcall C::*)(As...), Owning>
{
  typedef C class_type;
  typedef R return_type;
  typedef nsRunnableMethod<C, R, Owning> base_type;
};

template<class C, typename R, bool Owning>
struct nsRunnableMethodTraits<R(NS_STDCALL C::*)(), Owning>
{
  typedef C class_type;
  typedef R return_type;
  typedef nsRunnableMethod<C, R, Owning> base_type;
};
#endif








template<typename T>
struct IsParameterStorageClass : public mozilla::FalseType {};




template<typename T>
struct StoreCopyPassByValue
{
  typedef T stored_type;
  typedef T passed_type;
  stored_type m;
  template <typename A>
  StoreCopyPassByValue(A&& a) : m(mozilla::Forward<A>(a)) {}
  passed_type PassAsParameter() { return m; }
};
template<typename S>
struct IsParameterStorageClass<StoreCopyPassByValue<S>>
  : public mozilla::TrueType {};

template<typename T>
struct StoreCopyPassByConstLRef
{
  typedef T stored_type;
  typedef const T& passed_type;
  stored_type m;
  template <typename A>
  StoreCopyPassByConstLRef(A&& a) : m(mozilla::Forward<A>(a)) {}
  passed_type PassAsParameter() { return m; }
};
template<typename S>
struct IsParameterStorageClass<StoreCopyPassByConstLRef<S>>
  : public mozilla::TrueType {};

template<typename T>
struct StoreCopyPassByLRef
{
  typedef T stored_type;
  typedef T& passed_type;
  stored_type m;
  template <typename A>
  StoreCopyPassByLRef(A&& a) : m(mozilla::Forward<A>(a)) {}
  passed_type PassAsParameter() { return m; }
};
template<typename S>
struct IsParameterStorageClass<StoreCopyPassByLRef<S>>
  : public mozilla::TrueType {};

template<typename T>
struct StoreCopyPassByRRef
{
  typedef T stored_type;
  typedef T&& passed_type;
  stored_type m;
  template <typename A>
  StoreCopyPassByRRef(A&& a) : m(mozilla::Forward<A>(a)) {}
  passed_type PassAsParameter() { return mozilla::Move(m); }
};
template<typename S>
struct IsParameterStorageClass<StoreCopyPassByRRef<S>>
  : public mozilla::TrueType {};

template<typename T>
struct StoreRefPassByLRef
{
  typedef T& stored_type;
  typedef T& passed_type;
  stored_type m;
  template <typename A>
  StoreRefPassByLRef(A& a) : m(a) {}
  passed_type PassAsParameter() { return m; }
};
template<typename S>
struct IsParameterStorageClass<StoreRefPassByLRef<S>>
  : public mozilla::TrueType {};

template<typename T>
struct StoreConstRefPassByConstLRef
{
  typedef const T& stored_type;
  typedef const T& passed_type;
  stored_type m;
  template <typename A>
  StoreConstRefPassByConstLRef(const A& a) : m(a) {}
  passed_type PassAsParameter() { return m; }
};
template<typename S>
struct IsParameterStorageClass<StoreConstRefPassByConstLRef<S>>
  : public mozilla::TrueType {};

template<typename T>
struct StorensRefPtrPassByPtr
{
  typedef nsRefPtr<T> stored_type;
  typedef T* passed_type;
  stored_type m;
  template <typename A>
  StorensRefPtrPassByPtr(A a) : m(a) {}
  passed_type PassAsParameter() { return m.get(); }
};
template<typename S>
struct IsParameterStorageClass<StorensRefPtrPassByPtr<S>>
  : public mozilla::TrueType {};

template<typename T>
struct StorePtrPassByPtr
{
  typedef T* stored_type;
  typedef T* passed_type;
  stored_type m;
  template <typename A>
  StorePtrPassByPtr(A a) : m(a) {}
  passed_type PassAsParameter() { return m; }
};
template<typename S>
struct IsParameterStorageClass<StorePtrPassByPtr<S>>
  : public mozilla::TrueType {};

template<typename T>
struct StoreConstPtrPassByConstPtr
{
  typedef const T* stored_type;
  typedef const T* passed_type;
  stored_type m;
  template <typename A>
  StoreConstPtrPassByConstPtr(A a) : m(a) {}
  passed_type PassAsParameter() { return m; }
};
template<typename S>
struct IsParameterStorageClass<StoreConstPtrPassByConstPtr<S>>
  : public mozilla::TrueType {};

template<typename T>
struct StoreCopyPassByConstPtr
{
  typedef T stored_type;
  typedef const T* passed_type;
  stored_type m;
  template <typename A>
  StoreCopyPassByConstPtr(A&& a) : m(mozilla::Forward<A>(a)) {}
  passed_type PassAsParameter() { return &m; }
};
template<typename S>
struct IsParameterStorageClass<StoreCopyPassByConstPtr<S>>
  : public mozilla::TrueType {};

template<typename T>
struct StoreCopyPassByPtr
{
  typedef T stored_type;
  typedef T* passed_type;
  stored_type m;
  template <typename A>
  StoreCopyPassByPtr(A&& a) : m(mozilla::Forward<A>(a)) {}
  passed_type PassAsParameter() { return &m; }
};
template<typename S>
struct IsParameterStorageClass<StoreCopyPassByPtr<S>>
  : public mozilla::TrueType {};

namespace detail {

template<typename TWithoutPointer>
struct NonnsISupportsPointerStorageClass
  : mozilla::Conditional<mozilla::IsConst<TWithoutPointer>::value,
                         StoreConstPtrPassByConstPtr<
                           typename mozilla::RemoveConst<TWithoutPointer>::Type>,
                         StorePtrPassByPtr<TWithoutPointer>>
{};

template<typename TWithoutPointer>
struct PointerStorageClass
  : mozilla::Conditional<mozilla::IsBaseOf<nsISupports, TWithoutPointer>::value,
                         StorensRefPtrPassByPtr<TWithoutPointer>,
                         typename NonnsISupportsPointerStorageClass<
                           TWithoutPointer
                         >::Type>
{};

template<typename TWithoutRef>
struct LValueReferenceStorageClass
  : mozilla::Conditional<mozilla::IsConst<TWithoutRef>::value,
                         StoreConstRefPassByConstLRef<
                           typename mozilla::RemoveConst<TWithoutRef>::Type>,
                         StoreRefPassByLRef<TWithoutRef>>
{};

template<typename T>
struct NonLValueReferenceStorageClass
  : mozilla::Conditional<mozilla::IsRvalueReference<T>::value,
                         StoreCopyPassByRRef<
                           typename mozilla::RemoveReference<T>::Type>,
                         StoreCopyPassByValue<T>>
{};

template<typename T>
struct NonPointerStorageClass
  : mozilla::Conditional<mozilla::IsLvalueReference<T>::value,
                         typename LValueReferenceStorageClass<
                           typename mozilla::RemoveReference<T>::Type
                         >::Type,
                         typename NonLValueReferenceStorageClass<T>::Type>
{};

template<typename T>
struct NonParameterStorageClass
  : mozilla::Conditional<mozilla::IsPointer<T>::value,
                         typename PointerStorageClass<
                           typename mozilla::RemovePointer<T>::Type
                         >::Type,
                         typename NonPointerStorageClass<T>::Type>
{};

















template<typename T>
struct ParameterStorage
  : mozilla::Conditional<IsParameterStorageClass<T>::value,
                         T,
                         typename NonParameterStorageClass<T>::Type>
{};

} 


template <typename... Ts> struct nsRunnableMethodArguments;



template <>
struct nsRunnableMethodArguments<>
{
  template<class C, typename M> void apply(C* o, M m)
  {
    ((*o).*m)();
  }
};
template <typename T0>
struct nsRunnableMethodArguments<T0>
{
  typename ::detail::ParameterStorage<T0>::Type m0;
  template<typename A0>
  nsRunnableMethodArguments(A0&& a0)
    : m0(mozilla::Forward<A0>(a0))
  {}
  template<class C, typename M> void apply(C* o, M m)
  {
    ((*o).*m)(m0.PassAsParameter());
  }
};
template <typename T0, typename T1>
struct nsRunnableMethodArguments<T0, T1>
{
  typename ::detail::ParameterStorage<T0>::Type m0;
  typename ::detail::ParameterStorage<T1>::Type m1;
  template<typename A0, typename A1>
  nsRunnableMethodArguments(A0&& a0, A1&& a1)
    : m0(mozilla::Forward<A0>(a0))
    , m1(mozilla::Forward<A1>(a1))
  {}
  template<class C, typename M> void apply(C* o, M m)
  {
    ((*o).*m)(m0.PassAsParameter(), m1.PassAsParameter());
  }
};
template <typename T0, typename T1, typename T2>
struct nsRunnableMethodArguments<T0, T1, T2>
{
  typename ::detail::ParameterStorage<T0>::Type m0;
  typename ::detail::ParameterStorage<T1>::Type m1;
  typename ::detail::ParameterStorage<T2>::Type m2;
  template<typename A0, typename A1, typename A2>
  nsRunnableMethodArguments(A0&& a0, A1&& a1, A2&& a2)
    : m0(mozilla::Forward<A0>(a0))
    , m1(mozilla::Forward<A1>(a1))
    , m2(mozilla::Forward<A2>(a2))
  {}
  template<class C, typename M> void apply(C* o, M m)
  {
    ((*o).*m)(m0.PassAsParameter(), m1.PassAsParameter(), m2.PassAsParameter());
  }
};
template <typename T0, typename T1, typename T2, typename T3>
struct nsRunnableMethodArguments<T0, T1, T2, T3>
{
  typename ::detail::ParameterStorage<T0>::Type m0;
  typename ::detail::ParameterStorage<T1>::Type m1;
  typename ::detail::ParameterStorage<T2>::Type m2;
  typename ::detail::ParameterStorage<T3>::Type m3;
  template<typename A0, typename A1, typename A2, typename A3>
  nsRunnableMethodArguments(A0&& a0, A1&& a1, A2&& a2, A3&& a3)
    : m0(mozilla::Forward<A0>(a0))
    , m1(mozilla::Forward<A1>(a1))
    , m2(mozilla::Forward<A2>(a2))
    , m3(mozilla::Forward<A3>(a3))
  {}
  template<class C, typename M> void apply(C* o, M m)
  {
    ((*o).*m)(m0.PassAsParameter(), m1.PassAsParameter(),
              m2.PassAsParameter(), m3.PassAsParameter());
  }
};
template <typename T0, typename T1, typename T2, typename T3, typename T4>
struct nsRunnableMethodArguments<T0, T1, T2, T3, T4>
{
  typename ::detail::ParameterStorage<T0>::Type m0;
  typename ::detail::ParameterStorage<T1>::Type m1;
  typename ::detail::ParameterStorage<T2>::Type m2;
  typename ::detail::ParameterStorage<T3>::Type m3;
  typename ::detail::ParameterStorage<T4>::Type m4;
  template<typename A0, typename A1, typename A2, typename A3, typename A4>
  nsRunnableMethodArguments(A0&& a0, A1&& a1, A2&& a2, A3&& a3, A4&& a4)
    : m0(mozilla::Forward<A0>(a0))
    , m1(mozilla::Forward<A1>(a1))
    , m2(mozilla::Forward<A2>(a2))
    , m3(mozilla::Forward<A3>(a3))
    , m4(mozilla::Forward<A4>(a4))
  {}
  template<class C, typename M> void apply(C* o, M m)
  {
    ((*o).*m)(m0.PassAsParameter(), m1.PassAsParameter(),
              m2.PassAsParameter(), m3.PassAsParameter(),
              m4.PassAsParameter());
  }
};
template <typename T0, typename T1, typename T2, typename T3, typename T4,
          typename T5>
struct nsRunnableMethodArguments<T0, T1, T2, T3, T4, T5>
{
  typename ::detail::ParameterStorage<T0>::Type m0;
  typename ::detail::ParameterStorage<T1>::Type m1;
  typename ::detail::ParameterStorage<T2>::Type m2;
  typename ::detail::ParameterStorage<T3>::Type m3;
  typename ::detail::ParameterStorage<T4>::Type m4;
  typename ::detail::ParameterStorage<T5>::Type m5;
  template<typename A0, typename A1, typename A2, typename A3, typename A4,
           typename A5>
  nsRunnableMethodArguments(A0&& a0, A1&& a1, A2&& a2, A3&& a3, A4&& a4,
        A5&& a5)
    : m0(mozilla::Forward<A0>(a0))
    , m1(mozilla::Forward<A1>(a1))
    , m2(mozilla::Forward<A2>(a2))
    , m3(mozilla::Forward<A3>(a3))
    , m4(mozilla::Forward<A4>(a4))
    , m5(mozilla::Forward<A5>(a5))
  {}
  template<class C, typename M> void apply(C* o, M m)
  {
    ((*o).*m)(m0.PassAsParameter(), m1.PassAsParameter(),
              m2.PassAsParameter(), m3.PassAsParameter(),
              m4.PassAsParameter(), m5.PassAsParameter());
  }
};
template <typename T0, typename T1, typename T2, typename T3, typename T4,
          typename T5, typename T6>
struct nsRunnableMethodArguments<T0, T1, T2, T3, T4, T5, T6>
{
  typename ::detail::ParameterStorage<T0>::Type m0;
  typename ::detail::ParameterStorage<T1>::Type m1;
  typename ::detail::ParameterStorage<T2>::Type m2;
  typename ::detail::ParameterStorage<T3>::Type m3;
  typename ::detail::ParameterStorage<T4>::Type m4;
  typename ::detail::ParameterStorage<T5>::Type m5;
  typename ::detail::ParameterStorage<T6>::Type m6;
  template<typename A0, typename A1, typename A2, typename A3, typename A4,
           typename A5, typename A6>
  nsRunnableMethodArguments(A0&& a0, A1&& a1, A2&& a2, A3&& a3, A4&& a4,
        A5&& a5, A6&& a6)
    : m0(mozilla::Forward<A0>(a0))
    , m1(mozilla::Forward<A1>(a1))
    , m2(mozilla::Forward<A2>(a2))
    , m3(mozilla::Forward<A3>(a3))
    , m4(mozilla::Forward<A4>(a4))
    , m5(mozilla::Forward<A5>(a5))
    , m6(mozilla::Forward<A6>(a6))
  {}
  template<class C, typename M> void apply(C* o, M m)
  {
    ((*o).*m)(m0.PassAsParameter(), m1.PassAsParameter(),
              m2.PassAsParameter(), m3.PassAsParameter(),
              m4.PassAsParameter(), m5.PassAsParameter(),
              m6.PassAsParameter());
  }
};
template <typename T0, typename T1, typename T2, typename T3, typename T4,
          typename T5, typename T6, typename T7>
struct nsRunnableMethodArguments<T0, T1, T2, T3, T4, T5, T6, T7>
{
  typename ::detail::ParameterStorage<T0>::Type m0;
  typename ::detail::ParameterStorage<T1>::Type m1;
  typename ::detail::ParameterStorage<T2>::Type m2;
  typename ::detail::ParameterStorage<T3>::Type m3;
  typename ::detail::ParameterStorage<T4>::Type m4;
  typename ::detail::ParameterStorage<T5>::Type m5;
  typename ::detail::ParameterStorage<T6>::Type m6;
  typename ::detail::ParameterStorage<T7>::Type m7;
  template<typename A0, typename A1, typename A2, typename A3, typename A4,
           typename A5, typename A6, typename A7>
  nsRunnableMethodArguments(A0&& a0, A1&& a1, A2&& a2, A3&& a3, A4&& a4,
        A5&& a5, A6&& a6, A7&& a7)
    : m0(mozilla::Forward<A0>(a0))
    , m1(mozilla::Forward<A1>(a1))
    , m2(mozilla::Forward<A2>(a2))
    , m3(mozilla::Forward<A3>(a3))
    , m4(mozilla::Forward<A4>(a4))
    , m5(mozilla::Forward<A5>(a5))
    , m6(mozilla::Forward<A6>(a6))
    , m7(mozilla::Forward<A7>(a7))
  {}
  template<class C, typename M> void apply(C* o, M m)
  {
    ((*o).*m)(m0.PassAsParameter(), m1.PassAsParameter(),
              m2.PassAsParameter(), m3.PassAsParameter(),
              m4.PassAsParameter(), m5.PassAsParameter(),
              m6.PassAsParameter(), m7.PassAsParameter());
  }
};

template<typename Method, bool Owning, typename... Storages>
class nsRunnableMethodImpl
  : public nsRunnableMethodTraits<Method, Owning>::base_type
{
  typedef typename nsRunnableMethodTraits<Method, Owning>::class_type
      ClassType;
  nsRunnableMethodReceiver<ClassType, Owning> mReceiver;
  Method mMethod;
  nsRunnableMethodArguments<Storages...> mArgs;
public:
  virtual ~nsRunnableMethodImpl() { Revoke(); };
  template<typename... Args>
  explicit nsRunnableMethodImpl(ClassType* aObj, Method aMethod,
                                Args&&... aArgs)
    : mReceiver(aObj)
    , mMethod(aMethod)
    , mArgs(mozilla::Forward<Args>(aArgs)...)
  {
    static_assert(sizeof...(Storages) == sizeof...(Args), "Storages and Args should have equal sizes");
  }
  NS_IMETHOD Run()
  {
    if (MOZ_LIKELY(mReceiver.Get())) {
      mArgs.apply(mReceiver.Get(), mMethod);
    }
    return NS_OK;
  }
  void Revoke() { mReceiver.Revoke(); }
};











template<typename PtrType, typename Method>
typename nsRunnableMethodTraits<Method, true>::base_type*
NS_NewRunnableMethod(PtrType aPtr, Method aMethod)
{
  return new nsRunnableMethodImpl<Method, true>(aPtr, aMethod);
}

template<typename PtrType, typename Method>
typename nsRunnableMethodTraits<Method, false>::base_type*
NS_NewNonOwningRunnableMethod(PtrType&& aPtr, Method aMethod)
{
  return new nsRunnableMethodImpl<Method, false>(aPtr, aMethod);
}





template<typename Storage, typename Method, typename PtrType, typename Arg>
typename nsRunnableMethodTraits<Method, true>::base_type*
NS_NewRunnableMethodWithArg(PtrType&& aPtr, Method aMethod, Arg&& aArg)
{
  return new nsRunnableMethodImpl<Method, true, Storage>(
      aPtr, aMethod, mozilla::Forward<Arg>(aArg));
}





template<typename... Storages, typename Method, typename PtrType, typename... Args>
typename nsRunnableMethodTraits<Method, true>::base_type*
NS_NewRunnableMethodWithArgs(PtrType&& aPtr, Method aMethod, Args&&... aArgs)
{
  static_assert(sizeof...(Storages) == sizeof...(Args),
                "<Storages...> size should be equal to number of arguments");
  return new nsRunnableMethodImpl<Method, true, Storages...>(
      aPtr, aMethod, mozilla::Forward<Args>(aArgs)...);
}

template<typename... Storages, typename Method, typename PtrType, typename... Args>
typename nsRunnableMethodTraits<Method, false>::base_type*
NS_NewNonOwningRunnableMethodWithArgs(PtrType&& aPtr, Method aMethod,
                                      Args&&... aArgs)
{
  static_assert(sizeof...(Storages) == sizeof...(Args),
                "<Storages...> size should be equal to number of arguments");
  return new nsRunnableMethodImpl<Method, false, Storages...>(
      aPtr, aMethod, mozilla::Forward<Args>(aArgs)...);
}

#endif  














































template<class T>
class nsRevocableEventPtr
{
public:
  nsRevocableEventPtr() : mEvent(nullptr) {}
  ~nsRevocableEventPtr() { Revoke(); }

  const nsRevocableEventPtr& operator=(T* aEvent)
  {
    if (mEvent != aEvent) {
      Revoke();
      mEvent = aEvent;
    }
    return *this;
  }

  void Revoke()
  {
    if (mEvent) {
      mEvent->Revoke();
      mEvent = nullptr;
    }
  }

  void Forget() { mEvent = nullptr; }
  bool IsPending() { return mEvent != nullptr; }
  T* get() { return mEvent; }

private:
  
  nsRevocableEventPtr(const nsRevocableEventPtr&);
  nsRevocableEventPtr& operator=(const nsRevocableEventPtr&);

  nsRefPtr<T> mEvent;
};





class nsThreadPoolNaming
{
public:
  nsThreadPoolNaming() : mCounter(0) {}

  




  void SetThreadPoolName(const nsACString& aPoolName,
                         nsIThread* aThread = nullptr);

private:
  mozilla::Atomic<uint32_t> mCounter;

  nsThreadPoolNaming(const nsThreadPoolNaming&) = delete;
  void operator=(const nsThreadPoolNaming&) = delete;
};







class MOZ_STACK_CLASS nsAutoLowPriorityIO
{
public:
  nsAutoLowPriorityIO();
  ~nsAutoLowPriorityIO();

private:
  bool lowIOPrioritySet;
#if defined(XP_MACOSX)
  int oldPriority;
#endif
};

void
NS_SetMainThread();




#ifdef MOZILLA_INTERNAL_API
#ifdef MOZ_NUWA_PROCESS
extern void
NS_SetIgnoreStatusOfCurrentThread();
#else 
inline void
NS_SetIgnoreStatusOfCurrentThread()
{
}
#endif 
#endif 

#endif  
