







#ifndef nsISupportsImpl_h__
#define nsISupportsImpl_h__

#include "nscore.h"
#include "nsISupportsBase.h"
#include "nsISupportsUtils.h"


#if !defined(XPCOM_GLUE_AVOID_NSPR)
#include "prthread.h" 
#endif 

#include "nsDebug.h"
#include "nsXPCOM.h"
#ifndef XPCOM_GLUE
#include "mozilla/Atomics.h"
#endif
#include "mozilla/Attributes.h"
#include "mozilla/Assertions.h"
#include "mozilla/Compiler.h"
#include "mozilla/Likely.h"
#include "mozilla/MacroArgs.h"
#include "mozilla/MacroForEach.h"

namespace mozilla {
template <typename T>
struct HasDangerousPublicDestructor
{
  static const bool value = false;
};
}

#if defined(__clang__)
   
   
   
   
#  define MOZ_CAN_USE_IS_DESTRUCTIBLE_FALLBACK
#elif defined(__GNUC__)
   
#  if MOZ_USING_LIBSTDCXX && MOZ_GCC_VERSION_AT_LEAST(4, 8, 0)
#    define MOZ_HAVE_STD_IS_DESTRUCTIBLE


#  elif MOZ_GCC_VERSION_AT_LEAST(4, 8, 0)
#    define MOZ_CAN_USE_IS_DESTRUCTIBLE_FALLBACK
#  endif
#endif

#ifdef MOZ_HAVE_STD_IS_DESTRUCTIBLE
#  include <type_traits>
#  define MOZ_IS_DESTRUCTIBLE(X) (std::is_destructible<X>::value)
#elif defined MOZ_CAN_USE_IS_DESTRUCTIBLE_FALLBACK
  namespace mozilla {
    struct IsDestructibleFallbackImpl
    {
      template<typename T> static T&& Declval();

      template<typename T, typename = decltype(Declval<T>().~T())>
      static TrueType Test(int);

      template<typename>
      static FalseType Test(...);

      template<typename T>
      struct Selector
      {
        typedef decltype(Test<T>(0)) type;
      };
    };

    template<typename T>
    struct IsDestructibleFallback
      : IsDestructibleFallbackImpl::Selector<T>::type
    {
    };
  }
#  define MOZ_IS_DESTRUCTIBLE(X) (mozilla::IsDestructibleFallback<X>::value)
#endif

#ifdef MOZ_IS_DESTRUCTIBLE
#define MOZ_ASSERT_TYPE_OK_FOR_REFCOUNTING(X) \
  static_assert(!MOZ_IS_DESTRUCTIBLE(X) || \
                mozilla::HasDangerousPublicDestructor<X>::value, \
                "Reference-counted class " #X " should not have a public destructor. " \
                "Try to make this class's destructor non-public. If that is really " \
                "not possible, you can whitelist this class by providing a " \
                "HasDangerousPublicDestructor specialization for it."); \
  static_assert(!mozilla::HasDangerousPublicDestructor<X>::value || \
                MOZ_IS_DESTRUCTIBLE(X), \
                "Class " #X " has no public destructor. That's good! So please " \
                "remove the HasDangerousPublicDestructor specialization for it.");
#else
#define MOZ_ASSERT_TYPE_OK_FOR_REFCOUNTING(X)
#endif

inline nsISupports*
ToSupports(nsISupports* aSupports)
{
  return aSupports;
}

inline nsISupports*
ToCanonicalSupports(nsISupports* aSupports)
{
  return nullptr;
}




#if (defined(DEBUG) || (defined(NIGHTLY_BUILD) && !defined(MOZ_PROFILING))) && !defined(XPCOM_GLUE_AVOID_NSPR)

class nsAutoOwningThread
{
public:
  nsAutoOwningThread() { mThread = PR_GetCurrentThread(); }
  void* GetThread() const { return mThread; }

private:
  void* mThread;
};

#define NS_DECL_OWNINGTHREAD            nsAutoOwningThread _mOwningThread;
#define NS_ASSERT_OWNINGTHREAD_AGGREGATE(agg, _class) \
  NS_CheckThreadSafe(agg->_mOwningThread.GetThread(), #_class " not thread-safe")
#define NS_ASSERT_OWNINGTHREAD(_class) NS_ASSERT_OWNINGTHREAD_AGGREGATE(this, _class)
#else 

#define NS_DECL_OWNINGTHREAD
#define NS_ASSERT_OWNINGTHREAD_AGGREGATE(agg, _class) ((void)0)
#define NS_ASSERT_OWNINGTHREAD(_class)  ((void)0)

#endif 




#if defined(NS_BUILD_REFCNT_LOGGING) && !defined(MOZILLA_XPCOMRT_API)

#define NS_LOG_ADDREF(_p, _rc, _type, _size) \
  NS_LogAddRef((_p), (_rc), (_type), (uint32_t) (_size))

#define NS_LOG_RELEASE(_p, _rc, _type) \
  NS_LogRelease((_p), (_rc), (_type))

#include "mozilla/TypeTraits.h"
#define MOZ_ASSERT_CLASSNAME(_type)                         \
  static_assert(mozilla::IsClass<_type>::value,             \
                "Token '" #_type "' is not a class type.")


#if MOZ_IS_GCC
# if !MOZ_GCC_VERSION_AT_LEAST(4, 7, 0)
#  undef MOZ_ASSERT_CLASSNAME
#  define MOZ_ASSERT_CLASSNAME(_type)
# endif
#endif




#define MOZ_COUNT_CTOR(_type)                                 \
do {                                                          \
  MOZ_ASSERT_CLASSNAME(_type);                                \
  NS_LogCtor((void*)this, #_type, sizeof(*this));             \
} while (0)

#define MOZ_COUNT_CTOR_INHERITED(_type, _base)                    \
do {                                                              \
  MOZ_ASSERT_CLASSNAME(_type);                                    \
  MOZ_ASSERT_CLASSNAME(_base);                                    \
  NS_LogCtor((void*)this, #_type, sizeof(*this) - sizeof(_base)); \
} while (0)

#define MOZ_LOG_CTOR(_ptr, _name, _size) \
do {                                     \
  NS_LogCtor((void*)_ptr, _name, _size); \
} while (0)

#define MOZ_COUNT_DTOR(_type)                                 \
do {                                                          \
  MOZ_ASSERT_CLASSNAME(_type);                                \
  NS_LogDtor((void*)this, #_type, sizeof(*this));             \
} while (0)

#define MOZ_COUNT_DTOR_INHERITED(_type, _base)                    \
do {                                                              \
  MOZ_ASSERT_CLASSNAME(_type);                                    \
  MOZ_ASSERT_CLASSNAME(_base);                                    \
  NS_LogDtor((void*)this, #_type, sizeof(*this) - sizeof(_base)); \
} while (0)

#define MOZ_LOG_DTOR(_ptr, _name, _size) \
do {                                     \
  NS_LogDtor((void*)_ptr, _name, _size); \
} while (0)




#define NSCAP_LOG_ASSIGNMENT(_c, _p)                                \
  if (_p)                                                           \
    NS_LogCOMPtrAddRef((_c),static_cast<nsISupports*>(_p))

#define NSCAP_LOG_RELEASE(_c, _p)                                   \
  if (_p)                                                           \
    NS_LogCOMPtrRelease((_c), static_cast<nsISupports*>(_p))

#else 

#define NS_LOG_ADDREF(_p, _rc, _type, _size)
#define NS_LOG_RELEASE(_p, _rc, _type)
#define MOZ_COUNT_CTOR(_type)
#define MOZ_COUNT_CTOR_INHERITED(_type, _base)
#define MOZ_LOG_CTOR(_ptr, _name, _size)
#define MOZ_COUNT_DTOR(_type)
#define MOZ_COUNT_DTOR_INHERITED(_type, _base)
#define MOZ_LOG_DTOR(_ptr, _name, _size)

#endif 




#define NS_NUMBER_OF_FLAGS_IN_REFCNT 2
#define NS_IN_PURPLE_BUFFER (1 << 0)
#define NS_IS_PURPLE (1 << 1)
#define NS_REFCOUNT_CHANGE (1 << NS_NUMBER_OF_FLAGS_IN_REFCNT)
#define NS_REFCOUNT_VALUE(_val) (_val >> NS_NUMBER_OF_FLAGS_IN_REFCNT)

class nsCycleCollectingAutoRefCnt
{
public:
  nsCycleCollectingAutoRefCnt() : mRefCntAndFlags(0) {}

  explicit nsCycleCollectingAutoRefCnt(uintptr_t aValue)
    : mRefCntAndFlags(aValue << NS_NUMBER_OF_FLAGS_IN_REFCNT)
  {
  }

  MOZ_ALWAYS_INLINE uintptr_t incr(nsISupports* aOwner)
  {
    return incr(aOwner, nullptr);
  }

  MOZ_ALWAYS_INLINE uintptr_t incr(void* aOwner,
                                   nsCycleCollectionParticipant* aCp)
  {
    mRefCntAndFlags += NS_REFCOUNT_CHANGE;
    mRefCntAndFlags &= ~NS_IS_PURPLE;
    
    
    if (!IsInPurpleBuffer()) {
      mRefCntAndFlags |= NS_IN_PURPLE_BUFFER;
      
      MOZ_ASSERT(get() > 0);
      NS_CycleCollectorSuspect3(aOwner, aCp, this, nullptr);
    }
    return NS_REFCOUNT_VALUE(mRefCntAndFlags);
  }

  MOZ_ALWAYS_INLINE void stabilizeForDeletion()
  {
    
    
    mRefCntAndFlags = NS_REFCOUNT_CHANGE | NS_IN_PURPLE_BUFFER;
  }

  MOZ_ALWAYS_INLINE uintptr_t decr(nsISupports* aOwner,
                                   bool* aShouldDelete = nullptr)
  {
    return decr(aOwner, nullptr, aShouldDelete);
  }

  MOZ_ALWAYS_INLINE uintptr_t decr(void* aOwner,
                                   nsCycleCollectionParticipant* aCp,
                                   bool* aShouldDelete = nullptr)
  {
    MOZ_ASSERT(get() > 0);
    if (!IsInPurpleBuffer()) {
      mRefCntAndFlags -= NS_REFCOUNT_CHANGE;
      mRefCntAndFlags |= (NS_IN_PURPLE_BUFFER | NS_IS_PURPLE);
      uintptr_t retval = NS_REFCOUNT_VALUE(mRefCntAndFlags);
      
      NS_CycleCollectorSuspect3(aOwner, aCp, this, aShouldDelete);
      return retval;
    }
    mRefCntAndFlags -= NS_REFCOUNT_CHANGE;
    mRefCntAndFlags |= (NS_IN_PURPLE_BUFFER | NS_IS_PURPLE);
    return NS_REFCOUNT_VALUE(mRefCntAndFlags);
  }

  MOZ_ALWAYS_INLINE void RemovePurple()
  {
    MOZ_ASSERT(IsPurple(), "must be purple");
    mRefCntAndFlags &= ~NS_IS_PURPLE;
  }

  MOZ_ALWAYS_INLINE void RemoveFromPurpleBuffer()
  {
    MOZ_ASSERT(IsInPurpleBuffer());
    mRefCntAndFlags &= ~(NS_IS_PURPLE | NS_IN_PURPLE_BUFFER);
  }

  MOZ_ALWAYS_INLINE bool IsPurple() const
  {
    return !!(mRefCntAndFlags & NS_IS_PURPLE);
  }

  MOZ_ALWAYS_INLINE bool IsInPurpleBuffer() const
  {
    return !!(mRefCntAndFlags & NS_IN_PURPLE_BUFFER);
  }

  MOZ_ALWAYS_INLINE nsrefcnt get() const
  {
    return NS_REFCOUNT_VALUE(mRefCntAndFlags);
  }

  MOZ_ALWAYS_INLINE operator nsrefcnt() const
  {
    return get();
  }

private:
  uintptr_t mRefCntAndFlags;
};

class nsAutoRefCnt
{
public:
  nsAutoRefCnt() : mValue(0) {}
  explicit nsAutoRefCnt(nsrefcnt aValue) : mValue(aValue) {}

  
  nsrefcnt operator++() { return ++mValue; }
  nsrefcnt operator--() { return --mValue; }

  nsrefcnt operator=(nsrefcnt aValue) { return (mValue = aValue); }
  operator nsrefcnt() const { return mValue; }
  nsrefcnt get() const { return mValue; }

  static const bool isThreadSafe = false;
private:
  nsrefcnt operator++(int) = delete;
  nsrefcnt operator--(int) = delete;
  nsrefcnt mValue;
};

#ifndef XPCOM_GLUE
namespace mozilla {
class ThreadSafeAutoRefCnt
{
public:
  ThreadSafeAutoRefCnt() : mValue(0) {}
  explicit ThreadSafeAutoRefCnt(nsrefcnt aValue) : mValue(aValue) {}

  
  MOZ_ALWAYS_INLINE nsrefcnt operator++() { return ++mValue; }
  MOZ_ALWAYS_INLINE nsrefcnt operator--() { return --mValue; }

  MOZ_ALWAYS_INLINE nsrefcnt operator=(nsrefcnt aValue)
  {
    return (mValue = aValue);
  }
  MOZ_ALWAYS_INLINE operator nsrefcnt() const { return mValue; }
  MOZ_ALWAYS_INLINE nsrefcnt get() const { return mValue; }

  static const bool isThreadSafe = true;
private:
  nsrefcnt operator++(int) = delete;
  nsrefcnt operator--(int) = delete;
  
  
  
  Atomic<nsrefcnt> mValue;
};
}
#endif








#define NS_DECL_ISUPPORTS                                                     \
public:                                                                       \
  NS_IMETHOD QueryInterface(REFNSIID aIID,                                    \
                            void** aInstancePtr) override;                    \
  NS_IMETHOD_(MozExternalRefCountType) AddRef(void) override;                 \
  NS_IMETHOD_(MozExternalRefCountType) Release(void) override;                \
protected:                                                                    \
  nsAutoRefCnt mRefCnt;                                                       \
  NS_DECL_OWNINGTHREAD                                                        \
public:

#define NS_DECL_THREADSAFE_ISUPPORTS                                          \
public:                                                                       \
  NS_IMETHOD QueryInterface(REFNSIID aIID,                                    \
                            void** aInstancePtr) override;                    \
  NS_IMETHOD_(MozExternalRefCountType) AddRef(void) override;                 \
  NS_IMETHOD_(MozExternalRefCountType) Release(void) override;                \
protected:                                                                    \
  ::mozilla::ThreadSafeAutoRefCnt mRefCnt;                                    \
  NS_DECL_OWNINGTHREAD                                                        \
public:

#define NS_DECL_CYCLE_COLLECTING_ISUPPORTS                                    \
public:                                                                       \
  NS_IMETHOD QueryInterface(REFNSIID aIID,                                    \
                            void** aInstancePtr) override;                    \
  NS_IMETHOD_(MozExternalRefCountType) AddRef(void) override;                 \
  NS_IMETHOD_(MozExternalRefCountType) Release(void) override;                \
  NS_IMETHOD_(void) DeleteCycleCollectable(void);                             \
protected:                                                                    \
  nsCycleCollectingAutoRefCnt mRefCnt;                                        \
  NS_DECL_OWNINGTHREAD                                                        \
public:









#define NS_IMPL_CC_NATIVE_ADDREF_BODY(_class)                                 \
    MOZ_ASSERT_TYPE_OK_FOR_REFCOUNTING(_class)                                \
    MOZ_ASSERT(int32_t(mRefCnt) >= 0, "illegal refcnt");                      \
    NS_ASSERT_OWNINGTHREAD(_class);                                           \
    nsrefcnt count =                                                          \
      mRefCnt.incr(static_cast<void*>(this),                                  \
                   _class::NS_CYCLE_COLLECTION_INNERCLASS::GetParticipant()); \
    NS_LOG_ADDREF(this, count, #_class, sizeof(*this));                       \
    return count;

#define NS_IMPL_CC_NATIVE_RELEASE_BODY(_class)                                \
    MOZ_ASSERT(int32_t(mRefCnt) > 0, "dup release");                          \
    NS_ASSERT_OWNINGTHREAD(_class);                                           \
    nsrefcnt count =                                                          \
      mRefCnt.decr(static_cast<void*>(this),                                  \
                   _class::NS_CYCLE_COLLECTION_INNERCLASS::GetParticipant()); \
    NS_LOG_RELEASE(this, count, #_class);                                     \
    return count;

#define NS_IMPL_CYCLE_COLLECTING_NATIVE_ADDREF(_class)                        \
NS_METHOD_(MozExternalRefCountType) _class::AddRef(void)                      \
{                                                                             \
  NS_IMPL_CC_NATIVE_ADDREF_BODY(_class)                                       \
}

#define NS_IMPL_CYCLE_COLLECTING_NATIVE_RELEASE_WITH_LAST_RELEASE(_class, _last) \
NS_METHOD_(MozExternalRefCountType) _class::Release(void)                        \
{                                                                                \
    MOZ_ASSERT(int32_t(mRefCnt) > 0, "dup release");                             \
    NS_ASSERT_OWNINGTHREAD(_class);                                              \
    bool shouldDelete = false;                                                   \
    nsrefcnt count =                                                             \
      mRefCnt.decr(static_cast<void*>(this),                                     \
                   _class::NS_CYCLE_COLLECTION_INNERCLASS::GetParticipant(),     \
                   &shouldDelete);                                               \
    NS_LOG_RELEASE(this, count, #_class);                                        \
    if (count == 0) {                                                            \
        mRefCnt.incr(static_cast<void*>(this),                                   \
                     _class::NS_CYCLE_COLLECTION_INNERCLASS::GetParticipant());  \
        _last;                                                                   \
        mRefCnt.decr(static_cast<void*>(this),                                   \
                     _class::NS_CYCLE_COLLECTION_INNERCLASS::GetParticipant());  \
        if (shouldDelete) {                                                      \
            mRefCnt.stabilizeForDeletion();                                      \
            DeleteCycleCollectable();                                            \
        }                                                                        \
    }                                                                            \
    return count;                                                                \
}

#define NS_IMPL_CYCLE_COLLECTING_NATIVE_RELEASE(_class)                       \
NS_METHOD_(MozExternalRefCountType) _class::Release(void)                     \
{                                                                             \
  NS_IMPL_CC_NATIVE_RELEASE_BODY(_class)                                      \
}

#define NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(_class)            \
public:                                                                       \
  NS_METHOD_(MozExternalRefCountType) AddRef(void) {                          \
    NS_IMPL_CC_NATIVE_ADDREF_BODY(_class)                                     \
  }                                                                           \
  NS_METHOD_(MozExternalRefCountType) Release(void) {                         \
    NS_IMPL_CC_NATIVE_RELEASE_BODY(_class)                                    \
  }                                                                           \
protected:                                                                    \
  nsCycleCollectingAutoRefCnt mRefCnt;                                        \
  NS_DECL_OWNINGTHREAD                                                        \
public:











#define NS_INLINE_DECL_REFCOUNTING(_class, ...)                               \
public:                                                                       \
  NS_METHOD_(MozExternalRefCountType) AddRef(void) __VA_ARGS__ {              \
    MOZ_ASSERT_TYPE_OK_FOR_REFCOUNTING(_class)                                \
    MOZ_ASSERT(int32_t(mRefCnt) >= 0, "illegal refcnt");                      \
    NS_ASSERT_OWNINGTHREAD(_class);                                           \
    ++mRefCnt;                                                                \
    NS_LOG_ADDREF(this, mRefCnt, #_class, sizeof(*this));                     \
    return mRefCnt;                                                           \
  }                                                                           \
  NS_METHOD_(MozExternalRefCountType) Release(void) __VA_ARGS__ {             \
    MOZ_ASSERT(int32_t(mRefCnt) > 0, "dup release");                          \
    NS_ASSERT_OWNINGTHREAD(_class);                                           \
    --mRefCnt;                                                                \
    NS_LOG_RELEASE(this, mRefCnt, #_class);                                   \
    if (mRefCnt == 0) {                                                       \
      NS_ASSERT_OWNINGTHREAD(_class);                                         \
      mRefCnt = 1; /* stabilize */                                            \
      delete this;                                                            \
      return 0;                                                               \
    }                                                                         \
    return mRefCnt;                                                           \
  }                                                                           \
protected:                                                                    \
  nsAutoRefCnt mRefCnt;                                                       \
  NS_DECL_OWNINGTHREAD                                                        \
public:









#define NS_INLINE_DECL_THREADSAFE_REFCOUNTING(_class, ...)                    \
public:                                                                       \
  NS_METHOD_(MozExternalRefCountType) AddRef(void) __VA_ARGS__ {              \
    MOZ_ASSERT_TYPE_OK_FOR_REFCOUNTING(_class)                                \
    MOZ_ASSERT(int32_t(mRefCnt) >= 0, "illegal refcnt");                      \
    nsrefcnt count = ++mRefCnt;                                               \
    NS_LOG_ADDREF(this, count, #_class, sizeof(*this));                       \
    return (nsrefcnt) count;                                                  \
  }                                                                           \
  NS_METHOD_(MozExternalRefCountType) Release(void) __VA_ARGS__ {             \
    MOZ_ASSERT(int32_t(mRefCnt) > 0, "dup release");                          \
    nsrefcnt count = --mRefCnt;                                               \
    NS_LOG_RELEASE(this, count, #_class);                                     \
    if (count == 0) {                                                         \
      delete (this);                                                          \
      return 0;                                                               \
    }                                                                         \
    return count;                                                             \
  }                                                                           \
protected:                                                                    \
  ::mozilla::ThreadSafeAutoRefCnt mRefCnt;                                    \
public:





#define NS_IMPL_ADDREF(_class)                                                \
NS_IMETHODIMP_(MozExternalRefCountType) _class::AddRef(void)                  \
{                                                                             \
  MOZ_ASSERT_TYPE_OK_FOR_REFCOUNTING(_class)                                  \
  MOZ_ASSERT(int32_t(mRefCnt) >= 0, "illegal refcnt");                        \
  if (!mRefCnt.isThreadSafe)                                                  \
    NS_ASSERT_OWNINGTHREAD(_class);                                           \
  nsrefcnt count = ++mRefCnt;                                                 \
  NS_LOG_ADDREF(this, count, #_class, sizeof(*this));                         \
  return count;                                                               \
}








#define NS_IMPL_ADDREF_USING_AGGREGATOR(_class, _aggregator)                  \
NS_IMETHODIMP_(MozExternalRefCountType) _class::AddRef(void)                  \
{                                                                             \
  MOZ_ASSERT_TYPE_OK_FOR_REFCOUNTING(_class)                                  \
  NS_PRECONDITION(_aggregator, "null aggregator");                            \
  return (_aggregator)->AddRef();                                             \
}




















#define NS_IMPL_RELEASE_WITH_DESTROY(_class, _destroy)                        \
NS_IMETHODIMP_(MozExternalRefCountType) _class::Release(void)                 \
{                                                                             \
  MOZ_ASSERT(int32_t(mRefCnt) > 0, "dup release");                            \
  if (!mRefCnt.isThreadSafe)                                                  \
    NS_ASSERT_OWNINGTHREAD(_class);                                           \
  nsrefcnt count = --mRefCnt;                                                 \
  NS_LOG_RELEASE(this, count, #_class);                                       \
  if (count == 0) {                                                           \
    if (!mRefCnt.isThreadSafe)                                                \
      NS_ASSERT_OWNINGTHREAD(_class);                                         \
    mRefCnt = 1; /* stabilize */                                              \
    _destroy;                                                                 \
    return 0;                                                                 \
  }                                                                           \
  return count;                                                               \
}














#define NS_IMPL_RELEASE(_class) \
  NS_IMPL_RELEASE_WITH_DESTROY(_class, delete (this))








#define NS_IMPL_RELEASE_USING_AGGREGATOR(_class, _aggregator)                 \
NS_IMETHODIMP_(MozExternalRefCountType) _class::Release(void)                 \
{                                                                             \
  NS_PRECONDITION(_aggregator, "null aggregator");                            \
  return (_aggregator)->Release();                                            \
}


#define NS_IMPL_CYCLE_COLLECTING_ADDREF(_class)                               \
NS_IMETHODIMP_(MozExternalRefCountType) _class::AddRef(void)                  \
{                                                                             \
  MOZ_ASSERT_TYPE_OK_FOR_REFCOUNTING(_class)                                  \
  MOZ_ASSERT(int32_t(mRefCnt) >= 0, "illegal refcnt");                        \
  NS_ASSERT_OWNINGTHREAD(_class);                                             \
  nsISupports *base = NS_CYCLE_COLLECTION_CLASSNAME(_class)::Upcast(this);    \
  nsrefcnt count = mRefCnt.incr(base);                                        \
  NS_LOG_ADDREF(this, count, #_class, sizeof(*this));                         \
  return count;                                                               \
}

#define NS_IMPL_CYCLE_COLLECTING_RELEASE_WITH_DESTROY(_class, _destroy)       \
NS_IMETHODIMP_(MozExternalRefCountType) _class::Release(void)                 \
{                                                                             \
  MOZ_ASSERT(int32_t(mRefCnt) > 0, "dup release");                            \
  NS_ASSERT_OWNINGTHREAD(_class);                                             \
  nsISupports *base = NS_CYCLE_COLLECTION_CLASSNAME(_class)::Upcast(this);    \
  nsrefcnt count = mRefCnt.decr(base);                                        \
  NS_LOG_RELEASE(this, count, #_class);                                       \
  return count;                                                               \
}                                                                             \
NS_IMETHODIMP_(void) _class::DeleteCycleCollectable(void)                     \
{                                                                             \
  _destroy;                                                                   \
}

#define NS_IMPL_CYCLE_COLLECTING_RELEASE(_class)                              \
  NS_IMPL_CYCLE_COLLECTING_RELEASE_WITH_DESTROY(_class, delete (this))



#define NS_IMPL_CYCLE_COLLECTING_RELEASE_WITH_LAST_RELEASE(_class, _last)     \
NS_IMETHODIMP_(MozExternalRefCountType) _class::Release(void)                 \
{                                                                             \
  MOZ_ASSERT(int32_t(mRefCnt) > 0, "dup release");                            \
  NS_ASSERT_OWNINGTHREAD(_class);                                             \
  bool shouldDelete = false;                                                  \
  nsISupports *base = NS_CYCLE_COLLECTION_CLASSNAME(_class)::Upcast(this);    \
  nsrefcnt count = mRefCnt.decr(base, &shouldDelete);                         \
  NS_LOG_RELEASE(this, count, #_class);                                       \
  if (count == 0) {                                                           \
      mRefCnt.incr(base);                                                     \
      _last;                                                                  \
      mRefCnt.decr(base);                                                     \
      if (shouldDelete) {                                                     \
          mRefCnt.stabilizeForDeletion();                                     \
          DeleteCycleCollectable();                                           \
      }                                                                       \
  }                                                                           \
  return count;                                                               \
}                                                                             \
NS_IMETHODIMP_(void) _class::DeleteCycleCollectable(void)                     \
{                                                                             \
  delete this;                                                                \
}


















struct QITableEntry
{
  const nsIID* iid;     
  int32_t   offset;
};

nsresult NS_FASTCALL
NS_TableDrivenQI(void* aThis, REFNSIID aIID,
                 void** aInstancePtr, const QITableEntry* aEntries);





#define NS_INTERFACE_TABLE_HEAD(_class)                                       \
NS_IMETHODIMP _class::QueryInterface(REFNSIID aIID, void** aInstancePtr)      \
{                                                                             \
  NS_ASSERTION(aInstancePtr,                                                  \
               "QueryInterface requires a non-NULL destination!");            \
  nsresult rv = NS_ERROR_FAILURE;

#define NS_INTERFACE_TABLE_BEGIN                                              \
  static const QITableEntry table[] = {

#define NS_INTERFACE_TABLE_ENTRY(_class, _interface)                          \
  { &NS_GET_IID(_interface),                                                  \
    int32_t(reinterpret_cast<char*>(                                          \
                        static_cast<_interface*>((_class*) 0x1000)) -         \
               reinterpret_cast<char*>((_class*) 0x1000))                     \
  },

#define NS_INTERFACE_TABLE_ENTRY_AMBIGUOUS(_class, _interface, _implClass)    \
  { &NS_GET_IID(_interface),                                                  \
    int32_t(reinterpret_cast<char*>(                                          \
                        static_cast<_interface*>(                             \
                                       static_cast<_implClass*>(              \
                                                      (_class*) 0x1000))) -   \
               reinterpret_cast<char*>((_class*) 0x1000))                     \
  },







#define NS_INTERFACE_TABLE_END_WITH_PTR(_ptr)                                 \
  { nullptr, 0 } };                                                           \
  static_assert((sizeof(table)/sizeof(table[0])) > 1, "need at least 1 interface"); \
  rv = NS_TableDrivenQI(static_cast<void*>(_ptr),                             \
                        aIID, aInstancePtr, table);

#define NS_INTERFACE_TABLE_END                                                \
  NS_INTERFACE_TABLE_END_WITH_PTR(this)

#define NS_INTERFACE_TABLE_TAIL                                               \
  return rv;                                                                  \
}

#define NS_INTERFACE_TABLE_TAIL_INHERITING(_baseclass)                        \
  if (NS_SUCCEEDED(rv))                                                       \
    return rv;                                                                \
  return _baseclass::QueryInterface(aIID, aInstancePtr);                      \
}

#define NS_INTERFACE_TABLE_TAIL_USING_AGGREGATOR(_aggregator)                 \
  if (NS_SUCCEEDED(rv))                                                       \
    return rv;                                                                \
  NS_ASSERTION(_aggregator, "null aggregator");                               \
  return _aggregator->QueryInterface(aIID, aInstancePtr)                      \
}












#define NS_IMPL_QUERY_HEAD(_class)                                            \
NS_IMETHODIMP _class::QueryInterface(REFNSIID aIID, void** aInstancePtr)      \
{                                                                             \
  NS_ASSERTION(aInstancePtr,                                                  \
               "QueryInterface requires a non-NULL destination!");            \
  nsISupports* foundInterface;

#define NS_IMPL_QUERY_BODY(_interface)                                        \
  if ( aIID.Equals(NS_GET_IID(_interface)) )                                  \
    foundInterface = static_cast<_interface*>(this);                          \
  else

#define NS_IMPL_QUERY_BODY_CONDITIONAL(_interface, condition)                 \
  if ( (condition) && aIID.Equals(NS_GET_IID(_interface)))                    \
    foundInterface = static_cast<_interface*>(this);                          \
  else

#define NS_IMPL_QUERY_BODY_AMBIGUOUS(_interface, _implClass)                  \
  if ( aIID.Equals(NS_GET_IID(_interface)) )                                  \
    foundInterface = static_cast<_interface*>(                                \
                                    static_cast<_implClass*>(this));          \
  else

#define NS_IMPL_QUERY_BODY_AGGREGATED(_interface, _aggregate)                 \
  if ( aIID.Equals(NS_GET_IID(_interface)) )                                  \
    foundInterface = static_cast<_interface*>(_aggregate);                    \
  else

#define NS_IMPL_QUERY_TAIL_GUTS                                               \
    foundInterface = 0;                                                       \
  nsresult status;                                                            \
  if ( !foundInterface )                                                      \
    {                                                                         \
      /* nsISupports should be handled by this point. If not, fail. */        \
      MOZ_ASSERT(!aIID.Equals(NS_GET_IID(nsISupports)));                      \
      status = NS_NOINTERFACE;                                                \
    }                                                                         \
  else                                                                        \
    {                                                                         \
      NS_ADDREF(foundInterface);                                              \
      status = NS_OK;                                                         \
    }                                                                         \
  *aInstancePtr = foundInterface;                                             \
  return status;                                                              \
}

#define NS_IMPL_QUERY_TAIL_INHERITING(_baseclass)                             \
    foundInterface = 0;                                                       \
  nsresult status;                                                            \
  if ( !foundInterface )                                                      \
    status = _baseclass::QueryInterface(aIID, (void**)&foundInterface);       \
  else                                                                        \
    {                                                                         \
      NS_ADDREF(foundInterface);                                              \
      status = NS_OK;                                                         \
    }                                                                         \
  *aInstancePtr = foundInterface;                                             \
  return status;                                                              \
}

#define NS_IMPL_QUERY_TAIL_USING_AGGREGATOR(_aggregator)                      \
    foundInterface = 0;                                                       \
  nsresult status;                                                            \
  if ( !foundInterface ) {                                                    \
    NS_ASSERTION(_aggregator, "null aggregator");                             \
    status = _aggregator->QueryInterface(aIID, (void**)&foundInterface);      \
  } else                                                                      \
    {                                                                         \
      NS_ADDREF(foundInterface);                                              \
      status = NS_OK;                                                         \
    }                                                                         \
  *aInstancePtr = foundInterface;                                             \
  return status;                                                              \
}

#define NS_IMPL_QUERY_TAIL(_supports_interface)                               \
  NS_IMPL_QUERY_BODY_AMBIGUOUS(nsISupports, _supports_interface)              \
  NS_IMPL_QUERY_TAIL_GUTS








#define NS_INTERFACE_MAP_BEGIN(_implClass)      NS_IMPL_QUERY_HEAD(_implClass)
#define NS_INTERFACE_MAP_ENTRY(_interface)      NS_IMPL_QUERY_BODY(_interface)
#define NS_INTERFACE_MAP_ENTRY_CONDITIONAL(_interface, condition)             \
  NS_IMPL_QUERY_BODY_CONDITIONAL(_interface, condition)
#define NS_INTERFACE_MAP_ENTRY_AGGREGATED(_interface,_aggregate)              \
  NS_IMPL_QUERY_BODY_AGGREGATED(_interface,_aggregate)

#define NS_INTERFACE_MAP_END                    NS_IMPL_QUERY_TAIL_GUTS
#define NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(_interface, _implClass)              \
  NS_IMPL_QUERY_BODY_AMBIGUOUS(_interface, _implClass)
#define NS_INTERFACE_MAP_END_INHERITING(_baseClass)                           \
  NS_IMPL_QUERY_TAIL_INHERITING(_baseClass)
#define NS_INTERFACE_MAP_END_AGGREGATED(_aggregator)                          \
  NS_IMPL_QUERY_TAIL_USING_AGGREGATOR(_aggregator)

#define NS_INTERFACE_TABLE0(_class)                                           \
  NS_INTERFACE_TABLE_BEGIN                                                    \
    NS_INTERFACE_TABLE_ENTRY(_class, nsISupports)                             \
  NS_INTERFACE_TABLE_END

#define NS_INTERFACE_TABLE(aClass, ...)                                       \
  MOZ_STATIC_ASSERT_VALID_ARG_COUNT(__VA_ARGS__);                             \
  NS_INTERFACE_TABLE_BEGIN                                                    \
    MOZ_FOR_EACH(NS_INTERFACE_TABLE_ENTRY, (aClass,), (__VA_ARGS__))          \
    NS_INTERFACE_TABLE_ENTRY_AMBIGUOUS(aClass, nsISupports,                   \
                                       MOZ_ARG_1(__VA_ARGS__))                \
  NS_INTERFACE_TABLE_END

#define NS_IMPL_QUERY_INTERFACE0(_class)                                      \
  NS_INTERFACE_TABLE_HEAD(_class)                                             \
  NS_INTERFACE_TABLE0(_class)                                                 \
  NS_INTERFACE_TABLE_TAIL

#define NS_IMPL_QUERY_INTERFACE(aClass, ...)                                  \
  NS_INTERFACE_TABLE_HEAD(aClass)                                             \
  NS_INTERFACE_TABLE(aClass, __VA_ARGS__)                                     \
  NS_INTERFACE_TABLE_TAIL













#define NS_DECL_ISUPPORTS_INHERITED                                           \
public:                                                                       \
  NS_IMETHOD QueryInterface(REFNSIID aIID,                                    \
                            void** aInstancePtr) override;                \
  NS_IMETHOD_(MozExternalRefCountType) AddRef(void) override;             \
  NS_IMETHOD_(MozExternalRefCountType) Release(void) override;            \









#define NS_IMPL_ADDREF_INHERITED(Class, Super)                                \
NS_IMETHODIMP_(MozExternalRefCountType) Class::AddRef(void)                   \
{                                                                             \
  MOZ_ASSERT_TYPE_OK_FOR_REFCOUNTING(Class)                                   \
  nsrefcnt r = Super::AddRef();                                               \
  NS_LOG_ADDREF(this, r, #Class, sizeof(*this));                              \
  return r;                                                                   \
}

#define NS_IMPL_RELEASE_INHERITED(Class, Super)                               \
NS_IMETHODIMP_(MozExternalRefCountType) Class::Release(void)                  \
{                                                                             \
  nsrefcnt r = Super::Release();                                              \
  NS_LOG_RELEASE(this, r, #Class);                                            \
  return r;                                                                   \
}





#define NS_IMPL_NONLOGGING_ADDREF_INHERITED(Class, Super)                     \
NS_IMETHODIMP_(MozExternalRefCountType) Class::AddRef(void)                   \
{                                                                             \
  MOZ_ASSERT_TYPE_OK_FOR_REFCOUNTING(Class)                                   \
  return Super::AddRef();                                                     \
}

#define NS_IMPL_NONLOGGING_RELEASE_INHERITED(Class, Super)                    \
NS_IMETHODIMP_(MozExternalRefCountType) Class::Release(void)                  \
{                                                                             \
  return Super::Release();                                                    \
}

#define NS_INTERFACE_TABLE_INHERITED0(Class)

#define NS_INTERFACE_TABLE_INHERITED(aClass, ...)                             \
  MOZ_STATIC_ASSERT_VALID_ARG_COUNT(__VA_ARGS__);                             \
  NS_INTERFACE_TABLE_BEGIN                                                    \
    MOZ_FOR_EACH(NS_INTERFACE_TABLE_ENTRY, (aClass,), (__VA_ARGS__))          \
  NS_INTERFACE_TABLE_END

#define NS_IMPL_QUERY_INTERFACE_INHERITED0(aClass, aSuper)                    \
  NS_INTERFACE_TABLE_HEAD(aClass)                                             \
  NS_INTERFACE_TABLE_INHERITED0(aClass)                                       \
  NS_INTERFACE_TABLE_TAIL_INHERITING(aSuper)

#define NS_IMPL_QUERY_INTERFACE_INHERITED(aClass, aSuper, ...)                \
  NS_INTERFACE_TABLE_HEAD(aClass)                                             \
  NS_INTERFACE_TABLE_INHERITED(aClass, __VA_ARGS__)                           \
  NS_INTERFACE_TABLE_TAIL_INHERITING(aSuper)









#define NS_IMPL_ISUPPORTS0(_class)                                            \
  NS_IMPL_ADDREF(_class)                                                      \
  NS_IMPL_RELEASE(_class)                                                     \
  NS_IMPL_QUERY_INTERFACE0(_class)

#define NS_IMPL_ISUPPORTS(aClass, ...)                                        \
  NS_IMPL_ADDREF(aClass)                                                      \
  NS_IMPL_RELEASE(aClass)                                                     \
  NS_IMPL_QUERY_INTERFACE(aClass, __VA_ARGS__)

#define NS_IMPL_ISUPPORTS_INHERITED0(aClass, aSuper)                          \
    NS_IMPL_QUERY_INTERFACE_INHERITED0(aClass, aSuper)                        \
    NS_IMPL_ADDREF_INHERITED(aClass, aSuper)                                  \
    NS_IMPL_RELEASE_INHERITED(aClass, aSuper)                                 \

#define NS_IMPL_ISUPPORTS_INHERITED(aClass, aSuper, ...)                      \
  NS_IMPL_QUERY_INTERFACE_INHERITED(aClass, aSuper, __VA_ARGS__)              \
  NS_IMPL_ADDREF_INHERITED(aClass, aSuper)                                    \
  NS_IMPL_RELEASE_INHERITED(aClass, aSuper)






#define NS_INTERFACE_TABLE_TO_MAP_SEGUE \
  if (rv == NS_OK) return rv; \
  nsISupports* foundInterface;










#define NS_INTERFACE_MAP_END_THREADSAFE NS_IMPL_QUERY_TAIL_GUTS





#define NS_IMPL_THREADSAFE_CI(_class)                                         \
NS_IMETHODIMP                                                                 \
_class::GetInterfaces(uint32_t* _count, nsIID*** _array)                      \
{                                                                             \
  return NS_CI_INTERFACE_GETTER_NAME(_class)(_count, _array);                 \
}                                                                             \
                                                                              \
NS_IMETHODIMP                                                                 \
_class::GetScriptableHelper(nsIXPCScriptable** _retval)                       \
{                                                                             \
  *_retval = nullptr;                                                         \
  return NS_OK;                                                               \
}                                                                             \
                                                                              \
NS_IMETHODIMP                                                                 \
_class::GetContractID(char** _contractID)                                     \
{                                                                             \
  *_contractID = nullptr;                                                     \
  return NS_OK;                                                               \
}                                                                             \
                                                                              \
NS_IMETHODIMP                                                                 \
_class::GetClassDescription(char** _classDescription)                         \
{                                                                             \
  *_classDescription = nullptr;                                               \
  return NS_OK;                                                               \
}                                                                             \
                                                                              \
NS_IMETHODIMP                                                                 \
_class::GetClassID(nsCID** _classID)                                          \
{                                                                             \
  *_classID = nullptr;                                                        \
  return NS_OK;                                                               \
}                                                                             \
                                                                              \
NS_IMETHODIMP                                                                 \
_class::GetFlags(uint32_t* _flags)                                            \
{                                                                             \
  *_flags = nsIClassInfo::THREADSAFE;                                         \
  return NS_OK;                                                               \
}                                                                             \
                                                                              \
NS_IMETHODIMP                                                                 \
_class::GetClassIDNoAlloc(nsCID* _classIDNoAlloc)                             \
{                                                                             \
  return NS_ERROR_NOT_AVAILABLE;                                              \
}

#endif
